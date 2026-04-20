/**
 * Edge AI Data Glove - Main Entry Point
 * ESP32-S3-DevKitC-1 N16R8
 *
 * FreeRTOS Dual-Core Architecture:
 * - Core 1: Task_SensorRead (100Hz sampling)
 * - Core 0: Task_Inference + Task_Comms
 *
 * Phase: MVP - Edge Impulse Integration
 */

#include <Arduino.h>

// FreeRTOS Handles
TaskHandle_t TaskSensorReadHandle;
TaskHandle_t TaskInferenceHandle;
TaskHandle_t TaskCommsHandle;

// Queues for inter-task communication
QueueHandle_t inferenceQueue;
QueueHandle_t dataQueue;

// Sliding Window Buffer for Edge Impulse (30 frames × 21 features)
#define EI_WINDOW_SIZE 30
#define EI_FEATURE_COUNT 21
float window_buffer[EI_WINDOW_SIZE][EI_FEATURE_COUNT];
int buffer_idx = 0;

// Data structures
struct SensorData {
    float hall_xyz[15];  // 5 sensors × 3 axes
    float euler[3];      // Roll, Pitch, Yaw
    float gyro[3];       // Angular velocities
};

struct FullDataPacket {
    SensorData sensors;
    uint32_t timestamp;
};

struct InferenceResult {
    FullDataPacket packet;
    uint32_t gesture_id;
    float confidence;
};

/**
 * Task_SensorRead - Core 1, Priority 3
 * 100Hz sensor sampling with precise timing
 */
void Task_SensorRead(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms = 100Hz

    for (;;) {
        FullDataPacket packet;
        packet.timestamp = millis();

        // TODO: Read actual sensor data (Phase 1)
        // sensorManager.readAll(packet.sensors);

        // Placeholder: Simulate sensor data for testing
        for (int i = 0; i < 15; i++) {
            packet.sensors.hall_xyz[i] = 0.0f;
        }
        for (int i = 0; i < 3; i++) {
            packet.sensors.euler[i] = 0.0f;
            packet.sensors.gyro[i] = 0.0f;
        }

        // Update sliding window
        memcpy(window_buffer[buffer_idx], packet.sensors.hall_xyz, sizeof(float) * 15);
        memcpy(window_buffer[buffer_idx] + 15, packet.sensors.euler, sizeof(float) * 3);
        memcpy(window_buffer[buffer_idx] + 18, packet.sensors.gyro, sizeof(float) * 3);
        buffer_idx = (buffer_idx + 1) % EI_WINDOW_SIZE;

        // Output CSV for edge-impulse-data-forwarder
        // Format: 21 features comma-separated
        Serial.printf("%lu,", packet.timestamp);
        for (int i = 0; i < EI_FEATURE_COUNT; i++) {
            Serial.printf("%.4f", window_buffer[buffer_idx][i]);
            if (i < EI_FEATURE_COUNT - 1) {
                Serial.printf(",");
            }
        }
        Serial.println();

        // Send to inference queue
        xQueueSend(inferenceQueue, &packet, 0);

        // Precise 100Hz timing
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * Task_Inference - Core 0, Priority 2
 * Edge Impulse inference when window is full
 */
void Task_Inference(void *pvParameters) {
    FullDataPacket packet;

    for (;;) {
        if (xQueueReceive(inferenceQueue, &packet, portMAX_DELAY)) {
            InferenceResult res;
            res.packet = packet;
            res.gesture_id = 0;
            res.confidence = 0.0f;

            // TODO: Edge Impulse inference (Phase 3)
            // signal_t signal;
            // ei_impulse_result_t result;
            // run_classifier(&signal, &result, false);

            xQueueSend(dataQueue, &res, 0);
        }
    }
}

/**
 * Task_Comms - Core 0, Priority 1
 * BLE/WiFi communication
 */
void Task_Comms(void *pvParameters) {
    InferenceResult res;

    // TODO: BLE initialization (Phase 4)
    // bleManager.begin();

    for (;;) {
        // TODO: Handle BLE/WiFi transmission

        if (xQueueReceive(dataQueue, &res, 10)) {
            // Process inference result
            if (res.gesture_id > 0) {
                Serial.printf("Gesture: %u, Confidence: %.2f\n",
                              res.gesture_id, res.confidence);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("\n=== Edge AI Data Glove ===");
    Serial.println("ESP32-S3-DevKitC-1 N16R8");
    Serial.println("Phase: MVP Setup");
    Serial.println("===========================\n");

    // Create queues
    inferenceQueue = xQueueCreate(10, sizeof(FullDataPacket));
    dataQueue = xQueueCreate(10, sizeof(InferenceResult));

    if (inferenceQueue == NULL || dataQueue == NULL) {
        Serial.println("ERROR: Queue creation failed!");
        return;
    }

    // Create FreeRTOS tasks with CORRECT parameter order
    // xTaskCreatePinnedToCore(taskFunction, name, stackSize, params, priority, handle, coreID)

    xTaskCreatePinnedToCore(
        Task_SensorRead,    // Function pointer (NOT handle!)
        "SensorRead",
        4096,
        NULL,
        3,                  // Highest priority
        &TaskSensorReadHandle,
        1                   // Core 1
    );

    xTaskCreatePinnedToCore(
        Task_Inference,
        "Inference",
        8192,
        NULL,
        2,
        &TaskInferenceHandle,
        0                   // Core 0
    );

    xTaskCreatePinnedToCore(
        Task_Comms,
        "Comms",
        8192,
        NULL,
        1,                  // Lowest priority
        &TaskCommsHandle,
        0                   // Core 0
    );

    Serial.println("FreeRTOS tasks created successfully");
    Serial.println("Core 1: SensorRead (100Hz)");
    Serial.println("Core 0: Inference + Comms");
}

void loop() {
    // Main loop is empty - tasks handle everything
    vTaskDelete(NULL);
}