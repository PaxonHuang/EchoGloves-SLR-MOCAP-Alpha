#ifndef FLEX_MANAGER_H
#define FLEX_MANAGER_H

#include <Arduino.h>

class FlexManager {
public:
    // Reserved Pins: GPIO 4, 5, 6, 7, 15
    const uint8_t flexPins[5] = {4, 5, 6, 7, 15};

    FlexManager() {}

    void begin() {
        for (int i = 0; i < 5; i++) {
            pinMode(flexPins[i], INPUT);
        }
    }

    void readAll(float *data) {
        for (int i = 0; i < 5; i++) {
            // Placeholder for future flex sensor logic
            // data[i] = analogRead(flexPins[i]);
            data[i] = 0.0f; 
        }
    }
};

#endif
