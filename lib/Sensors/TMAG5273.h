/**
 * TMAG5273A1 3D Hall Sensor Driver
 *
 * Texas Instruments linear 3D Hall effect sensor
 * - 12-bit resolution, ±40mT range
 * - I2C interface, address 0x22 (default)
 * - Set/Reset trigger mode for temperature drift compensation
 *
 * Datasheet: https://www.ti.com/product/TMAG5273
 */

#ifndef TMAG5273_H
#define TMAG5273_H

#include <Arduino.h>
#include <Wire.h>
#include "TCA9548A.h"

// Register definitions
#define TMAG5273_DEVICE_ID      0x00  // Device ID (expected 0x11)
#define TMAG5273_X_MSB          0x01
#define TMAG5273_X_LSB          0x02
#define TMAG5273_Y_MSB          0x03
#define TMAG5273_Y_LSB          0x04
#define TMAG5273_Z_MSB          0x05
#define TMAG5273_Z_LSB          0x06
#define TMAG5273_CONV_STATUS    0x07  // Conversion status
#define TMAG5273_DEVICE_CONFIG  0x08  // Device configuration
#define TMAG5273_SENSOR_CONFIG  0x09  // Sensor configuration
#define TMAG5273_X_THR_CONFIG   0x0A
#define TMAG5273_Y_THR_CONFIG   0x0B
#define TMAG5273_Z_THR_CONFIG   0x0C
#define TMAG5273_CONV_MODE      0x0D  // Conversion mode config
#define TMAG5273_I2C_ADDRESS    0x0E

// Device Config bits
#define TMAG5273_OPERATING_MODE_CONTINUOUS  0x00
#define TMAG5273_OPERATING_MODE_TRIGGER     0x01

// Sensor Config bits (averaging)
#define TMAG5273_AVG_1X   0x00
#define TMAG5273_AVG_2X   0x01
#define TMAG5273_AVG_4X   0x02
#define TMAG5273_AVG_8X   0x03
#define TMAG5273_AVG_16X  0x04
#define TMAG5273_AVG_32X  0x05

class TMAG5273 {
public:
    /**
     * Constructor
     * @param wire I2C bus instance
     * @param muxChannel Channel on TCA9548A multiplexer (0-4)
     * @param mux Pointer to TCA9548A instance
     * @param addr I2C address (default 0x22)
     */
    TMAG5273(TwoWire *wire, uint8_t muxChannel, TCA9548A *mux, uint8_t addr = 0x22)
        : _wire(wire), _muxChannel(muxChannel), _mux(mux), _addr(addr),
          _range(40.0f), _initialized(false) {}

    /**
     * Initialize sensor
     * @return true if device ID matches (0x11)
     */
    bool begin() {
        // Select multiplexer channel
        if (!_mux->selectChannel(_muxChannel)) {
            return false;
        }

        // Verify device ID
        uint8_t deviceId = readRegister(TMAG5273_DEVICE_ID);
        if (deviceId != 0x11) {
            Serial.printf("TMAG5273 at channel %d: Device ID mismatch (got 0x%02X, expected 0x11)\n",
                          _muxChannel, deviceId);
            return false;
        }

        // Configure sensor
        // Set trigger mode + 32x averaging for noise reduction
        writeRegister(TMAG5273_CONV_MODE, TMAG5273_OPERATING_MODE_TRIGGER | TMAG5273_AVG_32X);

        // Set ±40mT range (default)
        writeRegister(TMAG5273_SENSOR_CONFIG, 0x00);

        _initialized = true;
        return true;
    }

    /**
     * Read X/Y/Z magnetic field values
     * @param x Output: X axis field (mT)
     * @param y Output: Y axis field (mT)
     * @param z Output: Z axis field (mT)
     * @return true if successful
     */
    bool readXYZ(float &x, float &y, float &z) {
        if (!_initialized) {
            return false;
        }

        // Select multiplexer channel
        if (!_mux->selectChannel(_muxChannel)) {
            return false;
        }

        // Trigger conversion
        triggerConversion();

        // Wait for conversion complete (typically ~1ms for 32x avg)
        delay(2);

        // Read X_MSB and X_LSB
        uint8_t xMSB = readRegister(TMAG5273_X_MSB);
        uint8_t xLSB = readRegister(TMAG5273_X_LSB);

        // Read Y_MSB and Y_LSB
        uint8_t yMSB = readRegister(TMAG5273_Y_MSB);
        uint8_t yLSB = readRegister(TMAG5273_Y_LSB);

        // Read Z_MSB and Z_LSB
        uint8_t zMSB = readRegister(TMAG5273_Z_MSB);
        uint8_t zLSB = readRegister(TMAG5273_Z_LSB);

        // Convert 12-bit signed to float (mT)
        // 12-bit data in bits [11:0] of 16-bit word, bit 11 is sign
        x = convertToMT(xMSB, xLSB);
        y = convertToMT(yMSB, yLSB);
        z = convertToMT(zMSB, zLSB);

        return true;
    }

    /**
     * Trigger a single conversion
     */
    void triggerConversion() {
        // Set trigger mode bit to start conversion
        uint8_t mode = readRegister(TMAG5273_CONV_MODE);
        mode |= 0x80;  // Set trigger bit
        writeRegister(TMAG5273_CONV_MODE, mode);
    }

    /**
     * Check if conversion is complete
     * @return true if conversion complete
     */
    bool isConversionComplete() {
        uint8_t status = readRegister(TMAG5273_CONV_STATUS);
        return (status & 0x80) != 0;  // Bit 7 = CONV_COMPLETE
    }

    /**
     * Get multiplexer channel
     */
    uint8_t getMuxChannel() {
        return _muxChannel;
    }

private:
    TwoWire *_wire;
    uint8_t _muxChannel;
    TCA9548A *_mux;
    uint8_t _addr;
    float _range;  // ±40mT
    bool _initialized;

    /**
     * Read single register
     */
    uint8_t readRegister(uint8_t reg) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->endTransmission();

        _wire->requestFrom(_addr, (uint8_t)1);
        return _wire->read();
    }

    /**
     * Write single register
     */
    void writeRegister(uint8_t reg, uint8_t value) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->write(value);
        _wire->endTransmission();
    }

    /**
     * Convert 12-bit signed MSB+LSB to millitesla (mT)
     * MSB contains bits [11:4], LSB contains bits [3:0]
     * Bit 11 is sign bit
     * LSB = 0.048 mT for ±40mT range
     */
    float convertToMT(uint8_t msb, uint8_t lsb) {
        // Combine MSB (bits 11-4) and LSB (bits 3-0)
        int16_t raw = ((int16_t)msb << 4) | ((int16_t)lsb & 0x0F);

        // Sign extend 12-bit to 16-bit
        if (raw & 0x800) {
            raw |= 0xF000;  // Sign extend
        }

        // Convert to mT: LSB = 0.048 mT
        return raw * 0.048f;
    }
};

#endif  // TMAG5273_H