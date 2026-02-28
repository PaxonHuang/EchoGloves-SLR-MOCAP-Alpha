# Signal Processing Skill
# 传感器信号处理与数据预处理

## 概述
本Skill专注于智能手套传感器数据的信号处理，包括MPU6050(IMU)数据滤波、弯曲传感器校准、特征提取和数据融合。

## 核心能力

### 1. 传感器数据滤波
- **卡尔曼滤波**: 最优状态估计
- **互补滤波**: 简单高效的姿态解算
- **滑动平均**: 基础噪声抑制
- **中值滤波**: 去除异常值

### 2. 数据融合
- **AHRS (姿态航向参考系统)**: 融合加速度计和陀螺仪
- **传感器标定**: 零偏和尺度因子校准
- **时间同步**: 多传感器数据对齐

### 3. 特征提取
- **时域特征**: 均值、方差、峰值、过零率
- **频域特征**: FFT频谱分析
- **时频特征**: 小波变换、STFT

## MPU6050专用处理

### 卡尔曼滤波器实现
```cpp
// kalman_filter.h
#pragma once

class KalmanFilter {
public:
    KalmanFilter(float process_noise = 0.01, float measurement_noise = 0.1) {
        Q = process_noise;      // 过程噪声协方差
        R = measurement_noise;  // 测量噪声协方差
        P = 1.0;                // 估计误差协方差
        X = 0.0;                // 状态估计
        K = 0.0;                // 卡尔曼增益
    }
    
    float update(float measurement) {
        // 预测
        float X_pred = X;           // 状态预测
        float P_pred = P + Q;       // 误差协方差预测
        
        // 更新
        K = P_pred / (P_pred + R);  // 计算卡尔曼增益
        X = X_pred + K * (measurement - X_pred);  // 状态更新
        P = (1 - K) * P_pred;       // 误差协方差更新
        
        return X;
    }
    
    void reset(float initial_value = 0.0) {
        X = initial_value;
        P = 1.0;
    }

private:
    float Q, R, P, X, K;
};

// 多轴卡尔曼滤波器
class KalmanFilter3D {
public:
    KalmanFilter3D(float process_noise = 0.01, float measurement_noise = 0.1)
        : kf_x(process_noise, measurement_noise),
          kf_y(process_noise, measurement_noise),
          kf_z(process_noise, measurement_noise) {}
    
    void update(float mx, float my, float mz, float& ox, float& oy, float& oz) {
        ox = kf_x.update(mx);
        oy = kf_y.update(my);
        oz = kf_z.update(mz);
    }
    
    void reset(float ix = 0, float iy = 0, float iz = 0) {
        kf_x.reset(ix);
        kf_y.reset(iy);
        kf_z.reset(iz);
    }

private:
    KalmanFilter kf_x, kf_y, kf_z;
};
```

### 互补滤波器 (AHRS)
```cpp
// complementary_filter.h
#pragma once
#include <math.h>

class ComplementaryFilter {
public:
    ComplementaryFilter(float alpha = 0.98) : alpha_(alpha) {
        roll_ = pitch_ = yaw_ = 0;
    }
    
    // 更新姿态角
    // accel: g (加速度，单位: m/s^2)
    // gyro: deg/s (陀螺仪，单位: 度/秒)
    // dt: 采样时间间隔(秒)
    void update(float accel[3], float gyro[3], float dt) {
        // 加速度计计算姿态角(低频)
        float accel_roll = atan2(accel[1], accel[2]) * 180 / PI;
        float accel_pitch = atan2(-accel[0], 
                                  sqrt(accel[1]*accel[1] + accel[2]*accel[2])) * 180 / PI;
        
        // 陀螺仪积分(高频)
        roll_ += gyro[0] * dt;
        pitch_ += gyro[1] * dt;
        yaw_ += gyro[2] * dt;
        
        // 互补滤波融合
        roll_ = alpha_ * roll_ + (1 - alpha_) * accel_roll;
        pitch_ = alpha_ * pitch_ + (1 - alpha_) * accel_pitch;
        // yaw角没有加速度计参考，使用纯积分
    }
    
    void getAngles(float& roll, float& pitch, float& yaw) {
        roll = roll_;
        pitch = pitch_;
        yaw = yaw_;
    }
    
    void reset() {
        roll_ = pitch_ = yaw_ = 0;
    }

private:
    float alpha_;      // 融合系数(0-1)
    float roll_, pitch_, yaw_;
};

// 改进版 - 四元数互补滤波
class QuaternionComplementaryFilter {
public:
    QuaternionComplementaryFilter(float beta = 0.1) : beta_(beta) {
        q0 = 1.0f; q1 = q2 = q3 = 0.0f;
    }
    
    void update(float accel[3], float gyro[3], float dt) {
        float recipNorm;
        float s0, s1, s2, s3;
        float qDot0, qDot1, qDot2, qDot3;
        float _2q0, _2q1, _2q2, _2q3;
        
        // 转换为rad/s
        float gx = gyro[0] * 0.0174533;
        float gy = gyro[1] * 0.0174533;
        float gz = gyro[2] * 0.0174533;
        
        // 归一化加速度
        recipNorm = 1.0f / sqrt(accel[0]*accel[0] + accel[1]*accel[1] + accel[2]*accel[2]);
        float ax = accel[0] * recipNorm;
        float ay = accel[1] * recipNorm;
        float az = accel[2] * recipNorm;
        
        // 计算目标方向
        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        
        s0 = _2q2 * ax - _2q1 * ay;
        s1 = -_2q3 * ax + _2q0 * ay + _2q1 * az;
        s2 = _2q0 * ax + _2q3 * ay - _2q2 * az;
        s3 = _2q1 * ax + _2q2 * ay + _2q3 * az;
        
        // 归一化步长
        recipNorm = 1.0f / sqrt(s0*s0 + s1*s1 + s2*s2 + s3*s3);
        s0 *= recipNorm;
        s1 *= recipNorm;
        s2 *= recipNorm;
        s3 *= recipNorm;
        
        // 计算四元数导数
        qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - beta_ * s0;
        qDot1 = 0.5f * (q0 * gx + q2 * gz - q3 * gy) - beta_ * s1;
        qDot2 = 0.5f * (q0 * gy - q1 * gz + q3 * gx) - beta_ * s2;
        qDot3 = 0.5f * (q0 * gz + q1 * gy - q2 * gx) - beta_ * s3;
        
        // 积分
        q0 += qDot0 * dt;
        q1 += qDot1 * dt;
        q2 += qDot2 * dt;
        q3 += qDot3 * dt;
        
        // 归一化
        recipNorm = 1.0f / sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
        q0 *= recipNorm;
        q1 *= recipNorm;
        q2 *= recipNorm;
        q3 *= recipNorm;
    }
    
    void getEulerAngles(float& roll, float& pitch, float& yaw) {
        roll = atan2(2*(q0*q1 + q2*q3), 1 - 2*(q1*q1 + q2*q2)) * 180 / PI;
        pitch = asin(2*(q0*q2 - q3*q1)) * 180 / PI;
        yaw = atan2(2*(q0*q3 + q1*q2), 1 - 2*(q2*q2 + q3*q3)) * 180 / PI;
    }

private:
    float beta_;
    float q0, q1, q2, q3;  // 四元数
};
```

## 弯曲传感器处理

### 校准和线性化
```cpp
// flex_sensor.h
#pragma once

class FlexSensor {
public:
    FlexSensor(int pin, float r_div = 10000.0, 
               float vcc = 3.3, int adc_resolution = 4095) 
        : pin_(pin), R_DIV_(r_div), VCC_(vcc), ADC_RES_(adc_resolution) {
        // 默认校准值
        flat_resistance_ = 25000.0;    // 平直时电阻(欧姆)
        bend_resistance_ = 100000.0;   // 最大弯曲时电阻(欧姆)
    }
    
    // 校准 - 记录平直和弯曲状态
    void calibrateFlat() {
        float sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += readResistance();
            delay(10);
        }
        flat_resistance_ = sum / 100.0;
    }
    
    void calibrateBend() {
        float sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += readResistance();
            delay(10);
        }
        bend_resistance_ = sum / 100.0;
    }
    
    // 读取电阻值
    float readResistance() {
        int adc = analogRead(pin_);
        float voltage = (adc * VCC_) / ADC_RES_;
        
        if (voltage < 0.1) return 0;  // 防止除以零
        
        return R_DIV_ * (VCC_ / voltage - 1.0);
    }
    
    // 读取角度 (0-90度)
    float readAngle() {
        float resistance = readResistance();
        
        // 限制范围
        if (resistance < flat_resistance_) resistance = flat_resistance_;
        if (resistance > bend_resistance_) resistance = bend_resistance_;
        
        // 线性映射到角度
        return map(resistance, flat_resistance_, bend_resistance_, 0, 90);
    }
    
    // 归一化值 (0-1)
    float readNormalized() {
        float angle = readAngle();
        return angle / 90.0;
    }
    
    void setCalibration(float flat, float bend) {
        flat_resistance_ = flat;
        bend_resistance_ = bend;
    }

private:
    int pin_;
    float R_DIV_, VCC_;
    int ADC_RES_;
    float flat_resistance_, bend_resistance_;
};

// 5传感器手套类
class SmartGloveSensors {
public:
    SmartGloveSensors(const int flex_pins[5], int mpu_addr = 0x68) {
        for (int i = 0; i < 5; i++) {
            flex_sensors_[i] = new FlexSensor(flex_pins[i]);
        }
        mpu_addr_ = mpu_addr;
    }
    
    void begin() {
        Wire.begin();
        // 初始化MPU6050
        mpu_.begin();
        mpu_.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu_.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
        
        // 初始化滤波器
        for (int i = 0; i < 3; i++) {
            accel_filter_[i] = new KalmanFilter(0.01, 0.1);
            gyro_filter_[i] = new KalmanFilter(0.01, 0.1);
        }
    }
    
    void calibrateAll() {
        Serial.println("校准弯曲传感器 - 请保持手指平直...");
        delay(2000);
        for (int i = 0; i < 5; i++) {
            flex_sensors_[i]->calibrateFlat();
        }
        
        Serial.println("校准弯曲传感器 - 请弯曲手指到最大角度...");
        delay(2000);
        for (int i = 0; i < 5; i++) {
            flex_sensors_[i]->calibrateBend();
        }
        Serial.println("校准完成!");
    }
    
    void readAll(float features[11]) {
        // 读取弯曲传感器 (5个)
        for (int i = 0; i < 5; i++) {
            features[i] = flex_sensors_[i]->readNormalized();
        }
        
        // 读取IMU
        sensors_event_t a, g, temp;
        mpu_.getEvent(&a, &g, &temp);
        
        // 应用滤波
        features[5] = accel_filter_[0]->update(a.acceleration.x) / 9.81;
        features[6] = accel_filter_[1]->update(a.acceleration.y) / 9.81;
        features[7] = accel_filter_[2]->update(a.acceleration.z) / 9.81;
        features[8] = gyro_filter_[0]->update(g.gyro.x);
        features[9] = gyro_filter_[1]->update(g.gyro.y);
        features[10] = gyro_filter_[2]->update(g.gyro.z);
    }

private:
    FlexSensor* flex_sensors_[5];
    Adafruit_MPU6050 mpu_;
    int mpu_addr_;
    KalmanFilter* accel_filter_[3];
    KalmanFilter* gyro_filter_[3];
};
```

## 特征提取

### 时域特征
```cpp
// feature_extraction.h
#pragma once
#include <math.h>

class FeatureExtractor {
public:
    // 计算均值
    static float mean(float* data, int len) {
        float sum = 0;
        for (int i = 0; i < len; i++) sum += data[i];
        return sum / len;
    }
    
    // 计算标准差
    static float std(float* data, int len) {
        float m = mean(data, len);
        float sum = 0;
        for (int i = 0; i < len; i++) {
            sum += (data[i] - m) * (data[i] - m);
        }
        return sqrt(sum / len);
    }
    
    // 计算最大值
    static float max(float* data, int len) {
        float m = data[0];
        for (int i = 1; i < len; i++) {
            if (data[i] > m) m = data[i];
        }
        return m;
    }
    
    // 计算最小值
    static float min(float* data, int len) {
        float m = data[0];
        for (int i = 1; i < len; i++) {
            if (data[i] < m) m = data[i];
        }
        return m;
    }
    
    // 过零率
    static int zeroCrossingRate(float* data, int len) {
        int count = 0;
        for (int i = 1; i < len; i++) {
            if ((data[i-1] > 0 && data[i] < 0) || (data[i-1] < 0 && data[i] > 0)) {
                count++;
            }
        }
        return count;
    }
    
    // 滑动窗口特征提取
    struct WindowFeatures {
        float mean[11];
        float std[11];
        float max[11];
        float min[11];
        float energy[11];  // 信号能量
    };
    
    static void extractWindowFeatures(float window[11][100], WindowFeatures& features) {
        for (int ch = 0; ch < 11; ch++) {
            features.mean[ch] = mean(window[ch], 100);
            features.std[ch] = std(window[ch], 100);
            features.max[ch] = max(window[ch], 100);
            features.min[ch] = min(window[ch], 100);
            
            // 计算能量
            float energy = 0;
            for (int i = 0; i < 100; i++) {
                energy += window[ch][i] * window[ch][i];
            }
            features.energy[ch] = energy / 100;
        }
    }
};
```

## 实用工具函数

### 滑动窗口
```cpp
// sliding_window.h
#pragma once

template<typename T, int WINDOW_SIZE, int NUM_CHANNELS>
class SlidingWindow {
public:
    SlidingWindow() : head_(0), count_(0) {}
    
    void push(T data[NUM_CHANNELS]) {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            buffer_[head_][i] = data[i];
        }
        head_ = (head_ + 1) % WINDOW_SIZE;
        if (count_ < WINDOW_SIZE) count_++;
    }
    
    bool isFull() {
        return count_ == WINDOW_SIZE;
    }
    
    void getWindow(T output[WINDOW_SIZE][NUM_CHANNELS]) {
        for (int i = 0; i < WINDOW_SIZE; i++) {
            int idx = (head_ + i) % WINDOW_SIZE;
            for (int j = 0; j < NUM_CHANNELS; j++) {
                output[i][j] = buffer_[idx][j];
            }
        }
    }
    
    void clear() {
        head_ = 0;
        count_ = 0;
    }

private:
    T buffer_[WINDOW_SIZE][NUM_CHANNELS];
    int head_;
    int count_;
};
```

## 使用示例

```cpp
#include "flex_sensor.h"
#include "kalman_filter.h"
#include "complementary_filter.h"

// 传感器引脚
const int FLEX_PINS[5] = {36, 39, 34, 35, 32};
SmartGloveSensors glove(FLEX_PINS);

// 互补滤波器
QuaternionComplementaryFilter ahrs(0.1);

void setup() {
    Serial.begin(115200);
    glove.begin();
    glove.calibrateAll();
}

void loop() {
    float features[11];
    glove.readAll(features);
    
    // 输出处理后的特征
    Serial.print("Flex: ");
    for (int i = 0; i < 5; i++) {
        Serial.print(features[i]);
        Serial.print(" ");
    }
    
    Serial.print(" Accel: ");
    for (int i = 5; i < 8; i++) {
        Serial.print(features[i]);
        Serial.print(" ");
    }
    
    Serial.print(" Gyro: ");
    for (int i = 8; i < 11; i++) {
        Serial.print(features[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    delay(20);  // 50Hz采样
}
```
