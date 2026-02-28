# ESP32 Embedded Development Skill
# ESP32嵌入式开发专家

## 概述
本Skill专注于ESP32系列(ESP32/ESP32-S3/ESP32-C3)的嵌入式开发，涵盖传感器驱动、通信协议、低功耗管理和FreeRTOS任务调度。

## 核心能力

### 1. 硬件驱动开发
- **GPIO管理**: 数字/模拟输入输出
- **I2C/SPI通信**: 传感器接口
- **WiFi/BLE**: 无线通信
- **OTA更新**: 空中固件升级

### 2. 系统架构
- **FreeRTOS**: 多任务管理
- **中断处理**: 高效事件响应
- **内存管理**: PSRAM/DRAM优化
- **电源管理**: 低功耗模式

### 3. 开发工具
- **PlatformIO**: 专业开发环境
- **Arduino框架**: 快速原型
- **ESP-IDF**: 高级功能
- **调试工具**: JTAG/GDB

## 项目专用配置

### PlatformIO配置 (platformio.ini)
```ini
; ESP32开发环境配置
[platformio]
default_envs = esp32dev

[env:esp32dev]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600

; 库依赖
lib_deps = 
    adafruit/Adafruit MPU6050 @ ^2.2.0
    adafruit/Adafruit Sensor @ ^1.1.6
    adafruit/Adafruit BusIO @ ^1.14.0
    Wire
    SPI

; 编译选项
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ARDUHAL_LOG_COLORS=1

; 分区表(用于更大程序)
board_build.partitions = default_16MB.csv

; ESP32-S3配置(升级推荐)
[env:esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600

lib_deps = 
    adafruit/Adafruit MPU6050 @ ^2.2.0
    adafruit/Adafruit Sensor @ ^1.1.6
    adafruit/Adafruit BusIO @ ^1.14.0

build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ARDUHAL_LOG_COLORS=1
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue

; 使用16MB Flash和8MB PSRAM
board_build.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_qspi
```

## 传感器驱动

### MPU6050高级驱动
```cpp
// mpu6050_advanced.h
#pragma once
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

struct IMUData {
    float accel[3];      // m/s^2
    float gyro[3];       // rad/s
    float temp;          // Celsius
    uint32_t timestamp;  // ms
};

class MPU6050Advanced {
public:
    MPU6050Advanced(uint8_t addr = 0x68) : addr_(addr), initialized_(false) {}
    
    bool begin() {
        Wire.begin();
        
        if (!mpu_.begin(addr_, &Wire)) {
            Serial.println("MPU6050初始化失败!");
            return false;
        }
        
        // 配置传感器参数
        mpu_.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu_.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu_.setFilterBandwidth(MPU6050_BAND_21_HZ);
        
        initialized_ = true;
        Serial.println("MPU6050初始化成功!");
        return true;
    }
    
    void calibrate(uint16_t samples = 1000) {
        if (!initialized_) return;
        
        Serial.println("开始校准MPU6050...");
        
        float accel_sum[3] = {0};
        float gyro_sum[3] = {0};
        
        for (uint16_t i = 0; i < samples; i++) {
            sensors_event_t a, g, temp;
            mpu_.getEvent(&a, &g, &temp);
            
            accel_sum[0] += a.acceleration.x;
            accel_sum[1] += a.acceleration.y;
            accel_sum[2] += a.acceleration.z;
            
            gyro_sum[0] += g.gyro.x;
            gyro_sum[1] += g.gyro.y;
            gyro_sum[2] += g.gyro.z;
            
            delay(1);
        }
        
        // 计算偏移
        accel_offset_[0] = accel_sum[0] / samples;
        accel_offset_[1] = accel_sum[1] / samples;
        accel_offset_[2] = (accel_sum[2] / samples) - 9.81;  // Z轴减去重力
        
        gyro_offset_[0] = gyro_sum[0] / samples;
        gyro_offset_[1] = gyro_sum[1] / samples;
        gyro_offset_[2] = gyro_sum[2] / samples;
        
        Serial.println("校准完成!");
        Serial.printf("Accel offset: %.3f, %.3f, %.3f\n", 
                      accel_offset_[0], accel_offset_[1], accel_offset_[2]);
        Serial.printf("Gyro offset: %.3f, %.3f, %.3f\n",
                      gyro_offset_[0], gyro_offset_[1], gyro_offset_[2]);
    }
    
    IMUData read() {
        IMUData data;
        sensors_event_t a, g, temp;
        
        mpu_.getEvent(&a, &g, &temp);
        
        // 应用偏移校正
        data.accel[0] = a.acceleration.x - accel_offset_[0];
        data.accel[1] = a.acceleration.y - accel_offset_[1];
        data.accel[2] = a.acceleration.z - accel_offset_[2];
        
        data.gyro[0] = g.gyro.x - gyro_offset_[0];
        data.gyro[1] = g.gyro.y - gyro_offset_[1];
        data.gyro[2] = g.gyro.z - gyro_offset_[2];
        
        data.temp = temp.temperature;
        data.timestamp = millis();
        
        return data;
    }
    
    void printConfig() {
        Serial.println("MPU6050配置:");
        
        Serial.print("加速度计量程: ");
        switch (mpu_.getAccelerometerRange()) {
            case MPU6050_RANGE_2_G: Serial.println("+-2G"); break;
            case MPU6050_RANGE_4_G: Serial.println("+-4G"); break;
            case MPU6050_RANGE_8_G: Serial.println("+-8G"); break;
            case MPU6050_RANGE_16_G: Serial.println("+-16G"); break;
        }
        
        Serial.print("陀螺仪量程: ");
        switch (mpu_.getGyroRange()) {
            case MPU6050_RANGE_250_DEG: Serial.println("+-250 deg/s"); break;
            case MPU6050_RANGE_500_DEG: Serial.println("+-500 deg/s"); break;
            case MPU6050_RANGE_1000_DEG: Serial.println("+-1000 deg/s"); break;
            case MPU6050_RANGE_2000_DEG: Serial.println("+-2000 deg/s"); break;
        }
        
        Serial.print("滤波带宽: ");
        switch (mpu_.getFilterBandwidth()) {
            case MPU6050_BAND_260_HZ: Serial.println("260 Hz"); break;
            case MPU6050_BAND_184_HZ: Serial.println("184 Hz"); break;
            case MPU6050_BAND_94_HZ: Serial.println("94 Hz"); break;
            case MPU6050_BAND_44_HZ: Serial.println("44 Hz"); break;
            case MPU6050_BAND_21_HZ: Serial.println("21 Hz"); break;
            case MPU6050_BAND_10_HZ: Serial.println("10 Hz"); break;
            case MPU6050_BAND_5_HZ: Serial.println("5 Hz"); break;
        }
    }

private:
    Adafruit_MPU6050 mpu_;
    uint8_t addr_;
    bool initialized_;
    float accel_offset_[3] = {0};
    float gyro_offset_[3] = {0};
};
```

## FreeRTOS多任务架构

### 任务管理
```cpp
// freertos_tasks.h
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// 任务优先级
#define SENSOR_TASK_PRIORITY    5
#define PROCESSING_TASK_PRIORITY 4
#define COMM_TASK_PRIORITY      3
#define DISPLAY_TASK_PRIORITY   2

// 堆栈大小(字节)
#define SENSOR_STACK_SIZE       4096
#define PROCESSING_STACK_SIZE   8192
#define COMM_STACK_SIZE         4096

// 队列大小
#define SENSOR_QUEUE_SIZE       10
#define RESULT_QUEUE_SIZE       5

struct SensorPacket {
    float flex[5];
    float accel[3];
    float gyro[3];
    uint32_t timestamp;
};

struct ResultPacket {
    int gesture_class;
    float confidence;
    uint32_t timestamp;
};

class TaskManager {
public:
    static TaskManager& getInstance() {
        static TaskManager instance;
        return instance;
    }
    
    void begin() {
        // 创建队列
        sensorQueue_ = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(SensorPacket));
        resultQueue_ = xQueueCreate(RESULT_QUEUE_SIZE, sizeof(ResultPacket));
        
        // 创建互斥锁
        i2cMutex_ = xSemaphoreCreateMutex();
        
        // 创建任务
        xTaskCreatePinnedToCore(
            sensorTask,
            "SensorTask",
            SENSOR_STACK_SIZE,
            this,
            SENSOR_TASK_PRIORITY,
            &sensorTaskHandle_,
            1  // 在Core 1上运行
        );
        
        xTaskCreatePinnedToCore(
            processingTask,
            "ProcessingTask",
            PROCESSING_STACK_SIZE,
            this,
            PROCESSING_TASK_PRIORITY,
            &processingTaskHandle_,
            0  // 在Core 0上运行
        );
        
        xTaskCreatePinnedToCore(
            communicationTask,
            "CommTask",
            COMM_STACK_SIZE,
            this,
            COMM_TASK_PRIORITY,
            &commTaskHandle_,
            1
        );
    }
    
    QueueHandle_t getSensorQueue() { return sensorQueue_; }
    QueueHandle_t getResultQueue() { return resultQueue_; }
    SemaphoreHandle_t getI2CMutex() { return i2cMutex_; }

private:
    TaskManager() {}
    
    static void sensorTask(void* parameter) {
        TaskManager* tm = (TaskManager*)parameter;
        SensorPacket packet;
        
        TickType_t lastWakeTime = xTaskGetTickCount();
        const TickType_t frequency = pdMS_TO_TICKS(20);  // 50Hz
        
        while (true) {
            // 读取传感器数据
            // 这里添加实际的传感器读取代码
            packet.timestamp = millis();
            
            // 发送到处理队列
            xQueueSend(tm->sensorQueue_, &packet, portMAX_DELAY);
            
            vTaskDelayUntil(&lastWakeTime, frequency);
        }
    }
    
    static void processingTask(void* parameter) {
        TaskManager* tm = (TaskManager*)parameter;
        SensorPacket sensorData;
        ResultPacket result;
        
        while (true) {
            // 等待传感器数据
            if (xQueueReceive(tm->sensorQueue_, &sensorData, portMAX_DELAY) == pdTRUE) {
                // 执行推理
                // 这里添加模型推理代码
                result.gesture_class = 0;  // 示例
                result.confidence = 0.95;
                result.timestamp = sensorData.timestamp;
                
                // 发送结果
                xQueueSend(tm->resultQueue_, &result, portMAX_DELAY);
            }
        }
    }
    
    static void communicationTask(void* parameter) {
        TaskManager* tm = (TaskManager*)parameter;
        ResultPacket result;
        
        while (true) {
            if (xQueueReceive(tm->resultQueue_, &result, portMAX_DELAY) == pdTRUE) {
                // 发送结果到串口/蓝牙/WiFi
                Serial.printf("Gesture: %d, Confidence: %.2f\n",
                            result.gesture_class, result.confidence);
            }
        }
    }
    
    QueueHandle_t sensorQueue_;
    QueueHandle_t resultQueue_;
    SemaphoreHandle_t i2cMutex_;
    
    TaskHandle_t sensorTaskHandle_;
    TaskHandle_t processingTaskHandle_;
    TaskHandle_t commTaskHandle_;
};
```

## WiFi通信

### WiFi管理器
```cpp
// wifi_manager.h
#pragma once
#include <WiFi.h>
#include <WiFiManager.h>

class WiFiManagerAdvanced {
public:
    WiFiManagerAdvanced() : connected_(false) {}
    
    void begin(const char* apName = "SmartGlove-Setup") {
        WiFi.mode(WIFI_STA);
        
        // 尝试连接已保存的WiFi
        WiFi.begin();
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi已连接!");
            Serial.print("IP地址: ");
            Serial.println(WiFi.localIP());
            connected_ = true;
        } else {
            // 启动配置AP
            Serial.println("\n启动配置模式...");
            startConfigPortal(apName);
        }
    }
    
    void startConfigPortal(const char* apName) {
        WiFiManager wm;
        
        // 自定义参数
        WiFiManagerParameter custom_text("<p>智能手套WiFi配置</p>");
        wm.addParameter(&custom_text);
        
        wm.setConfigPortalTimeout(180);
        wm.setAPCallback([](WiFiManager* wifiManager) {
            Serial.println("进入配置模式");
            Serial.println(WiFi.softAPIP());
        });
        
        if (!wm.startConfigPortal(apName)) {
            Serial.println("配置失败，重启...");
            ESP.restart();
        }
        
        Serial.println("WiFi配置成功!");
        connected_ = true;
    }
    
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }
    
    String getIP() {
        return WiFi.localIP().toString();
    }
    
    void disconnect() {
        WiFi.disconnect();
        connected_ = false;
    }

private:
    bool connected_;
};
```

## OTA固件更新

### OTA管理器
```cpp
// ota_manager.h
#pragma once
#include <ArduinoOTA.h>
#include <WiFi.h>

class OTAManager {
public:
    void begin(const char* hostname = "smartglove") {
        ArduinoOTA.setHostname(hostname);
        
        ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else {
                type = "filesystem";
            }
            Serial.println("开始OTA更新: " + type);
        });
        
        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA更新完成");
        });
        
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("进度: %u%%\r", (progress / (total / 100)));
        });
        
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("错误[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("认证失败");
            else if (error == OTA_BEGIN_ERROR) Serial.println("开始失败");
            else if (error == OTA_CONNECT_ERROR) Serial.println("连接失败");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("接收失败");
            else if (error == OTA_END_ERROR) Serial.println("结束失败");
        });
        
        ArduinoOTA.begin();
        Serial.println("OTA已就绪");
    }
    
    void handle() {
        ArduinoOTA.handle();
    }
};
```

## 低功耗管理

### 电源管理
```cpp
// power_manager.h
#pragma once
#include <esp_sleep.h>
#include <esp32/ulp.h>

class PowerManager {
public:
    enum SleepMode {
        LIGHT_SLEEP,
        DEEP_SLEEP,
        HIBERNATE
    };
    
    void begin() {
        // 禁用蓝牙和WiFi以节省电量
        btStop();
    }
    
    void lightSleep(uint32_t sleepTimeMs) {
        esp_sleep_enable_timer_wakeup(sleepTimeMs * 1000);
        esp_light_sleep_start();
    }
    
    void deepSleep(uint32_t sleepTimeMs) {
        esp_sleep_enable_timer_wakeup(sleepTimeMs * 1000);
        esp_deep_sleep_start();
    }
    
    void enableWakeupOnGPIO(gpio_num_t gpio, int level) {
        esp_sleep_enable_ext0_wakeup(gpio, level);
    }
    
    void printWakeupReason() {
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        
        switch(wakeup_reason) {
            case ESP_SLEEP_WAKEUP_EXT0:
                Serial.println("由外部信号EXT0唤醒");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                Serial.println("由外部信号EXT1唤醒");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                Serial.println("由定时器唤醒");
                break;
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                Serial.println("由触摸传感器唤醒");
                break;
            default:
                Serial.printf("唤醒原因: %d\n", wakeup_reason);
                break;
        }
    }
    
    // 设置CPU频率
    void setCPUFrequency(uint32_t freq_mhz) {
        setCpuFrequencyMhz(freq_mhz);
    }
};
```

## 完整项目模板

### main.cpp
```cpp
#include <Arduino.h>
#include "mpu6050_advanced.h"
#include "freertos_tasks.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include "power_manager.h"

// 全局对象
MPU6050Advanced imu;
WiFiManagerAdvanced wifi;
OTAManager ota;
PowerManager power;

// 传感器引脚
const int FLEX_PINS[5] = {36, 39, 34, 35, 32};

void setup() {
    Serial.begin(115200);
    Serial.println("\n智能手套启动中...");
    
    // 初始化电源管理
    power.begin();
    power.printWakeupReason();
    
    // 初始化WiFi
    wifi.begin("SmartGlove");
    
    // 初始化OTA
    ota.begin();
    
    // 初始化IMU
    if (!imu.begin()) {
        Serial.println("MPU6050初始化失败!");
    } else {
        imu.calibrate(500);
        imu.printConfig();
    }
    
    // 启动FreeRTOS任务
    TaskManager::getInstance().begin();
    
    Serial.println("系统初始化完成!");
}

void loop() {
    // 处理OTA
    ota.handle();
    
    // 主循环可以执行其他低优先级任务
    delay(10);
}
```

## 调试技巧

### 内存监控
```cpp
void printMemoryInfo() {
    Serial.printf("=== 内存信息 ===\n");
    Serial.printf("堆总大小: %d bytes\n", ESP.getHeapSize());
    Serial.printf("空闲堆: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("最小空闲堆: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("最大分配块: %d bytes\n", ESP.getMaxAllocHeap());
    
    if (psramFound()) {
        Serial.printf("PSRAM总大小: %d bytes\n", ESP.getPsramSize());
        Serial.printf("空闲PSRAM: %d bytes\n", ESP.getFreePsram());
    }
}
```

### CPU使用率
```cpp
void printTaskInfo() {
    TaskStatus_t* taskStats;
    volatile UBaseType_t uxArraySize, x;
    unsigned long ulTotalRunTime;
    
    uxArraySize = uxTaskGetNumberOfTasks();
    taskStats = (TaskStatus_t*)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (taskStats != NULL) {
        uxArraySize = uxTaskGetSystemState(taskStats, uxArraySize, &ulTotalRunTime);
        
        Serial.printf("%-20s %-10s %-10s %-10s\n", "任务", "状态", "优先级", "堆栈剩余");
        for (x = 0; x < uxArraySize; x++) {
            Serial.printf("%-20s %-10d %-10d %-10d\n",
                        taskStats[x].pcTaskName,
                        taskStats[x].eCurrentState,
                        taskStats[x].uxCurrentPriority,
                        taskStats[x].usStackHighWaterMark);
        }
        
        vPortFree(taskStats);
    }
}
```
