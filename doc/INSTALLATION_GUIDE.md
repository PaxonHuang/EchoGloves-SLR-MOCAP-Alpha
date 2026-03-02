# Smart Glove ASL Translation - 安装配置指南
# Smart Glove ASL Translation - Installation & Configuration Guide

## 目录 (Table of Contents)
1. [项目概述 (Project Overview)](#项目概述)
2. [Python环境配置 (Python Environment)](#python环境配置)
3. [Arduino/ESP32开发环境 (Arduino/ESP32 Development Environment)](#arduinoesp32开发环境)
4. [硬件连接 (Hardware Connections)](#硬件连接)
5. [快速开始 (Quick Start)](#快速开始)
6. [故障排除 (Troubleshooting)](#故障排除)

---

## 项目概述

本项目是一个智能手套系统，用于识别美国手语(ASL)并将其转换为文本/语音。系统包括：

- **硬件**: ESP32-S3, 5个弯曲传感器, MPU6050 IMU传感器
- **机器学习**: Random Forest分类器
- **部署选项**: 
  - 方案1: Raspberry Pi 4 + ESP32 (WiFi通信)
  - 方案2: ESP32独立运行 (直接推理)

### 系统要求

| 组件 | 最低要求 | 推荐配置 |
|------|---------|---------|
| Python | 3.8 | **3.9** |
| ESP32核心 | 2.0.0 | **2.0.14+** |
| 操作系统 | Windows 10 / Ubuntu 20.04 | Windows 11 / Ubuntu 22.04 |

---

## Python环境配置

### 步骤1: 安装Python

**Windows:**
```powershell
# 从 https://www.python.org/downloads/release/python-3913/ 下载Python 3.9.13
# 安装时勾选 "Add Python to PATH"
```

**Linux/macOS:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install python3.9 python3.9-venv python3.9-dev

# macOS (使用 Homebrew)
brew install python@3.9
```

### 步骤2: 创建虚拟环境

```bash
# 进入项目目录
cd Smart-glove-main

# 创建虚拟环境
python -m venv smartglove_env

# 激活虚拟环境 (Windows)
smartglove_env\Scripts\activate

# 激活虚拟环境 (Linux/macOS)
source smartglove_env/bin/activate
```

### 步骤3: 安装Python依赖

```bash
# 安装核心依赖
pip install -r requirements.txt
```

或者手动安装：

```bash
# 核心ML依赖
pip install numpy==1.21.6 pandas==1.3.5 scikit-learn==1.0.2 joblib==1.1.0

# 可视化依赖
pip install matplotlib==3.5.3 seaborn==0.11.2

# 部署依赖
pip install pyttsx3==2.90
```

### 步骤4: 验证安装

```bash
# 验证Python版本
python --version  # 应显示 Python 3.9.x

# 验证核心库
python -c "import numpy; print(f'numpy: {numpy.__version__}')"
python -c "import pandas; print(f'pandas: {pandas.__version__}')"
python -c "import sklearn; print(f'scikit-learn: {sklearn.__version__}')"

# 运行训练脚本测试
python RF.py
```

---

## Arduino/ESP32开发环境

### 方案A: Arduino IDE (推荐初学者)

#### 1. 安装Arduino IDE

从 [Arduino官网](https://www.arduino.cc/en/software) 下载并安装 Arduino IDE 1.8.x 或 2.0+

#### 2. 添加ESP32开发板支持

**步骤:**
1. 打开 Arduino IDE
2. 进入 `文件(File)` > `首选项(Preferences)`
3. 在"附加开发板管理器网址"中添加：
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. 点击 `确定(OK)`

#### 3. 安装ESP32开发板

1. 进入 `工具(Tools)` > `开发板(Board)` > `开发板管理器(Board Manager)`
2. 搜索 "ESP32"
3. 安装 "ESP32 by Espressif Systems" (版本 >= 2.0.0)
4. **推荐版本**: 2.0.14 或更高

#### 4. 安装必要的库

**通过库管理器安装:**

1. 进入 `项目(Project)` > `加载库(Load Library)` > `管理库(Manage Libraries)`
2. 搜索并安装以下库：

| 库名 | 版本 | 用途 |
|------|------|------|
| Adafruit MPU6050 | ^2.2.0 | MPU6050传感器驱动 |
| Adafruit Unified Sensor | ^1.1.6 | 传感器抽象层 |
| Adafruit BusIO | ^1.14.0 | I2C通信工具 |

**手动安装FlexLibrary:**

FlexLibrary不在Arduino库管理器中，需要手动安装：

```bash
# 1. 下载FlexLibrary
# 访问: https://github.com/... (请根据实际项目地址更新)

# 2. 解压并复制到Arduino库文件夹
# Windows: Documents\Arduino\libraries\
# Linux: ~/Arduino/libraries/
# macOS: ~/Documents/Arduino/libraries/
```

#### 5. 配置开发板

1. 选择开发板: `工具(Tools)` > `开发板(Board)` > `ESP32 Arduino` > `ESP32S3 Dev Module`
2. 选择端口: `工具(Tools)` > `端口(Port)` > 选择ESP32连接的COM端口
3. 其他设置:
   - CPU Frequency: 240MHz (WiFi/BT)
   - Flash Mode: DIO
   - Flash Frequency: 80MHz
   - Flash Size: 8MB
   - Partition Scheme: Default 8MB with spiffs

### 方案B: PlatformIO (推荐高级用户)

#### 1. 安装PlatformIO

**VS Code扩展:**
1. 打开VS Code
2. 进入扩展(Extensions)市场
3. 搜索 "PlatformIO IDE"
4. 点击安装

**命令行安装:**
```bash
pip install platformio
```

#### 2. 使用PlatformIO打开项目

```bash
# 进入项目目录
cd Smart-glove-main

# 使用PlatformIO编译
pio run

# 上传到ESP32
pio run --target upload

# 打开串口监视器
pio device monitor
```

#### 3. PlatformIO配置说明

项目已包含 `platformio.ini` 配置文件：

```ini
[env:esp32-s3]
platform = espressif32@6.3.0
board = esp32-s3-devkitc-1
framework = arduino

lib_deps =
    adafruit/Adafruit MPU6050 @ ^2.2.0
    adafruit/Adafruit Unified Sensor @ ^1.1.6
    adafruit/Adafruit BusIO @ ^1.14.0
```

**注意**: FlexLibrary需要手动下载并放入 `lib/` 文件夹

---

## 硬件连接

### 组件清单

| 组件 | 数量 | 规格 | 备注 |
|------|------|------|------|
| ESP32-S3 DevKitC-1 | 1 | ESP32-S3-WROOM-1 | 主控板 |
| 弯曲传感器 | 5 | 电阻式 | 测量手指弯曲 |
| MPU6050 | 1 | I2C接口 | 6轴IMU传感器 |
| 电阻 | 5 | 10kΩ | 与弯曲传感器组成分压电路 |
| 面包板 | 1 | 标准尺寸 | 原型搭建 |
| 杜邦线 | 若干 | 公对母/公对公 | 连接线 |

### 电路连接图

```
ESP32-S3 DevKitC-1 引脚分配:

弯曲传感器连接:
├── 传感器1 (拇指)  → GPIO 36 (VP)
├── 传感器2 (食指)  → GPIO 39 (VN)
├── 传感器3 (中指)  → GPIO 34
├── 传感器4 (无名指) → GPIO 35
└── 传感器5 (小指)  → GPIO 32

MPU6050 I2C连接:
├── VCC  → 3.3V
├── GND  → GND
├── SCL  → GPIO 22
├── SDA  → GPIO 21
└── XDA/XCL/ADO/INT → 悬空

电源:
├── 3.3V → 传感器VCC (通过分压电路)
├── 5V   → 弯曲传感器供电
└── GND  → 共地
```

### 弯曲传感器电路

每个弯曲传感器使用分压电路：

```
5V ---- [弯曲传感器] ----+---- GPIO (模拟输入)
                         |
                        [10kΩ电阻]
                         |
                        GND
```

**传感器规格:**
- 平直电阻: ~25-35kΩ
- 弯曲电阻: ~70-100kΩ
- 供电电压: 5V

### MPU6050配置

代码中的默认配置 (`success.ino`):

```cpp
// 加速度计量程: ±8G
mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

// 陀螺仪量程: ±500度/秒
mpu.setGyroRange(MPU6050_RANGE_500_DEG);

// 数字低通滤波器: 21Hz
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
```

---

## 快速开始

### 场景1: 数据采集 (用于训练)

1. **上传数据收集固件:**
   ```
   文件: Random Forest on esp32/Datasetcollection.ino
   ```

2. **打开串口监视器** (波特率: 115200)

3. **执行手势并记录数据**

4. **保存CSV文件**到 `modified dataset/alphabet/` 目录

### 场景2: 模型训练

1. **准备数据:**
   确保CSV文件在 `modified dataset/alphabet/` 中

2. **运行训练脚本:**
   ```bash
   python RF.py
   ```

3. **查看输出:**
   - 模型准确率
   - 混淆矩阵
   - 生成的 `classifier.h` (用于ESP32部署)

### 场景3: 实时推理 (WiFi方案)

1. **配置WiFi** (修改 `success.ino`):
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. **上传固件到ESP32:**
   ```
   文件: success.ino
   ```

3. **运行接收程序:**
   ```bash
   python model_deployment.py
   ```

4. **查看预测结果**

### 场景4: 独立运行 (ESP32直接推理)

1. **生成模型头文件:**
   运行 `RF.py` 后会生成 `classifier.h`

2. **复制模型文件:**
   将生成的 `classifier.h` 复制到 `Random Forest on esp32/`

3. **上传推理固件:**
   ```
   文件: Random Forest on esp32/rf.ino
   ```

4. **打开串口监视器查看预测**

---

## 故障排除

### Python环境问题

**问题: `ModuleNotFoundError: No module named 'sklearn'`**
```bash
# 解决: 确保虚拟环境已激活
smartglove_env\Scripts\activate  # Windows
source smartglove_env/bin/activate  # Linux/Mac

# 重新安装依赖
pip install -r requirements.txt
```

**问题: `ImportError: cannot import name 'RandomForestClassifier'`**
```bash
# 解决: 升级scikit-learn
pip install --upgrade scikit-learn==1.0.2
```

**问题: TensorFlow安装失败**
```bash
# 解决: 使用conda安装 (推荐)
conda install tensorflow==2.10.0

# 或者使用pip指定版本
pip install tensorflow-cpu==2.10.0
```

### ESP32/Arduino问题

**问题: 无法找到ESP32开发板**
- 检查开发板管理器URL是否正确添加
- 确认已安装ESP32支持包
- 尝试重新启动Arduino IDE

**问题: 编译错误 `'FlexLibrary.h' not found`**
- FlexLibrary需要手动安装
- 下载并解压到Arduino库文件夹
- 确认文件夹名为 `FlexLibrary`

**问题: MPU6050初始化失败**
```cpp
// 检查I2C地址
if (!mpu.begin(0x68)) {  // 默认地址
// 或尝试
if (!mpu.begin(0x69)) {  // 如果AD0引脚接VCC
```

**问题: WiFi连接超时**
- 检查WiFi SSID和密码
- 确认ESP32支持2.4GHz网络 (不支持5GHz)
- 检查路由器设置

### 硬件问题

**问题: 传感器读数不稳定**
- 检查电源稳定性 (使用外部电源或电容滤波)
- 确认分压电阻值正确 (10kΩ)
- 检查接线是否牢固

**问题: 弯曲传感器没有变化**
- 确认传感器方向 (有些传感器单向敏感)
- 检查分压电路连接
- 用万用表测量电阻变化

### 模型问题

**问题: 预测准确率低**
- 检查训练数据质量
- 增加训练样本数量
- 尝试调整模型超参数
- 检查特征标准化是否正常

**问题: ESP32推理结果与Python不一致**
- 确认使用了相同的特征缩放参数
- 检查 `scaler.joblib` 是否正确加载
- 验证输入数据的数值范围

---

## 版本历史

| 日期 | 版本 | 说明 |
|------|------|------|
| 2026-02-28 | 1.0.0 | 初始版本，基于项目分析创建 |

## 参考资料

- [ESP32 Arduino Core文档](https://docs.espressif.com/projects/arduino-esp32/)
- [Adafruit MPU6050指南](https://learn.adafruit.com/mpu6050-6-dof-accelerometer-and-gyro)
- [scikit-learn文档](https://scikit-learn.org/stable/)
- [PlatformIO文档](https://docs.platformio.org/)

## 贡献与支持

如有问题或建议，请通过以下方式联系：
- GitHub Issues: [项目仓库]
- 邮件: [联系邮箱]

---

**版权所有 © 2026 Smart Glove Project**
**开源协议: MIT License**
