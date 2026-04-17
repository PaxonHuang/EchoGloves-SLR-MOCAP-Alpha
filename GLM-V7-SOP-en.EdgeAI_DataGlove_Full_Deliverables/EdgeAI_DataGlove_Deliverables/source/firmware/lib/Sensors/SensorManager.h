#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Wire.h>
#include <SparkFun_TMAG5273_Arduino_Library.h>
#include <Adafruit_BNO08x.h>
#include "../../Filters/Kalman1D.h"

#define TCA_ADDR 0x70
#define BNO_ADDR 0x4A

class SensorManager {
public:
    SensorManager() {
        // Initialize Kalman Filters
        float dt = 0.01f;
        float hallQ = 0.01f * 0.01f * dt;
        float imuQ = 0.05f * 0.05f * dt;

        for (int i = 0; i < 15; i++) {
            hallFilters[i] = Kalman1D(hallQ, 0.1f);
        }
        for (int i = 0; i < 6; i++) {
            imuFilters[i] = Kalman1D(imuQ, 0.1f);
        }
    }

    bool begin() {
        Wire.begin(8, 9);
        
        if (!bno.begin_I2C(BNO_ADDR)) {
            Serial.println("Failed to find BNO085 chip");
            return false;
        }
        setBNOReports();

        for (uint8_t i = 0; i < 5; i++) {
            tcaSelect(i);
            if (tmag[i].begin() == false) {
                Serial.printf("Failed to find TMAG5273 at channel %d\n", i);
            }
            tmag[i].setOperatingMode(TMAG5273_OPERATING_MODE_CONTINUOUS);
        }
        return true;
    }

    void tcaSelect(uint8_t i) {
        if (i > 7) return;
        Wire.beginTransmission(TCA_ADDR);
        Wire.write(1 << i);
        Wire.endTransmission();
    }

    struct SensorData {
        float hall_xyz[15]; // 5 sensors * 3 axes
        float euler[3];     // Roll, Pitch, Yaw
        float gyro[3];      // Angular velocities
    };

    void readAll(SensorData &data) {
        // Read BNO085
        sh2_SensorValue_t sensorValue;
        if (bno.getSensorEvent(&sensorValue)) {
            if (sensorValue.sensorId == SH2_GAME_ROTATION_VECTOR) {
                float qi = sensorValue.un.gameRotationVector.i;
                float qj = sensorValue.un.gameRotationVector.j;
                float qk = sensorValue.un.gameRotationVector.k;
                float qr = sensorValue.un.gameRotationVector.real;

                // Convert to Euler (Eq 2, 3)
                float roll = atan2(2.0f * (qr * qi + qj * qk), 1.0f - 2.0f * (qi * qi + qj * qj));
                float pitch = asin(2.0f * (qr * qj - qk * qi));
                float yaw = atan2(2.0f * (qr * qk + qi * qj), 1.0f - 2.0f * (qj * qj + qk * qk));

                data.euler[0] = imuFilters[0].update(roll);
                data.euler[1] = imuFilters[1].update(pitch);
                data.euler[2] = imuFilters[2].update(yaw);
            }
            if (sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED) {
                data.gyro[0] = imuFilters[3].update(sensorValue.un.gyroscope.x);
                data.gyro[1] = imuFilters[4].update(sensorValue.un.gyroscope.y);
                data.gyro[2] = imuFilters[5].update(sensorValue.un.gyroscope.z);
            }
        }

        // Read TMAG5273
        for (uint8_t i = 0; i < 5; i++) {
            tcaSelect(i);
            data.hall_xyz[i*3 + 0] = hallFilters[i*3 + 0].update(tmag[i].getX());
            data.hall_xyz[i*3 + 1] = hallFilters[i*3 + 1].update(tmag[i].getY());
            data.hall_xyz[i*3 + 2] = hallFilters[i*3 + 2].update(tmag[i].getZ());
        }
    }

private:
    Adafruit_BNO08x bno;
    TMAG5273 tmag[5];
    Kalman1D hallFilters[15];
    Kalman1D imuFilters[6];

    void setBNOReports() {
        if (!bno.enableReport(SH2_GAME_ROTATION_VECTOR)) {
            Serial.println("Could not enable game rotation vector");
        }
        if (!bno.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
            Serial.println("Could not enable gyroscope");
        }
    }
};

#endif
