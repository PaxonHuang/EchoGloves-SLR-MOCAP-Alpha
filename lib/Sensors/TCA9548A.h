/**
 * TCA9548A I2C Multiplexer Driver
 *
 * 8-channel I2C switch for resolving address conflicts
 * Address: 0x70 (default, A0/A1/A2 all grounded)
 *
 * Usage:
 *   TCA9548A mux(&Wire, 0x70);
 *   mux.begin();
 *   mux.selectChannel(0);  // Select channel 0
 *   // Now read sensor on channel 0
 */

#ifndef TCA9548A_H
#define TCA9548A_H

#include <Arduino.h>
#include <Wire.h>

class TCA9548A {
public:
    /**
     * Constructor
     * @param wire I2C bus instance (Wire, Wire1, etc.)
     * @param addr I2C address (default 0x70)
     */
    TCA9548A(TwoWire *wire, uint8_t addr = 0x70)
        : _wire(wire), _addr(addr), _currentChannel(0xFF) {}

    /**
     * Initialize and verify I2C communication
     * @return true if device found
     */
    bool begin() {
        _wire->beginTransmission(_addr);
        uint8_t error = _wire->endTransmission();
        if (error == 0) {
            // Device found, disable all channels initially
            disableAll();
            return true;
        }
        return false;
    }

    /**
     * Select a single channel (0-7)
     * @param ch Channel number (0-7)
     * @return true if successful
     */
    bool selectChannel(uint8_t ch) {
        if (ch > 7) {
            return false;
        }

        _wire->beginTransmission(_addr);
        _wire->write(1 << ch);  // Bit mask for single channel
        uint8_t error = _wire->endTransmission();

        if (error == 0) {
            _currentChannel = ch;
            // Wait for bus stabilization (recommended 1ms)
            delay(1);
            return true;
        }
        return false;
    }

    /**
     * Disable all channels
     * @return true if successful
     */
    bool disableAll() {
        _wire->beginTransmission(_addr);
        _wire->write(0x00);  // All channels off
        uint8_t error = _wire->endTransmission();

        if (error == 0) {
            _currentChannel = 0xFF;
            return true;
        }
        return false;
    }

    /**
     * Enable multiple channels simultaneously
     * @param mask Bit mask (e.g., 0x05 = channels 0 and 2)
     * @return true if successful
     */
    bool enableChannels(uint8_t mask) {
        _wire->beginTransmission(_addr);
        _wire->write(mask);
        uint8_t error = _wire->endTransmission();

        if (error == 0) {
            delay(1);
            return true;
        }
        return false;
    }

    /**
     * Get currently selected channel
     * @return Channel number (0-7) or 0xFF if none/disabled
     */
    uint8_t getCurrentChannel() {
        return _currentChannel;
    }

private:
    TwoWire *_wire;
    uint8_t _addr;
    uint8_t _currentChannel;  // Track current channel (0xFF = none)
};

#endif  // TCA9548A_H