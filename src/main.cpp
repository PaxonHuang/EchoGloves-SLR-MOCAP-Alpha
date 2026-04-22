/**
 * Edge AI Data Glove - Main Entry Point
 * ESP32-S3-DevKitC-1 N16R8
 *
 * Phase: P1-P2 Complete - HAL + Signal Processing
 *
 * FreeRTOS Dual-Core Architecture:
 * - Core 1: Task_SensorRead (100Hz sampling) + Kalman filtering
 * - Core 0: Task_Inference + Task_Comms
 *
 * Hardware:
 * - 5x TMAG5273 Hall sensors (via TCA9548A channels 0-4)
 * - 1x BNO085 IMU (I2C address 0x4A)
 */

#include <Arduino.h>
#include <Wire.h>

// Sensor drivers
#include "TCA9548A.h"
#include "TMAG5273.h"
#include "BNO085Wrapper.h"

// Filters
#include "Kalman1D.h"

// I2C Configuration
#define I2C_SDA     8
#define I2C_SCL     9
#define I2C_SPEED   400000  // 400kHz Fast Mode

// Device addresses
#define TCA_ADDR    0x70
#define BNO_ADDR    0x4A

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
    float hall_xyz[15];  // 5 sensors × 3 axes (filtered)
    float euler[3];      // Roll, Pitch, Yaw (filtered)
    float gyro[3];       // Angular velocities (filtered)
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

// Sensor instances
TCA9548A i2cMux(&Wire, TCA_ADDR);
TMAG5273* hallSensors[5];  // Array of 5 Hall sensors
BNO085Wrapper imu(&Wire, BNO_ADDR);

// Kalman filters (15 Hall + 6 IMU = 21 total)
Kalman1D hallFilters[15];
Kalman1D imuFilters[6];

/**
 * Initialize I2C bus and all sensors
 */
bool initSensors() {
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_SPEED);

    Serial.println("Initializing sensors...");

    // Initialize TCA9548A multiplexer
    if (!i2cMux.begin()) {
        Serial.println("ERROR: TCA9548A not found!");
        return false;
    }
    Serial.println("TCA9548A multiplexer initialized");

    // Initialize 5 TMAG5273 Hall sensors
    for (uint8_t i = 0; i < 5; i++) {
        hallSensors[i] = new TMAG5273(&Wire, i, &i2cMux);
        if (!hallSensors[i]->begin()) {
            Serial.printf("ERROR: TMAG5273 channel %d not found!\n", i);
            return false;
        }
        Serial.printf("TMAG5273 channel %d initialized\n", i);
    }

    // Initialize BNO085 IMU
    if (!imu.begin()) {
        Serial.println("ERROR: BNO085 not found!");
        return false;
    }
    Serial.println("BNO085 IMU initialized");

    // Initialize Kalman filters
    // Hall sensors: Q=0.0001, R=0.01 (high noise, slow movement)
    for (int i = 0; i < 15; i++) {
        hallFilters[i].setParameters(0.0001f, 0.01f);
    }

    // IMU: Q=0.0001, R=0.005 (lower noise)
    for (int i = 0; i < 6; i++) {
        imuFilters[i].setParameters(0.0001f, 0.005f);
    }

    Serial.println("All sensors initialized successfully!");
    return true;
}

/**
 * Read all sensors with Kalman filtering
 */
void readAllSensors(SensorData &data) {
    // Read 5 TMAG5273 Hall sensors
    for (uint8_t i = 0; i < 5; i++) {
        float x, y, z;
        if (hallSensors[i]->readXYZ(x, y, z)) {
            // Apply Kalman filtering
            data.hall_xyz[i*3 + 0] = hallFilters[i*3 + 0].update(x);
            data.hall_xyz[i*3 + 1] = hallFilters[i*3 + 1].update(y);
            data.hall_xyz[i*3 + 2] = hallFilters[i*3 + 2].update(z);
        } else {
            // Sensor read failed, use last filtered value
            data.hall_xyz[i*3 + 0] = hallFilters[i*3 + 0].getState();
            data.hall_xyz[i*3 + 1] = hallFilters[i*3 + 1].getState();
            data.hall_xyz[i*3 + 2] = hallFilters[i*3 + 2].getState();
        }
    }

    // Read BNO085 IMU
    float roll, pitch, yaw, gx, gy, gz;
    if (imu.readAll(roll, pitch, yaw, gx, gy, gz)) {
        // Apply Kalman filtering
        data.euler[0] = imuFilters[0].update(roll);
        data.euler[1] = imuFilters[1].update(pitch);
        data.euler[2] = imuFilters[2].update(yaw);
        data.gyro[0] = imuFilters[3].update(gx);
        data.gyro[1] = imuFilters[4].update(gy);
        data.gyro[2] = imuFilters[5].update(gz);
    }
}

/**
 * Task_SensorRead - Core 1, Priority 3
 * 100Hz sensor sampling with precise timing + Kalman filtering
 */
void Task_SensorRead(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms = 100Hz

    Serial.println("Task_SensorRead started on Core 1");

    for (;;) {
        FullDataPacket packet;
        packet.timestamp = millis();

        // Read all sensors with filtering
        readAllSensors(packet.sensors);

        // Update sliding window for Edge Impulse
        // 15 Hall + 3 Euler + 3 Gyro = 21 features
        memcpy(window_buffer[buffer_idx], packet.sensors.hall_xyz, sizeof(float) * 15);
        memcpy(window_buffer[buffer_idx] + 15, packet.sensors.euler, sizeof(float) * 3);
        memcpy(window_buffer[buffer_idx] + 18, packet.sensors.gyro, sizeof(float) * 3);
        buffer_idx = (buffer_idx + 1) % EI_WINDOW_SIZE;

        // Output CSV for edge-impulse-data-forwarder
        // Format: 21 features comma-separated (no timestamp for EI)
        for (int i = 0; i < EI_FEATURE_COUNT; i++) {
            int idx = (buffer_idx + i) % EI_WINDOW_SIZE;  // Window from oldest to newest
            Serial.printf("%.4f", window_buffer[idx][i]);
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

    Serial.println("Task_Inference started on Core 0");

    for (;;) {
        if (xQueueReceive(inferenceQueue, &packet, portMAX_DELAY)) {
            InferenceResult res;
            res.packet = packet;
            res.gesture_id = 0;
            res.confidence = 0.0f;

            // TODO: Edge Impulse inference (Phase 3)
            // When window is full (30 frames), run inference:
            // signal_t signal;
            // ei_impulse_result_t result;
            // run_classifier(&signal, &result, false);
            // if (result.classification[i].value > 0.85) {
            //     res.gesture_id = i;
            //     res.confidence = result.classification[i].value;
            // }

            xQueueSend(dataQueue, &res, 0);
        }
    }
}

/**
 * Task_Comms - Core 0, Priority 1
 * BLE/WiFi communication (Phase 4)
 */
void Task_Comms(void *pvParameters) {
    InferenceResult res;

    Serial.println("Task_Comms started on Core 0");

    // TODO: BLE initialization (Phase 4)
    // BLEManager bleManager;
    // bleManager.begin();

    for (;;) {
        // TODO: Handle BLE/WiFi transmission

        if (xQueueReceive(dataQueue, &res, 10)) {
            // Process inference result
            if (res.gesture_id > 0) {
                Serial.printf("[GESTURE] ID=%u, Conf=%.2f\n",
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

    Serial.println("\n========================================");
    Serial.println("Edge AI Data Glove - ESP32-S3-DevKitC-1");
    Serial.println("Phase: P1-P2 HAL + Signal Processing");
    Serial.println("========================================\n");

    // Initialize sensors
    if (!initSensors()) {
        Serial.println("Sensor initialization failed! Halting.");
        while (1) { delay(1000); }
    }

    // Create queues
    inferenceQueue = xQueueCreate(10, sizeof(FullDataPacket));
    dataQueue = xQueueCreate(10, sizeof(InferenceResult));

    if (inferenceQueue == NULL || dataQueue == NULL) {
        Serial.println("ERROR: Queue creation failed!");
        return;
    }

    // Create FreeRTOS tasks with CORRECT parameter order
    xTaskCreatePinnedToCore(
        Task_SensorRead,        // Function pointer
        "SensorRead",
        4096,
        NULL,
        3,                      // Highest priority (100Hz critical)
        &TaskSensorReadHandle,
        1                       // Core 1 (Application CPU)
    );

    xTaskCreatePinnedToCore(
        Task_Inference,
        "Inference",
        8192,
        NULL,
        2,
        &TaskInferenceHandle,
        0                       // Core 0 (Protocol CPU)
    );

    xTaskCreatePinnedToCore(
        Task_Comms,
        "Comms",
        4096,
        NULL,
        1,                      // Lowest priority
        &TaskCommsHandle,
        0                       // Core 0
    );

    Serial.println("\nFreeRTOS tasks created:");
    Serial.println("  Core 1: SensorRead (100Hz)");
    Serial.println("  Core 0: Inference + Comms");
    Serial.println("\nReady for Edge Impulse data collection!");
    Serial.println("Run: edge-impulse-data-forwarder --baud-rate 115200");
    Serial.println("");
}

void loop() {
    // Main loop empty - FreeRTOS tasks handle everything
    vTaskDelete(NULL);
}