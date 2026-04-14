#include <Arduino.h>
#include "SensorManager.h"
#include "FlexManager.h"
#include "UDPTransmitter.h"
#include "BLEManager.h"
#include "WSManager.h"
#include "model_data.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

SensorManager sensorManager;
FlexManager flexManager;
UDPTransmitter udpTransmitter("FallbackSSID", "FallbackPASS", "192.168.1.100", 8888);
BLEManager bleManager;
WSManager wsManager(81);

// TFLite Globals
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
constexpr int kTensorArenaSize = 64 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

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

// Sliding Window Buffer
float window_buffer[30][21];
int buffer_idx = 0;

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
        buffer_idx = (buffer_idx + 1) % 30;

        xQueueSend(inferenceQueue, &packet, 0);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Task_Inference(void *pvParameters) {
    FullDataPacket packet;
    for (;;) {
        if (xQueueReceive(inferenceQueue, &packet, portMAX_DELAY)) {
            // Fill TFLite input tensor
            for (int i = 0; i < 30; i++) {
                int idx = (buffer_idx + i) % 30;
                for (int j = 0; j < 21; j++) {
                    input->data.int8[i * 21 + j] = (int8_t)(window_buffer[idx][j] * 127.0f); // Simple quantization
                }
            }

            if (interpreter->Invoke() == kTfLiteOk) {
                int8_t max_val = -128;
                int max_idx = 0;
                for (int i = 0; i < 46; i++) {
                    if (output->data.int8[i] > max_val) {
                        max_val = output->data.int8[i];
                        max_idx = i;
                    }
                }

                InferenceResult res;
                res.packet = packet;
                res.gesture_id = (max_val > 108) ? max_idx : 0; // Threshold Tau = 0.85 -> 108 in INT8
                xQueueSend(dataQueue, &res, 0);
            }
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
    
    // TFLite Setup
    model = tflite::GetModel(g_model);
    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, nullptr);
    interpreter = &static_interpreter;
    interpreter->AllocateTensors();
    input = interpreter->input(0);
    output = interpreter->output(0);

    sensorManager.begin();
    flexManager.begin();

    dataQueue = xQueueCreate(10, sizeof(InferenceResult));
    inferenceQueue = xQueueCreate(10, sizeof(FullDataPacket));

    xTaskCreatePinnedToCore(TaskSensorReadHandle, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);
    xTaskCreatePinnedToCore(TaskInferenceHandle, "Inference", 8192, NULL, 2, &TaskInferenceHandle, 0);
    xTaskCreatePinnedToCore(TaskCommsHandle, "Comms", 8192, NULL, 1, &TaskCommsHandle, 0);
}
