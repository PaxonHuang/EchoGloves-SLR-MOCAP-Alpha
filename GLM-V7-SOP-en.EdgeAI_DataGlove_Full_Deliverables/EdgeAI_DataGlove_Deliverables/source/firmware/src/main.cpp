#include <Arduino.h>
#include "SensorManager.h"
#include "FlexManager.h"
#include "UDPTransmitter.h"
#include "BLEManager.h"
#include "WSManager.h"

// NOTE: Uncomment this after exporting the Edge Impulse library
// #include <Edge_AI_Glove_inferencing.h>

SensorManager sensorManager;
FlexManager flexManager;
UDPTransmitter udpTransmitter("FallbackSSID", "FallbackPASS", "192.168.1.100", 8888);
BLEManager bleManager;
WSManager wsManager(81);

// FreeRTOS Handles
TaskHandle_t TaskSensorReadHandle;
TaskHandle_t TaskCommsHandle;
TaskHandle_t TaskInferenceHandle;
QueueHandle_t dataQueue;
QueueHandle_t inferenceQueue;

struct FullDataPacket {
    SensorManager::SensorData sensors;
    float flex[5];
    uint32_t timestamp;
};

struct InferenceResult {
    FullDataPacket packet;
    uint32_t gesture_id;
};

// Sliding Window Buffer for Edge Impulse (e.g., 30 frames * 21 features)
#define EI_WINDOW_SIZE 30
#define EI_FEATURE_COUNT 21
float window_buffer[EI_WINDOW_SIZE][EI_FEATURE_COUNT];
int buffer_idx = 0;

// Edge Impulse Signal Callback
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t out_idx = 0;
    for (size_t i = 0; i < length; i++) {
        size_t global_idx = offset + i;
        size_t frame_idx = (buffer_idx + (global_idx / EI_FEATURE_COUNT)) % EI_WINDOW_SIZE;
        size_t feature_idx = global_idx % EI_FEATURE_COUNT;
        out_ptr[out_idx++] = window_buffer[frame_idx][feature_idx];
    }
    return 0;
}

void Task_SensorRead(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    for (;;) {
        FullDataPacket packet;
        packet.timestamp = millis();
        sensorManager.readAll(packet.sensors);
        flexManager.readAll(packet.flex);

        // Update sliding window
        memcpy(window_buffer[buffer_idx], packet.sensors.hall_xyz, sizeof(float) * 15);
        memcpy(window_buffer[buffer_idx] + 15, packet.sensors.euler, sizeof(float) * 3);
        memcpy(window_buffer[buffer_idx] + 18, packet.sensors.gyro, sizeof(float) * 3);
        buffer_idx = (buffer_idx + 1) % EI_WINDOW_SIZE;

        // Print for edge-impulse-data-forwarder (CSV format)
        // Serial.printf("%.2f,%.2f,...\n", packet.sensors.hall_xyz[0], ...);

        xQueueSend(inferenceQueue, &packet, 0);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Task_Inference(void *pvParameters) {
    FullDataPacket packet;
    for (;;) {
        if (xQueueReceive(inferenceQueue, &packet, portMAX_DELAY)) {
            InferenceResult res;
            res.packet = packet;
            res.gesture_id = 0;

            /* Uncomment after adding Edge Impulse Library
            signal_t signal;
            int err = numpy::signal_from_buffer(raw_feature_get_data, EI_WINDOW_SIZE * EI_FEATURE_COUNT, &signal);
            if (err != 0) continue;

            ei_impulse_result_t result = { 0 };
            err = run_classifier(&signal, &result, false);
            if (err != EI_IMPULSE_OK) continue;

            // Find max confidence
            float max_val = 0;
            int max_idx = 0;
            for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                if (result.classification[i].value > max_val) {
                    max_val = result.classification[i].value;
                    max_idx = i;
                }
            }

            if (max_val > 0.85) {
                res.gesture_id = max_idx;
            }
            */

            xQueueSend(dataQueue, &res, 0);
        }
    }
}

void Task_Comms(void *pvParameters) {
    InferenceResult res;
    
    // Start BLE for provisioning
    bleManager.begin();
    
    // Wait for WiFi provisioning via BLE or timeout to fallback
    unsigned long startWait = millis();
    while (!bleManager.isProvisioned && (millis() - startWait < 15000)) {
        delay(100);
    }
    
    if (bleManager.isProvisioned) {
        WiFi.begin(bleManager.provisionedSSID.c_str(), bleManager.provisionedPASS.c_str());
    } else {
        WiFi.begin("FallbackSSID", "FallbackPASS");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
    
    // Start UDP and WebSocket
    udpTransmitter.begin();
    wsManager.begin();

    uint8_t buffer[256];

    for (;;) {
        wsManager.loop(); // Keep WebSocket alive

        if (xQueueReceive(dataQueue, &res, 10)) {
            GloveData msg = GloveData_init_zero;
            msg.timestamp = res.packet.timestamp;
            msg.hall_features_count = 15;
            memcpy(msg.hall_features, res.packet.sensors.hall_xyz, sizeof(float) * 15);
            msg.imu_features_count = 6;
            memcpy(msg.imu_features, res.packet.sensors.euler, sizeof(float) * 3);
            memcpy(msg.imu_features + 3, res.packet.sensors.gyro, sizeof(float) * 3);
            msg.flex_features_count = 5;
            memcpy(msg.flex_features, res.packet.flex, sizeof(float) * 5);
            msg.l1_gesture_id = res.gesture_id;

            // 1. Send UDP Broadcast for 3D Rendering (Low Latency, 100Hz)
            udpTransmitter.send(msg);

            // 2. If Gesture detected or L2 needed, send via WebSocket (Reliable TCP)
            if (res.gesture_id > 0) {
                pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
                if (pb_encode(&stream, GloveData_fields, &msg)) {
                    wsManager.broadcastMessage(buffer, stream.bytes_written);
                }
            }

            // 3. Low-speed BLE backup (e.g., every 5th frame = 20Hz)
            if (res.packet.timestamp % 50 < 10) {
                pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
                if (pb_encode(&stream, GloveData_fields, &msg)) {
                    bleManager.notifyData(buffer, stream.bytes_written);
                }
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    sensorManager.begin();
    flexManager.begin();

    dataQueue = xQueueCreate(10, sizeof(InferenceResult));
    inferenceQueue = xQueueCreate(10, sizeof(FullDataPacket));

    // FIXED: First param must be function pointer, NOT handle variable
    xTaskCreatePinnedToCore(Task_SensorRead, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);
    xTaskCreatePinnedToCore(Task_Inference, "Inference", 8192, NULL, 2, &TaskInferenceHandle, 0);
    xTaskCreatePinnedToCore(Task_Comms, "Comms", 8192, NULL, 1, &TaskCommsHandle, 0);
}
