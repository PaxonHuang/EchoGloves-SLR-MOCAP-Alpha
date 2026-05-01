/* =============================================================================
 * EdgeAI Data Glove V3 — Shared Data Structures & Constants
 * =============================================================================
 * Central header for all shared types, constants, and configuration values
 * used across sensor, filter, model, and communication modules.
 *
 * Included by:
 *   - SensorManager.h, FlexManager.h
 *   - KalmanFilter1D.h (indirectly via SensorManager)
 *   - BaseModel.h, TFLiteModel.h, ModelRegistry.h
 *   - BLEManager.h, UDPTransmitter.h
 *   - main.cpp (FreeRTOS queues)
 * ============================================================================= */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>
#include <cstring>

// =============================================================================
// Sensor Hardware Constants
// =============================================================================

/// Number of TMAG5273 Hall sensors (one per finger)
static constexpr uint8_t NUM_HALL_SENSORS = 5;

/// Number of flex sensor channels (reserved for V3.1)
static constexpr uint8_t NUM_FLEX_SENSORS = 5;

/// Hall sensor features per frame: 5 sensors × 3 axes = 15
static constexpr uint8_t HALL_FEATURE_COUNT = NUM_HALL_SENSORS * 3;  // 15

/// IMU features stored in sensor data: 4 quaternion + 3 gyro = 7
/// (BNO085 quaternion is already fused; euler is derived, not stored in KF)
static constexpr uint8_t IMU_FEATURE_COUNT = 7;

/// Total model input features per frame: 15 hall + 6 IMU (euler + gyro)
/// Quaternion is filtered internally but NOT part of the model feature vector
static constexpr uint16_t FEATURE_COUNT = HALL_FEATURE_COUNT + 6;  // 21

/// Number of gesture classes (Chinese Sign Language vocabulary)
static constexpr uint16_t NUM_CLASSES = 46;

/// Sliding window length in frames (300 ms at 100 Hz)
static constexpr uint16_t WINDOW_SIZE = 30;

// =============================================================================
// Pin Assignments
// =============================================================================

namespace I2CPins {
    static constexpr uint8_t SDA  = 8;
    static constexpr uint8_t SCL  = 9;
    static constexpr uint32_t FREQ = 400000;  // 400 kHz Fast Mode
}

namespace FlexPins {
    static constexpr uint8_t FLEX_0 = 4;   // Thumb
    static constexpr uint8_t FLEX_1 = 5;   // Index
    static constexpr uint8_t FLEX_2 = 6;   // Middle
    static constexpr uint8_t FLEX_3 = 7;   // Ring
    static constexpr uint8_t FLEX_4 = 15;  // Pinky
}

/// BNO085 interrupt pin (data-ready)
static constexpr uint8_t BNO085_INT_PIN = 21;

// =============================================================================
// Communication Constants
// =============================================================================

/// UDP telemetry port (ESP32 → Python Relay)
static constexpr uint16_t UDP_PORT = 8888;

/// BLE notification rate limit (Hz) — respects BLE MTU bandwidth
static constexpr uint16_t BLE_NOTIFY_RATE_HZ = 20;

// =============================================================================
// Mux Channel Assignments (TCA9548A)
// =============================================================================

namespace MuxChannels {
    static constexpr uint8_t HALL_SENSOR_0 = 0;  // Thumb
    static constexpr uint8_t HALL_SENSOR_1 = 1;  // Index
    static constexpr uint8_t HALL_SENSOR_2 = 2;  // Middle
    static constexpr uint8_t HALL_SENSOR_3 = 3;  // Ring
    static constexpr uint8_t HALL_SENSOR_4 = 4;  // Pinky
    static constexpr uint8_t BNO085_IMU      = 5;
}

// =============================================================================
// SensorData — Single-frame sensor reading (pushed to FreeRTOS queue)
// =============================================================================

struct SensorData {
    uint32_t timestamp_us;                           ///< Microsecond timestamp
    uint32_t seq;                                    ///< Monotonic sequence counter

    float hall_xyz[HALL_FEATURE_COUNT];              ///< 15 floats: [thumb_xyz, index_xyz, ..., pinky_z]
    float quaternion[4];                             ///< BNO085 quaternion [w, x, y, z]
    float euler[3];                                  ///< Euler angles [roll, pitch, yaw] in degrees
    float gyro[3];                                   ///< Gyroscope [gx, gy, gz] in deg/s
    float flex[NUM_FLEX_SENSORS];                    ///< Flex sensors (reserved, zeros in V3.0)

    /// Zero-initialize all fields
    void zero() {
        timestamp_us = 0;
        seq = 0;
        memset(hall_xyz, 0, sizeof(hall_xyz));
        memset(quaternion, 0, sizeof(quaternion));
        memset(euler, 0, sizeof(euler));
        memset(gyro, 0, sizeof(gyro));
        memset(flex, 0, sizeof(flex));
    }

    /**
     * @brief Flatten into a 21-element feature array for model input.
     * Order: [15 hall features, 3 euler angles, 3 gyro values]
     * @param out  Output buffer (must hold ≥ FEATURE_COUNT floats).
     */
    void toFeatureArray(float* out) const {
        memcpy(out, hall_xyz, HALL_FEATURE_COUNT * sizeof(float));
        memcpy(out + HALL_FEATURE_COUNT, euler, 3 * sizeof(float));
        memcpy(out + HALL_FEATURE_COUNT + 3, gyro, 3 * sizeof(float));
    }
};

// =============================================================================
// GestureResult — L1 inference output
// =============================================================================

struct GestureResult {
    int32_t  gesture_id;             ///< Predicted class index (0-based), -1 if unknown
    float    confidence;             ///< Softmax probability of the top prediction [0, 1]
    bool     valid;                  ///< true if confidence >= threshold and gesture confirmed
    bool     l2_requested;           ///< true if confidence is uncertain — trigger L2 inference
    uint32_t inference_time_us;      ///< Inference latency in microseconds
    float    scores[NUM_CLASSES];    ///< Per-class softmax probabilities

    void zero() {
        gesture_id = -1;
        confidence = 0.0f;
        valid = false;
        l2_requested = false;
        inference_time_us = 0;
        memset(scores, 0, sizeof(scores));
    }
};

// =============================================================================
// ModelInfo — Runtime model metadata
// =============================================================================

struct ModelInfo {
    char     name[32];              ///< Model name / alias
    char     type[32];              ///< Model type string (e.g., "CNNAttention", "MSTCN")
    uint16_t input_features;        ///< Features per frame (default 21)
    uint16_t window_size;           ///< Temporal window length (default 30)
    uint16_t num_classes;           ///< Output classes (default 46)
    uint32_t model_size_bytes;      ///< Binary size in flash
    uint32_t arena_size_bytes;      ///< Tensor arena allocation size
};

// =============================================================================
// FullDataPacket — Combined sensor + inference data (for BLE notification)
// =============================================================================

struct FullDataPacket {
    SensorData     sensor;
    GestureResult  inference;

    /// System status for debug / frontend display
    enum Status : uint8_t {
        STREAMING = 0,
        MODEL_SWITCHING,
        ERROR_STATE,
        CALIBRATING,
        IDLE
    } status;

    FullDataPacket() : status(STREAMING) {
        sensor.zero();
        inference.zero();
    }
};

// =============================================================================
// Confidence Thresholds
// =============================================================================

namespace ConfThreshold {
    static constexpr float L1_DIRECT     = 0.6f;   ///< L1 confident enough to output directly
    static constexpr float L2_REQUEST    = 0.3f;   ///< Below this, don't request L2 at all
    static constexpr float DEBOUNCE_MIN  = 0.5f;   ///< Min confidence for debouncing
}

/// Number of consecutive matching frames to confirm a gesture
static constexpr uint8_t DEBOUNCE_FRAMES = 5;

/// Static gesture transition silence period (ms)
static constexpr uint16_t TRANSITION_SILENCE_MS = 100;

#endif // DATA_STRUCTURES_H
