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
 * Build: platformio run -e esp32-s3-devkitc-1-n16r8
 * Monitor: platformio device monitor
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
#include "esp_psram.h"

// ---- Project Headers ----
#include "data_structures.h"

// ---- Sensor Libraries ----
#include "lib/Sensors/SensorManager.h"
#include "lib/Sensors/FlexManager.h"

// ---- Communication Libraries ----
#include "lib/Comms/BLEManager.h"
#include "lib/Comms/UDPTransmitter.h"

// ---- Model Libraries ----
#include "lib/Models/ModelRegistry.h"

// =============================================================================
// Global Objects
// =============================================================================

/// Unified sensor manager (I2C bus, mux, Hall sensors, IMU)
static SensorManager g_sensor_manager;

/// Flex sensor manager (ADC, currently placeholder)
static FlexManager g_flex_manager;

/// BLE provisioning + data notification manager
static BLEManager g_ble_manager;

/// WiFi UDP transmitter
static UDPTransmitter g_udp_transmitter;

/// Model registry for L1 inference + hot-switching
static ModelRegistry& g_model_registry = ModelRegistry::instance();

// =============================================================================
// FreeRTOS Queues
// =============================================================================

/// Queue for sensor data: SensorRead → Inference + Comms
static QueueHandle_t g_data_queue = nullptr;

/// Queue for inference results: Inference → Comms
static QueueHandle_t g_inference_queue = nullptr;

// =============================================================================
// FreeRTOS Task Handles
// =============================================================================

static TaskHandle_t g_task_sensor_handle    = nullptr;  ///< Core 1, Priority 3
static TaskHandle_t g_task_inference_handle = nullptr;  ///< Core 0, Priority 2
static TaskHandle_t g_task_comms_handle     = nullptr;  ///< Core 0, Priority 1

// =============================================================================
// Shared State (protected by mutex where needed)
// =============================================================================

/// System status flag (atomically written, read by comms task)
static volatile FullDataPacket::Status g_system_status = FullDataPacket::IDLE;

/// Latest inference result (written by inference task, read by comms task)
static volatile InferenceResult g_latest_inference = {};

/// Flag: WiFi is connected and UDP is ready
static volatile bool g_wifi_connected = false;

/// Ring buffer for sliding window (sensor task writes, inference task reads)
static SensorData g_ring_buffer[SENSOR_RING_SIZE];
static volatile uint16_t g_ring_head = 0;  // Write position (sensor task)
static volatile uint16_t g_ring_tail = 0;  // Read position (inference task)
static SemaphoreHandle_t g_ring_mutex = nullptr;  // Protects head/tail

// =============================================================================
// Function Prototypes (MUST match xTaskCreatePinnedToCore calls exactly)
// =============================================================================

void Task_SensorRead(void* pvParameters);
void Task_Inference(void* pvParameters);
void Task_Comms(void* pvParameters);

// =============================================================================
// STATIC ASSERT: Validate function pointer types at compile time
// =============================================================================
// This is the KEY safety check that would have caught the V2 bug.
// TaskFunction_t is defined as void (*)(void*).
// We verify each function has the correct signature.

static_assert(
    std::is_same<decltype(&Task_SensorRead), TaskFunction_t>::value,
    "Task_SensorRead must have signature void(*)(void*) for xTaskCreatePinnedToCore"
);

static_assert(
    std::is_same<decltype(&Task_Inference), TaskFunction_t>::value,
    "Task_Inference must have signature void(*)(void*) for xTaskCreatePinnedToCore"
);

static_assert(
    std::is_same<decltype(&Task_Comms), TaskFunction_t>::value,
    "Task_Comms must have signature void(*)(void*) for xTaskCreatePinnedToCore"
);

// Also verify that the handle variables are pointers (not functions!)
static_assert(
    std::is_pointer<TaskHandle_t>::value,
    "TaskHandle_t must be a pointer type (handle), NOT a function pointer"
);

// =============================================================================
// setup() — Arduino Framework Entry Point
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(500);  // Wait for serial monitor

    // ---- Print Boot Banner ----
    Serial.println("");
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║   EdgeAI Data Glove V3 — Firmware v3.0.0-alpha  ║");
    Serial.println("║   ESP32-S3 Dual-Core + 8MB PSRAM               ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.printf("  CPU Frequency : %lu MHz\n", getCpuFrequencyMhz());
    Serial.printf("  Free Heap     : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  PSRAM Size    : %u bytes\n", ESP.getPsramSize());
    Serial.printf("  PSRAM Free    : %u bytes\n", ESP.getFreePsram());
    Serial.printf("  Flash Size    : %u bytes\n", ESP.getFlashChipSize());
    Serial.println("");

    // ---- Step 1: Initialize Sensors ----
    Serial.println("[SETUP] Step 1/5: Initializing sensors...");
    g_system_status = FullDataPacket::CALIBRATING;

    if (!g_sensor_manager.begin()) {
        Serial.println("[SETUP] WARNING: Sensor init failed — running with degraded mode");
    }

    if (!g_flex_manager.begin()) {
        Serial.println("[SETUP] WARNING: Flex sensor init failed (expected in V3.0)");
    }

    // ---- Step 2: Initialize BLE Provisioning ----
    Serial.println("[SETUP] Step 2/5: Starting BLE provisioning...");
    g_ble_manager.begin("DataGlove-V3");

    // ---- Step 3: Create FreeRTOS Queues ----
    Serial.println("[SETUP] Step 3/5: Creating FreeRTOS queues...");

    // Data queue: carries FullDataPacket (sensor + inference)
    g_data_queue = xQueueCreate(DATA_QUEUE_DEPTH, sizeof(FullDataPacket));
    if (g_data_queue == nullptr) {
        Serial.println("[SETUP] FATAL: Failed to create dataQueue!");
        while (1) { delay(1000); }  // Halt
    }

    // Inference queue: carries InferenceResult
    g_inference_queue = xQueueCreate(INFERENCE_QUEUE_DEPTH, sizeof(InferenceResult));
    if (g_inference_queue == nullptr) {
        Serial.println("[SETUP] FATAL: Failed to create inferenceQueue!");
        while (1) { delay(1000); }  // Halt
    }

    // Ring buffer mutex
    g_ring_mutex = xSemaphoreCreateMutex();
    if (g_ring_mutex == nullptr) {
        Serial.println("[SETUP] FATAL: Failed to create ring mutex!");
        while (1) { delay(1000); }
    }

    Serial.printf("[SETUP] Queues created: dataQueue=%d, inferenceQueue=%d, ringBuf=%d\n",
                  DATA_QUEUE_DEPTH, INFERENCE_QUEUE_DEPTH, SENSOR_RING_SIZE);

    // ---- Step 4: Initialize Model Registry ----
    Serial.println("[SETUP] Step 4/5: Loading model configuration...");
    g_model_registry.loadConfig();
    g_model_registry.printRegisteredModels();

    // ---- Step 5: Create FreeRTOS Tasks ----
    Serial.println("[SETUP] Step 5/5: Creating FreeRTOS tasks...");

    // =========================================================================
    // TASK CREATION — CORRECT PARAMETER ORDER (V2 BUG FIX)
    // =========================================================================
    // xTaskCreatePinnedToCore(
    //     pvTaskCode,    — Function pointer (TaskFunction_t = void(*)(void*))
    //     pcName,        — Descriptive name (const char*)
    //     usStackDepth,  — Stack size in WORDS (not bytes!)
    //     pvParameters,  — Parameter passed to task (void*)
    //     uxPriority,    — Task priority (1=low, configMAX_PRIORITIES-1=high)
    //     pvCreatedTask, — [out] Task handle pointer (TaskHandle_t*)
    //     xCoreID        — Core to pin to (0 or 1 for ESP32-S3)
    // );
    // =========================================================================

    // ---- Task_SensorRead: Core 1, Priority 3, 100 Hz ----
    // Reads all sensors and fills ring buffer + data queue.
    // Stack: 10 KB (I2C operations + sensor data + log buffer)
    BaseType_t ret1 = xTaskCreatePinnedToCore(
        Task_SensorRead,         // ← FUNCTION POINTER (not the handle!)
        "SensorRead",            // Task name
        10000,                   // Stack size in words (10 KB)
        NULL,                    // No parameters
        3,                       // Priority 3 (highest — time-critical)
        &g_task_sensor_handle,   // ← Handle OUTPUT (pointer to handle)
        1                        // ← Core 1 (Protocol CPU)
    );

    if (ret1 != pdPASS) {
        Serial.printf("[SETUP] FATAL: Failed to create Task_SensorRead (err=%ld)\n", (long)ret1);
        while (1) { delay(1000); }
    }
    Serial.println("[SETUP] Task_SensorRead created on Core 1, Priority 3");

    // ---- Task_Inference: Core 0, Priority 2, ~30 Hz ----
    // Runs L1 inference on sliding window of sensor data.
    // Stack: 16 KB (TFLite interpreter needs extra stack)
    BaseType_t ret2 = xTaskCreatePinnedToCore(
        Task_Inference,          // ← FUNCTION POINTER (not the handle!)
        "Inference",             // Task name
        16000,                   // Stack size in words (16 KB for TFLite)
        NULL,                    // No parameters
        2,                       // Priority 2
        &g_task_inference_handle,// ← Handle OUTPUT (pointer to handle)
        0                        // ← Core 0 (Application CPU)
    );

    if (ret2 != pdPASS) {
        Serial.printf("[SETUP] FATAL: Failed to create Task_Inference (err=%ld)\n", (long)ret2);
        while (1) { delay(1000); }
    }
    Serial.println("[SETUP] Task_Inference created on Core 0, Priority 2");

    // ---- Task_Comms: Core 0, Priority 1, 100 Hz send ----
    // Manages BLE + WiFi UDP communication.
    // Stack: 8 KB (UDP + serialization + WiFi stack interaction)
    BaseType_t ret3 = xTaskCreatePinnedToCore(
        Task_Comms,              // ← FUNCTION POINTER (not the handle!)
        "Comms",                 // Task name
        8192,                    // Stack size in words (8 KB)
        NULL,                    // No parameters
        1,                       // Priority 1 (lowest)
        &g_task_comms_handle,    // ← Handle OUTPUT (pointer to handle)
        0                        // ← Core 0 (Application CPU)
    );

    if (ret3 != pdPASS) {
        Serial.printf("[SETUP] FATAL: Failed to create Task_Comms (err=%ld)\n", (long)ret3);
        while (1) { delay(1000); }
    }
    Serial.println("[SETUP] Task_Comms created on Core 0, Priority 1");

    // ---- Setup Complete ----
    g_system_status = FullDataPacket::STREAMING;
    Serial.println("");
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║   SETUP COMPLETE — All tasks running             ║");
    Serial.println("║   Waiting for BLE provisioning or WiFi connect  ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.println("");

    // Print CSV header for edge-impulse-data-forwarder
    Serial.println("timestamp,hall_0,hall_1,hall_2,hall_3,hall_4,hall_5,hall_6,hall_7,hall_8,hall_9,hall_10,hall_11,hall_12,hall_13,hall_14,euler_r,euler_p,euler_y,gyro_x,gyro_y,gyro_z,gesture_id,confidence");
}

// =============================================================================
// loop() — Arduino Framework Loop (UNUSED — FreeRTOS handles everything)
// =============================================================================

void loop() {
    // In V3, all work is done by FreeRTOS tasks.
    // This loop() is intentionally empty. The Arduino framework
    // requires it to exist, but it should never be reached in practice
    // because the scheduler is running the three tasks.

    // Watchdog + idle indicator
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Periodic heap/PSRAM monitoring (every 10 seconds)
    static uint32_t last_print = 0;
    uint32_t now = millis();
    if (now - last_print > 10000) {
        last_print = now;
        Serial.printf("[HEAP] Free: %u, Min: %u | PSRAM Free: %u\n",
                      ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getFreePsram());

        // If we're low on heap, that's a problem
        if (ESP.getFreeHeap() < 20000) {
            Serial.println("[HEAP] WARNING: Very low heap! Consider reducing stack sizes.");
        }
    }
}

// =============================================================================
// Task_SensorRead — Sensor Acquisition (Core 1, Priority 3, 100 Hz)
// =============================================================================
// Responsibilities:
//   1. Read all sensors via I2C (multiplexed)
//   2. Apply Kalman filtering (done inside SensorManager::readAll)
//   3. Write to ring buffer (for inference task sliding window)
//   4. Push to dataQueue (for comms task UDP send)
//   5. Output CSV to Serial (for edge-impulse-data-forwarder)
//
// Timing Budget:
//   - 5 Hall sensors × ~2 ms each (mux switch + conversion) = ~10 ms
//   - BNO085 IMU read = ~1 ms
//   - Total: ~11 ms per cycle, leaving ~-1 ms headroom at 100 Hz
//   - ACTUALLY: Hall conversion with 32× averaging takes ~6.5 ms each,
//     but we can overlap trigger+read with sequential reads.
//   - Realistic total: 40-60 ms → effective rate ~17-25 Hz per full scan
//   - For 100 Hz, reduce averaging to 4× or use parallel mux scanning
// =============================================================================

void Task_SensorRead(void* pvParameters) {
    (void)pvParameters;  // Unused

    const TickType_t task_interval = pdMS_TO_TICKS(10);  // 100 Hz = 10 ms
    TickType_t last_wake_time = xTaskGetTickCount();

    FullDataPacket packet;
    uint32_t loop_count = 0;
    uint32_t total_read_time_us = 0;
    uint32_t max_read_time_us = 0;

    Serial.println("[Task_SensorRead] Started on Core 1");

    // Wait for sensors to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    for (;;) {
        uint32_t t_start = esp_timer_get_time();

        // ---- Step 1: Read all sensors ----
        bool ok = g_sensor_manager.readAll(packet.sensor);

        if (ok) {
            // ---- Step 2: Read flex sensors (placeholder, returns zeros) ----
            g_flex_manager.readAll(packet.sensor.flex);

            // ---- Step 3: Write to ring buffer ----
            if (xSemaphoreTake(g_ring_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                g_ring_buffer[g_ring_head] = packet.sensor;
                g_ring_head = (g_ring_head + 1) % SENSOR_RING_SIZE;
                xSemaphoreGive(g_ring_mutex);
            }

            // ---- Step 4: Attach latest inference result ----
            packet.inference = g_latest_inference.gesture;
            packet.status = (FullDataPacket::Status)g_system_status;

            // ---- Step 5: Push to data queue (non-blocking) ----
            // If queue is full, drop the packet (Comms can't keep up)
            BaseType_t qret = xQueueSend(g_data_queue, &packet, 0);
            if (qret != pdTRUE) {
                // Queue full — Comms task is behind
                static uint32_t drop_count = 0;
                drop_count++;
                if (drop_count % 100 == 0) {
                    Serial.printf("[Task_SensorRead] WARNING: dataQueue overflow (dropped %lu)\n",
                                  (unsigned long)drop_count);
                }
            }

            // ---- Step 6: Serial CSV output (for edge-impulse-data-forwarder) ----
            // Output format: timestamp,21 features,gesture_id,confidence
            // edge-impulse-data-forwarder captures this and creates .csv files
            if (loop_count % 10 == 0) {  // Print every 10th sample (10 Hz to Serial)
                Serial.printf("%u,", packet.sensor.timestamp_us);
                for (int i = 0; i < FEATURE_COUNT; i++) {
                    float features[FEATURE_COUNT];
                    packet.sensor.toFeatureArray(features);
                    if (i < FEATURE_COUNT - 1) {
                        Serial.printf("%.4f,", features[i]);
                    } else {
                        Serial.printf("%.4f", features[i]);
                    }
                }
                Serial.printf(",%d,%.3f\n",
                              packet.inference.gesture_id,
                              packet.inference.confidence);
            }

            loop_count++;
        }

        // ---- Timing Statistics ----
        uint32_t t_end = esp_timer_get_time();
        uint32_t elapsed = t_end - t_start;
        total_read_time_us += elapsed;
        if (elapsed > max_read_time_us) max_read_time_us = elapsed;

        if (loop_count > 0 && loop_count % 1000 == 0) {
            float avg_us = (float)total_read_time_us / 1000.0f;
            Serial.printf("[Task_SensorRead] %lu samples | avg=%.1f us, max=%lu us\n",
                          (unsigned long)loop_count, avg_us, (unsigned long)max_read_time_us);
            total_read_time_us = 0;
            max_read_time_us = 0;
        }

        // ---- Yield to scheduler ----
        // Use vTaskDelayUntil for precise 100 Hz timing
        vTaskDelayUntil(&last_wake_time, task_interval);
    }

    // Should never reach here
    vTaskDelete(nullptr);
}

// =============================================================================
// Task_Inference — L1 On-Device Inference (Core 0, Priority 2, ~30 Hz)
// =============================================================================
// Responsibilities:
//   1. Check for pending model switch requests
//   2. Read WINDOW_SIZE (30) frames from ring buffer
//   3. Run L1 inference via ModelRegistry
//   4. Push InferenceResult to inferenceQueue
//
// Sliding Window Logic:
//   - Ring buffer is written by SensorRead at ~100 Hz
//   - This task reads 30-frame windows at ~30 Hz
//   - We simply take the last 30 frames from the ring buffer
//   - If buffer has fewer than 30 frames, we pad with zeros
//
// Inference Budget:
//   - CNN Attention V2: ~15-25 ms per inference on ESP32-S3 @ 240 MHz
//   - MS-TCN V1: ~8-12 ms per inference
//   - At 30 Hz target: 33 ms per cycle → CNN fits, MS-TCN has headroom
// =============================================================================

void Task_Inference(void* pvParameters) {
    (void)pvParameters;

    const TickType_t task_interval = pdMS_TO_TICKS(33);  // ~30 Hz
    TickType_t last_wake_time = xTaskGetTickCount();

    SensorData window[WINDOW_SIZE];
    InferenceResult result;
    uint32_t inference_count = 0;
    uint32_t total_inference_us = 0;
    uint32_t max_inference_us = 0;

    Serial.println("[Task_Inference] Started on Core 0");

    // Wait for model to be ready
    while (!g_model_registry.hasActiveModel()) {
        Serial.println("[Task_Inference] Waiting for active model...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Serial.printf("[Task_Inference] Active model: '%s' — starting inference loop\n",
                  g_model_registry.getActiveModelName());

    for (;;) {
        // ---- Step 0: Check for pending model switch ----
        char requested_model[32];
        if (g_model_registry.checkPendingSwitch(requested_model, sizeof(requested_model))) {
            Serial.printf("[Task_Inference] Switching model to '%s'...\n", requested_model);
            if (g_model_registry.switchModel(requested_model)) {
                Serial.printf("[Task_Inference] Model switched to '%s'\n",
                              g_model_registry.getActiveModelName());
            }
        }

        // ---- Step 1: Read WINDOW_SIZE frames from ring buffer ----
        if (xSemaphoreTake(g_ring_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Calculate how many valid frames are in the buffer
            uint16_t valid_count = 0;
            if (g_ring_head >= g_ring_tail) {
                valid_count = g_ring_head - g_ring_tail;
            } else {
                valid_count = SENSOR_RING_SIZE - g_ring_tail + g_ring_head;
            }

            // Take the last min(WINDOW_SIZE, valid_count) frames
            uint16_t frames_to_take = (valid_count < WINDOW_SIZE) ? valid_count : WINDOW_SIZE;
            uint16_t read_start;
            if (valid_count >= WINDOW_SIZE) {
                // Buffer has enough frames — take the last WINDOW_SIZE
                read_start = (g_ring_head + SENSOR_RING_SIZE - WINDOW_SIZE) % SENSOR_RING_SIZE;
            } else {
                // Not enough frames yet — take what we have
                read_start = (g_ring_head + SENSOR_RING_SIZE - valid_count) % SENSOR_RING_SIZE;
            }

            // Copy frames from ring buffer to local window
            for (uint16_t i = 0; i < frames_to_take; i++) {
                window[i] = g_ring_buffer[(read_start + i) % SENSOR_RING_SIZE];
            }

            // Zero-pad if we don't have enough frames yet
            for (uint16_t i = frames_to_take; i < WINDOW_SIZE; i++) {
                window[i].zero();
            }

            xSemaphoreGive(g_ring_mutex);
        } else {
            // Couldn't get mutex — skip this cycle
            vTaskDelayUntil(&last_wake_time, task_interval);
            continue;
        }

        // ---- Step 2: Run inference ----
        result.zero();
        result.timestamp_us = esp_timer_get_time();
        strncpy(result.model_name, g_model_registry.getActiveModelName(),
                sizeof(result.model_name) - 1);

        uint32_t t0 = esp_timer_get_time();
        int ret = g_model_registry.inferActive(window, WINDOW_SIZE, &result.gesture);
        uint32_t t1 = esp_timer_get_time();
        uint32_t inference_us = t1 - t0;

        total_inference_us += inference_us;
        if (inference_us > max_inference_us) max_inference_us = inference_us;

        if (ret == 0 && result.gesture.valid) {
            // Update shared latest inference (read by sensor task)
            g_latest_inference = result;

            // Push to inference queue for comms task
            xQueueSend(g_inference_queue, &result, 0);  // Non-blocking

            inference_count++;

            if (inference_count % 100 == 0) {
                float avg_us = (float)total_inference_us / 100.0f;
                Serial.printf("[Task_Inference] %lu inferences | avg=%.1f us, max=%lu us, "
                              "gesture=%d conf=%.2f (model='%s')\n",
                              (unsigned long)inference_count, avg_us,
                              (unsigned long)max_inference_us,
                              result.gesture.gesture_id, result.gesture.confidence,
                              result.model_name);
                total_inference_us = 0;
                max_inference_us = 0;
            }
        }

        // ---- Step 3: Yield to scheduler ----
        vTaskDelayUntil(&last_wake_time, task_interval);
    }

    vTaskDelete(nullptr);
}

// =============================================================================
// Task_Comms — BLE Provisioning + WiFi UDP (Core 0, Priority 1)
// =============================================================================
// Responsibilities:
//   1. Check BLE provisioning status
//   2. Connect to WiFi when credentials are provisioned
//   3. Drain dataQueue and send over UDP (100 Hz target)
//   4. Send data over BLE at 20 Hz (backup)
//   5. Handle incoming inference results
//
// WiFi Connection Strategy:
//   - Poll BLE provisioning status every 1 second
//   - Once provisioned, connect to WiFi and start UDP
//   - If WiFi drops, fall back to BLE-only mode
//   - Support re-provisioning via BLE clearCredentials()
//
// Data Flow:
//   dataQueue → pop FullDataPacket → merge with inference → UDP send
//   inferenceQueue → pop InferenceResult → update BLE status
// =============================================================================

void Task_Comms(void* pvParameters) {
    (void)pvParameters;

    FullDataPacket packet;
    InferenceResult inf_result;

    uint32_t udp_send_count = 0;
    uint32_t ble_send_count = 0;
    uint32_t provision_check_count = 0;

    Serial.println("[Task_Comms] Started on Core 0");
    Serial.println("[Task_Comms] Waiting for BLE WiFi provisioning...");

    for (;;) {
        // =====================================================================
        // Step 1: Check BLE Provisioning (every ~1 second)
        // =====================================================================
        provision_check_count++;
        if (provision_check_count % 100 == 0) {  // ~100 iterations × 10 ms = 1 s
            if (!g_wifi_connected && g_ble_manager.isProvisioned()) {
                Serial.println("[Task_Comms] WiFi credentials received via BLE!");
                Serial.printf("[Task_Comms] SSID: '%s', PASS: (%zu chars)\n",
                              g_ble_manager.provisionedSSID(),
                              strlen(g_ble_manager.provisionedPASS()));

                // Connect WiFi
                g_wifi_connected = g_udp_transmitter.begin(
                    g_ble_manager.provisionedSSID(),
                    g_ble_manager.provisionedPASS(),
                    "255.255.255.255",  // Broadcast
                    UDP_PORT
                );

                if (g_wifi_connected) {
                    g_ble_manager.setStatus("STREAMING");
                    g_system_status = FullDataPacket::STREAMING;
                    Serial.println("[Task_Comms] WiFi connected — switching to UDP mode");
                } else {
                    g_ble_manager.setStatus("WIFI_FAIL");
                    Serial.println("[Task_Comms] WiFi connection failed — staying in BLE mode");
                }
            }

            // Monitor WiFi health
            if (g_wifi_connected && WiFi.status() != WL_CONNECTED) {
                Serial.println("[Task_Comms] WiFi disconnected! Falling back to BLE...");
                g_wifi_connected = false;
                g_ble_manager.setStatus("WIFI_LOST");
                g_system_status = FullDataPacket::IDLE;
            }

            // Print link status
            if (g_wifi_connected) {
                Serial.printf("[Task_Comms] Link: WiFi (RSSI=%d, UDP sent=%lu)\n",
                              g_udp_transmitter.getRSSI(), (unsigned long)udp_send_count);
            } else {
                Serial.printf("[Task_Comms] Link: BLE only (notifications=%lu)\n",
                              (unsigned long)ble_send_count);
            }
        }

        // =====================================================================
        // Step 2: Drain inference queue
        // =====================================================================
        while (xQueueReceive(g_inference_queue, &inf_result, 0) == pdTRUE) {
            // Latest inference is already updated by inference task
            // (written to g_latest_inference)
            // Here we could trigger BLE/UDP specific actions
        }

        // =====================================================================
        // Step 3: Drain data queue and send
        // =====================================================================
        if (xQueueReceive(g_data_queue, &packet, pdMS_TO_TICKS(10)) == pdTRUE) {
            // ---- UDP Send (100 Hz, when connected) ----
            if (g_wifi_connected) {
                if (g_udp_transmitter.send(packet)) {
                    udp_send_count++;
                }
            }

            // ---- BLE Notify (20 Hz, always) ----
            g_ble_manager.notifyData(packet);
            ble_send_count++;

            // ---- Periodic stats ----
            if (udp_send_count > 0 && udp_send_count % 1000 == 0) {
                Serial.printf("[Task_Comms] Stats: UDP=%lu sent, %lu dropped, BLE=%lu notifies\n",
                              (unsigned long)g_udp_transmitter.packetsSent(),
                              (unsigned long)g_udp_transmitter.packetsDropped(),
                              (unsigned long)ble_send_count);
            }
        }

        // =====================================================================
        // Step 4: Short delay to avoid starving other Core 0 tasks
        // =====================================================================
        vTaskDelay(pdMS_TO_TICKS(1));  // ~1000 Hz loop rate
    }

    vTaskDelete(nullptr);
}

// =============================================================================
// END OF main.cpp
// =============================================================================
