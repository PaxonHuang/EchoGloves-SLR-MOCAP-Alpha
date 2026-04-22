/**
 * BNO085 9-Axis IMU Driver Wrapper
 *
 * Bosch BNO085 with built-in SH-2 sensor fusion
 * - Hardware quaternion fusion (no MCU overhead)
 * - Game Rotation Vector (no magnetic interference)
 * - Calibrated gyroscope
 *
 * Uses Adafruit BNO08x library for SH-2 protocol handling
 */

#ifndef BNO085_WRAPPER_H
#define BNO085_WRAPPER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO08x.h>

// SH-2 Report IDs
#define SH2_GAME_ROTATION_VECTOR    0x05
#define SH2_GYROSCOPE_CALIBRATED    0x09

class BNO085Wrapper {
public:
    /**
     * Constructor
     * @param wire I2C bus instance
     * @param addr I2C address (default 0x4A)
     */
    BNO085Wrapper(TwoWire *wire, uint8_t addr = 0x4A)
        : _wire(wire), _addr(addr), _bno(addr) {}

    /**
     * Initialize BNO085 and enable reports
     * @return true if successful
     */
    bool begin() {
        // Initialize BNO085
        if (!_bno.begin_I2C(_addr, _wire)) {
            Serial.println("Failed to find BNO085 chip");
            return false;
        }

        // Adafruit BNO08x library handles initialization internally
        // Note: reset() method removed in newer library versions
        delay(100);

        // Enable Game Rotation Vector (quaternion, 100Hz)
        if (!_bno.enableReport(SH2_GAME_ROTATION_VECTOR, 100)) {
            Serial.println("Could not enable Game Rotation Vector");
            return false;
        }

        // Enable Calibrated Gyroscope (100Hz)
        if (!_bno.enableReport(SH2_GYROSCOPE_CALIBRATED, 100)) {
            Serial.println("Could not enable Calibrated Gyroscope");
            return false;
        }

        Serial.println("BNO085 initialized successfully");
        return true;
    }

    /**
     * Read quaternion (w, x, y, z)
     * @param qw Quaternion real component (w)
     * @param qx Quaternion imaginary (x)
     * @param qy Quaternion imaginary (y)
     * @param qz Quaternion imaginary (z)
     * @return true if data available
     */
    bool getQuaternion(float &qw, float &qx, float &qy, float &qz) {
        sh2_SensorValue_t sensorValue;

        if (_bno.getSensorEvent(&sensorValue)) {
            if (sensorValue.sensorId == SH2_GAME_ROTATION_VECTOR) {
                qw = sensorValue.un.gameRotationVector.real;
                qx = sensorValue.un.gameRotationVector.i;
                qy = sensorValue.un.gameRotationVector.j;
                qz = sensorValue.un.gameRotationVector.k;
                return true;
            }
        }
        return false;
    }

    /**
     * Read Euler angles (Roll, Pitch, Yaw) in degrees
     * @param roll Output: Roll angle (-180 to 180)
     * @param pitch Output: Pitch angle (-90 to 90)
     * @param yaw Output: Yaw angle (-180 to 180)
     * @return true if data available
     */
    bool getEuler(float &roll, float &pitch, float &yaw) {
        float qw, qx, qy, qz;

        if (getQuaternion(qw, qx, qy, qz)) {
            // Convert quaternion to Euler angles
            // Reference: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles

            // Roll (x-axis rotation)
            float sinr_cosp = 2.0f * (qw * qx + qy * qz);
            float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
            roll = atan2(sinr_cosp, cosr_cosp) * 180.0f / PI;

            // Pitch (y-axis rotation)
            float sinp = 2.0f * (qw * qy - qz * qx);
            if (fabs(sinp) >= 1.0f) {
                // Gimbal lock: use 90 degrees
                pitch = copysign(90.0f, sinp);
            } else {
                pitch = asin(sinp) * 180.0f / PI;
            }

            // Yaw (z-axis rotation)
            float siny_cosp = 2.0f * (qw * qz + qx * qy);
            float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
            yaw = atan2(siny_cosp, cosy_cosp) * 180.0f / PI;

            return true;
        }
        return false;
    }

    /**
     * Read calibrated gyroscope (rad/s converted to deg/s)
     * @param gx Output: X angular velocity (deg/s)
     * @param gy Output: Y angular velocity (deg/s)
     * @param gz Output: Z angular velocity (deg/s)
     * @return true if data available
     */
    bool getGyro(float &gx, float &gy, float &gz) {
        sh2_SensorValue_t sensorValue;

        if (_bno.getSensorEvent(&sensorValue)) {
            if (sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED) {
                // Convert rad/s to deg/s
                gx = sensorValue.un.gyroscope.x * 180.0f / PI;
                gy = sensorValue.un.gyroscope.y * 180.0f / PI;
                gz = sensorValue.un.gyroscope.z * 180.0f / PI;
                return true;
            }
        }
        return false;
    }

    /**
     * Read all sensor data in one call
     * @param roll Output: Roll angle (deg)
     * @param pitch Output: Pitch angle (deg)
     * @param yaw Output: Yaw angle (deg)
     * @param gx Output: Gyro X (deg/s)
     * @param gy Output: Gyro Y (deg/s)
     * @param gz Output: Gyro Z (deg/s)
     * @return true if all data available
     */
    bool readAll(float &roll, float &pitch, float &yaw,
                 float &gx, float &gy, float &gz) {
        bool eulerOK = getEuler(roll, pitch, yaw);
        bool gyroOK = getGyro(gx, gy, gz);
        return eulerOK && gyroOK;
    }

    /**
     * Check calibration status
     * @return Calibration level (0-3, 3 = fully calibrated)
     */
    uint8_t getCalibrationStatus() {
        sh2_SensorValue_t sensorValue;
        if (_bno.getSensorEvent(&sensorValue)) {
            // Calibration status embedded in sensor data
            return sensorValue.status;
        }
        return 0;
    }

private:
    TwoWire *_wire;
    uint8_t _addr;
    Adafruit_BNO08x _bno;
};

#endif  // BNO085_WRAPPER_H