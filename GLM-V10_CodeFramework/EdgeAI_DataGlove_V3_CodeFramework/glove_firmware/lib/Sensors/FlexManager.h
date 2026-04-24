/* =============================================================================
 * EdgeAI Data Glove V3 — Flex Sensor Manager (Reserved / Placeholder)
 * =============================================================================
 * Manages 5 flex (bend) sensors attached to ADC pins on the ESP32-S3.
 *
 * Current Status (V3.0): PLACEHOLDER
 *   - Hardware not yet populated on V3 PCB
 *   - readAll() returns zeros for all 5 channels
 *   - ADC configuration and calibration code is included but commented out
 *   - Will be activated in V3.1 when flex sensors are soldered
 *
 * ADC Configuration (when active):
 *   - ESP32-S3 ADC1, 12-bit resolution, ~3.3V range
 *   - Hardware-averaged: 8 samples per reading
 *   - Raw values normalized to 0.0 (straight) – 1.0 (fully bent)
 *
 * Pin Mapping:
 *   GPIO 4  → Thumb flex
 *   GPIO 5  → Index flex
 *   GPIO 6  → Middle flex
 *   GPIO 7  → Ring flex
 *   GPIO 15 → Pinky flex
 * =============================================================================
 */

#ifndef FLEX_MANAGER_H
#define FLEX_MANAGER_H

#include <Arduino.h>
#include "data_structures.h"

class FlexManager {
public:
    // =========================================================================
    // Construction
    // =========================================================================

    FlexManager() : _initialized(false) {}

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initialize ADC pins for flex sensor reading.
     *
     * In V3.0 this is a no-op — pins are configured but not read.
     * In V3.1, uncomment the analogReadResolution() and attenuation calls.
     *
     * @return true (always succeeds in V3.0 placeholder mode).
     */
    bool begin() {
        Serial.println("[FlexManager] V3.0 placeholder mode — returning zeros");

        // Configure ADC pins as inputs (low power)
        pinMode(FlexPins::FLEX_0, INPUT);
        pinMode(FlexPins::FLEX_1, INPUT);
        pinMode(FlexPins::FLEX_2, INPUT);
        pinMode(FlexPins::FLEX_3, INPUT);
        pinMode(FlexPins::FLEX_4, INPUT);

        // When flex sensors are populated, enable these:
        // analogReadResolution(12);               // 12-bit ADC
        // analogSetAttenuation(ADC_11db);         // Full 0–3.3V range
        // analogSetPinAttenuation(FlexPins::FLEX_0, ADC_11db);
        // analogSetPinAttenuation(FlexPins::FLEX_1, ADC_11db);
        // analogSetPinAttenuation(FlexPins::FLEX_2, ADC_11db);
        // analogSetPinAttenuation(FlexPins::FLEX_3, ADC_11db);
        // analogSetPinAttenuation(FlexPins::FLEX_4, ADC_11db);

        _initialized = true;
        return true;
    }

    // =========================================================================
    // Reading
    // =========================================================================

    /**
     * @brief Read all 5 flex sensors.
     *
     * In V3.0: Returns all zeros.
     * In V3.1: Will read ADC and normalize to 0.0–1.0 range.
     *
     * @param flex  Output array of 5 float values (normalized).
     */
    void readAll(float flex[NUM_FLEX_SENSORS]) {
        if (!_initialized) {
            memset(flex, 0, NUM_FLEX_SENSORS * sizeof(float));
            return;
        }

#if FLEX_SENSORS_POPULATED  // Will be defined in V3.1
        static const uint8_t pins[NUM_FLEX_SENSORS] = {
            FlexPins::FLEX_0, FlexPins::FLEX_1, FlexPins::FLEX_2,
            FlexPins::FLEX_3, FlexPins::FLEX_4
        };

        // Calibrated straight and bent ADC values (per sensor)
        // These must be populated during a calibration procedure
        static const int16_t straight_adc[NUM_FLEX_SENSORS] = {1800, 1800, 1800, 1800, 1800};
        static const int16_t bent_adc[NUM_FLEX_SENSORS]    = {2800, 2800, 2800, 2800, 2800};

        for (uint8_t i = 0; i < NUM_FLEX_SENSORS; i++) {
            // Average 8 readings for stability
            int32_t sum = 0;
            for (uint8_t s = 0; s < 8; s++) {
                sum += analogRead(pins[i]);
                delayMicroseconds(100);
            }
            int16_t avg = sum / 8;

            // Normalize to 0.0 (straight) – 1.0 (bent)
            float range = (float)(bent_adc[i] - straight_adc[i]);
            if (range > 0.0f) {
                flex[i] = constrain((avg - straight_adc[i]) / range, 0.0f, 1.0f);
            } else {
                flex[i] = 0.0f;
            }
        }
#else
        // V3.0: No flex sensors populated — return zeros
        memset(flex, 0, NUM_FLEX_SENSORS * sizeof(float));
#endif
    }

    bool isInitialized() const { return _initialized; }

private:
    bool _initialized;

    /// Pin array for iteration
    static constexpr uint8_t _pins[NUM_FLEX_SENSORS] = {
        FlexPins::FLEX_0, FlexPins::FLEX_1, FlexPins::FLEX_2,
        FlexPins::FLEX_3, FlexPins::FLEX_4
    };
};

#endif // FLEX_MANAGER_H
