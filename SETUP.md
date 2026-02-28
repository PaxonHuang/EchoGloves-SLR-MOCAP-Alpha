# Smart Glove ASL Translation - 项目设置指南

## 🚀 快速开始

本项目是一个智能手套系统，用于识别美国手语(ASL)并转换为文本/语音。

### 📋 系统要求

- **Python**: 3.8, 3.9, 或 3.10 (推荐 **3.9**)
- **ESP32**: ESP32-S3-DevKitC-1 (推荐) 或 ESP32 DevKit
- **传感器**: 5x 弯曲传感器 + MPU6050 IMU

---

## 📦 配置文件

项目已包含以下配置文件：

| 文件 | 位置 | 用途 |
|------|------|------|
| `requirements.txt` | `Smart-glove-main/requirements.txt` | Python依赖包 |
| `platformio.ini` | `Smart-glove-main/platformio.ini` | PlatformIO编译配置 |
| `INSTALLATION_GUIDE.md` | `Smart-glove-main/INSTALLATION_GUIDE.md` | 详细安装指南 |

---

## ⚡ 5分钟快速设置

### 1. Python环境设置

```bash
# 进入项目目录
cd Smart-glove-main

# 创建虚拟环境
python -m venv smartglove_env

# 激活环境 (Windows)
smartglove_env\Scripts\activate

# 激活环境 (Linux/Mac)
source smartglove_env/bin/activate

# 安装依赖
pip install -r requirements.txt
```

### 2. Arduino IDE设置

1. **添加ESP32开发板支持**:
   - 打开 `文件` > `首选项`
   - 添加开发板管理器URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

2. **安装开发板**:
   - `工具` > `开发板` > `开发板管理器`
   - 搜索 "ESP32" 并安装 (版本 >= 2.0.0)

3. **安装库**:
   - `项目` > `加载库` > `管理库`
   - 安装:
     - `Adafruit MPU6050` (^2.2.0)
     - `Adafruit Unified Sensor` (^1.1.6)
   - 手动安装 `FlexLibrary` (下载后放入 `Documents/Arduino/libraries/`)

### 3. 硬件连接

```
ESP32-S3引脚分配:
├── GPIO 36 → 弯曲传感器1 (拇指)
├── GPIO 39 → 弯曲传感器2 (食指)
├── GPIO 34 → 弯曲传感器3 (中指)
├── GPIO 35 → 弯曲传感器4 (无名指)
├── GPIO 32 → 弯曲传感器5 (小指)
├── GPIO 21 → MPU6050 SDA
├── GPIO 22 → MPU6050 SCL
└── 3.3V/GND → 传感器供电
```

---

## 📖 详细文档

完整的安装指南、故障排除和API文档请查看：

📄 **[Smart-glove-main/INSTALLATION_GUIDE.md](Smart-glove-main/INSTALLATION_GUIDE.md)**

包含内容：
- Python 3.9 完整安装步骤
- Arduino IDE / PlatformIO 配置
- 硬件电路图和接线说明
- 4种使用场景教程
- 常见问题排查

---

## 🎯 使用场景

### 场景1: 训练模型
```bash
# 确保CSV数据在 modified dataset/alphabet/ 中
python RF.py
```

### 场景2: WiFi实时推理
```bash
# 1. 上传 success.ino 到ESP32
# 2. 运行接收程序
python "model deployment.py"
```

### 场景3: ESP32独立推理
```bash
# 1. 将生成的 classifier.h 复制到 Random Forest on esp32/
# 2. 上传 rf.ino
```

---

## 📦 依赖版本

### Python包
```
numpy==1.21.6
pandas==1.3.5
scikit-learn==1.0.2
joblib==1.1.0
matplotlib==3.5.3
seaborn==0.11.2
pyttsx3==2.90
```

### Arduino库
```
ESP32 Arduino Core >= 2.0.0
Adafruit MPU6050 ^2.2.0
Adafruit Unified Sensor ^1.1.6
FlexLibrary (手动安装)
```

---

## 🔧 使用PlatformIO (可选)

```bash
cd Smart-glove-main

# 编译
pio run

# 上传
pio run --target upload

# 监视串口
pio device monitor
```

---

## 🆘 需要帮助?

查看详细指南: [INSTALLATION_GUIDE.md](Smart-glove-main/INSTALLATION_GUIDE.md)

常见问题：
- **模块导入错误**: 确保虚拟环境已激活
- **ESP32未识别**: 检查USB驱动和端口选择
- **传感器无读数**: 检查分压电路和接线

---

## 📄 项目结构

```
Smart-glove-main/
├── RF.py                      # 训练脚本
├── requirements.txt           # Python依赖 ⭐
├── platformio.ini            # PlatformIO配置 ⭐
├── INSTALLATION_GUIDE.md     # 详细指南 ⭐
├── success.ino               # WiFi固件
├── model deployment.py       # 接收程序
├── Random Forest on esp32/
│   ├── rf.ino               # 独立推理固件
│   ├── Datasetcollection.ino # 数据采集
│   └── classifier.h         # 模型文件
├── smartglovepcb/           # PCB设计文件
└── modified dataset/        # 训练数据
```

---

## 📜 许可证

MIT License

---

**最后更新**: 2026-02-28
