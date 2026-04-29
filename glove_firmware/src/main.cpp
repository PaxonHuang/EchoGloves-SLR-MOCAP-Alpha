/* =============================================================================
 * EdgeAI Data Glove V3 — Main Entry Point
 * =============================================================================
 * FreeRTOS dual-core task architecture for ESP32-S3:
 *
 *   Core 1 (Protocol CPU):
 *     └─ Task_SensorRead  (Priority 3, 100 Hz)
 *        - Reads all sensors via I2C (TCA9548A mux + 5× TMAG5273 + BNO085)
 *        - Applies Kalman filtering
 *        - Pushes FullDataPacket → dataQueue
 *        - Fills sliding window ring buffer
 *
 *   Core 0 (Application CPU):
 *     ├─ Task_Inference   (Priority 2, ~30 Hz)
 *     │  - Pops WINDOW_SIZE frames from ring buffer
 *     │  - Runs L1 inference via ModelRegistry
 *     │  - Pushes InferenceResult → inferenceQueue
 *     │
 *     └─ Task_Comms       (Priority 1, 100 Hz send / 20 Hz BLE)
 *        - Manages BLE provisioning (NimBLE)
 *        - Sends FullDataPacket over UDP (Nanopb serialized)
 *        - Falls back to BLE if WiFi unavailable
 *        - Handles incoming UDP commands (model switch, calibration)
 *
 * CRITICAL BUG FIX (from V2):
 *   V2 had WRONG xTaskCreatePinnedToCore parameter order — it passed the
 *   TaskHandle_t variable name as the function pointer:
 *
 *     WRONG: xTaskCreatePinnedToCore(TaskSensorReadHandle, "SensorRead", ...)
 *            ^^^^^^^^^^^^^^^^^^ This is a TaskHandle_t, NOT a function pointer!
 *            → Crash at boot or undefined behavior
 *
 *   V3 uses the CORRECT order with static_assert validation:
 *
 *     CORRECT: xTaskCreatePinnedToCore(Task_SensorRead, "SensorRead",
 *               10000, NULL, 3, &TaskSensorReadHandle, 1)
 *               ^^^^^^^^^^^^^^ This is a TaskFunction_t (void (*)(void*))
 *
 *   Parameter order: (function_ptr, name, stack_size, param, priority, handle_ptr, core_id)
 *
 * Build: pio run -e esp32-s3-devkitc-1-n16r8
 * Monitor: pio device monitor
 * =============================================================================
 */

#include <Arduino.h>

// ---- FreeRTOS Headers ----
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// ---- ESP-IDF Headers ----
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#ifdef BOARD_HAS_PSRAM
#include "esp32-hal-psram.h"
#endif

// ---- Project Headers ----
#include "data_structures.h"

// ---- Sensor Libraries ----
#include "SensorManager.h"
#include "FlexManager.h"

// ---- Communication Libraries ----
// #include "lib/Comms/BLEManager.h"
// #include "lib/Comms/UDPTransmitter.h"

// ---- Model Libraries ----
// #include "lib/Models/ModelRegistry.h"

// =============================================================================
// Global Objects
// =============================================================================

/// Unified sensor manager (I2C bus, mux, Hall sensors, IMU)
static SensorManager g_sensor_manager;

/// Flex sensor manager (ADC, currently placeholder)
static FlexManager g_flex_manager;

/// BLE provisioning + data notification manager
// static BLEManager g_ble_manager;

/// WiFi UDP transmitter
// static UDPTransmitter g_udp_transmitter;

/// Model registry for L1 inference + hot-switching
// static ModelRegistry& g_model_registry = ModelRegistry::instance();

// =============================================================================
// FreeRTOS Queues
// =============================================================================

/// Queue for sensor data: SensorRead → Inference + Comms
static QueueHandle_t g_data_queue = nullptr;

/// Queue for inference results: Inference → Comms
static QueueHandle_t g_inference_queue = nullptr;

// =============================================================================
// Task Handles (for static_assert validation)
// =============================================================================

static TaskHandle_t TaskSensorReadHandle = nullptr;
static TaskHandle_t TaskInferenceHandle = nullptr;
static TaskHandle_t TaskCommsHandle = nullptr;

// =============================================================================
// Task Function Prototypes
// =============================================================================

void Task_SensorRead(void* pvParameters);
void Task_Inference(void* pvParameters);
void Task_Comms(void* pvParameters);

// =============================================================================
// Static Asserts (compile-time verification of function pointers)
// =============================================================================

// Validate that task functions match TaskFunction_t signature
static_assert(std::is_same<decltype(&Task_SensorRead), TaskFunction_t>::value,
              "Task_SensorRead must have signature void(void*)");
static_assert(std::is_same<decltype(&Task_Inference), TaskFunction_t>::value,
              "Task_Inference must have signature void(void*)");
static_assert(std::is_same<decltype(&Task_Comms), TaskFunction_t>::value,
              "Task_Comms must have signature void(void*)");

// =============================================================================
// Setup
// =============================================================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);  // Allow serial to stabilize

    Serial.println("\n========================================");
    Serial.println("EdgeAI Data Glove V3");
    Serial.println("ESP32-S3-DevKitC-1 N16R8");
    Serial.println("========================================\n");

    // Check PSRAM availability
    Serial.printf("Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

    if (psramFound()) {
        Serial.printf("PSRAM found: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("WARNING: PSRAM not found!");
    }

    // Create queues
    g_data_queue = xQueueCreate(10, sizeof(SensorData));
    g_inference_queue = xQueueCreate(10, sizeof(InferenceResult));

    if (g_data_queue == nullptr || g_inference_queue == nullptr) {
        Serial.println("ERROR: Failed to create queues");
        return;
    }

    // Initialize components (to be implemented)
    // g_sensor_manager.init();
    // g_ble_manager.init();
    // g_udp_transmitter.init();
    // g_model_registry.init();

    // Create FreeRTOS tasks (pinned to specific cores)

    // Task_SensorRead: Core 1, Priority 3, 100Hz
    xTaskCreatePinnedToCore(
        Task_SensorRead,        // Function pointer (CORRECT!)
        "SensorRead",           // Task name
        10000,                  // Stack size (bytes)
        nullptr,                // Parameter
        3,                      // Priority
        &TaskSensorReadHandle,  // Task handle
        1                       // Core ID (1 = Protocol CPU)
    );

    // Task_Inference: Core 0, Priority 2, ~30Hz
    xTaskCreatePinnedToCore(
        Task_Inference,
        "Inference",
        16384,                  // Larger stack for ML inference
        nullptr,
        2,
        &TaskInferenceHandle,
        0                       // Core ID (0 = Application CPU)
    );

    // Task_Comms: Core 0, Priority 1, 100Hz send / 20Hz BLE
    xTaskCreatePinnedToCore(
        Task_Comms,
        "Comms",
        8192,
        nullptr,
        1,
        &TaskCommsHandle,
        0
    );

    Serial.println("FreeRTOS tasks created successfully");
    Serial.println("System ready\n");
}

// =============================================================================
// Main Loop (empty - all work in tasks)
// =============================================================================

void loop() {
    // All processing happens in FreeRTOS tasks
    // This loop can be used for idle processing or watchdog
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// =============================================================================
// Task Implementations
// =============================================================================

void Task_SensorRead(void* pvParameters) {
    Serial.println("[Task_SensorRead] Started on Core 1");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 100 Hz

    while (true) {
        // TODO: Read sensors via I2C mux
        // TODO: Apply Kalman filtering
        // TODO: Push to data queue

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Task_Inference(void* pvParameters) {
    Serial.println("[Task_Inference] Started on Core 0");

    while (true) {
        // TODO: Collect WINDOW_SIZE frames from ring buffer
        // TODO: Run L1 inference via ModelRegistry
        // TODO: Push result to inference queue

        vTaskDelay(pdMS_TO_TICKS(33));  // ~30 Hz
    }
}

void Task_Comms(void* pvParameters) {
    Serial.println("[Task_Comms] Started on Core 0");

    while (true) {
        // TODO: Check for BLE provisioning state
        // TODO: Send data over UDP (100 Hz)
        // TODO: Handle incoming commands

        vTaskDelay(pdMS_TO_TICKS(10));  // 100 Hz
    }
}
