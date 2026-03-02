# Smart Glove MCP 完整配置方案
# 方案B: 高级用户配置

## 安装完成总结

### 配置文件位置
```
Smart-glove-main/
├── .mcp/
│   └── mcp.json                    # MCP服务器配置
├── .skills/
│   ├── tinyml-tensorflow.md        # TinyML/TensorFlow Lite技能
│   ├── signal-processing.md        # 信号处理技能
│   └── embedded-esp32.md           # ESP32嵌入式开发技能
└── MCP_INSTALLATION_GUIDE.md       # 本文件
```

## 已安装的MCP Servers

### 1. PlatformIO MCP ⭐⭐⭐⭐⭐
**用途**: 嵌入式开发平台管理
**功能**:
- 编译ESP32/Arduino项目
- 库依赖管理(Adafruit_MPU6050等)
- 固件上传和烧录
- 串口监视器

**可用工具**:
- `pio_compile` - 编译项目
- `pio_upload` - 上传固件
- `pio_monitor` - 串口监控
- `pio_lib_install` - 安装库
- `pio_init` - 初始化项目

### 2. Arduino MCP ⭐⭐⭐⭐
**用途**: Arduino CLI集成
**功能**:
- Arduino sketch编译
- 板子管理(FQBN配置)
- 库安装

**可用工具**:
- `arduino_compile` - 编译sketch
- `arduino_upload` - 上传sketch
- `arduino_lib_install` - 安装库
- `arduino_board_list` - 列出板子

### 3. KiCad MCP ⭐⭐⭐⭐
**用途**: AI辅助PCB设计
**功能**:
- 原理图分析
- PCB布局优化
- Gerber导出
- 52个专业设计工具

**可用工具**:
- `kicad_open_project` - 打开项目
- `kicad_analyze_schematic` - 分析原理图
- `kicad_optimize_layout` - 优化布局
- `kicad_export_gerber` - 导出Gerber

### 4. ESP32 MCP ⭐⭐⭐⭐⭐
**用途**: ESP32专用开发
**功能**:
- WiFi配置管理
- OTA固件更新
- 传感器监控
- 固件刷写

**可用工具**:
- `esp32_configure_wifi` - WiFi配置
- `esp32_ota_update` - OTA更新
- `esp32_monitor_sensors` - 传感器监控
- `esp32_flash_firmware` - 刷写固件

## 已配置Skills

### 1. TinyML/TensorFlow Lite Skill
**文件**: `.skills/tinyml-tensorflow.md`

**核心内容**:
- Conv1D + Bi-LSTM架构模板
- TensorFlow Lite模型转换脚本
- ESP32 TFLite Micro代码模板
- INT8量化优化指南
- 数据预处理管道

**关键特性**:
- 针对ASL手势识别优化的神经网络架构
- 自动生成C++头文件的工具脚本
- 50KB Tensor Arena内存管理
- 目标模型大小<300KB

### 2. Signal Processing Skill
**文件**: `.skills/signal-processing.md`

**核心内容**:
- 卡尔曼滤波器实现(C++)
- 互补滤波器(AHRS姿态解算)
- 四元数姿态估计
- 弯曲传感器校准和线性化
- 特征提取工具(时域/频域)

**关键特性**:
- MPU6050专用滤波算法
- 5通道弯曲传感器处理
- 实时数据归一化
- 滑动窗口实现

### 3. ESP32 Embedded Development Skill
**文件**: `.skills/embedded-esp32.md`

**核心内容**:
- PlatformIO配置模板(platformio.ini)
- MPU6050高级驱动
- FreeRTOS多任务架构
- WiFi管理器和OTA更新
- 电源管理和低功耗模式

**关键特性**:
- 双核任务分配策略
- 队列和互斥锁管理
- WiFi配置门户
- 自动OTA更新支持

## 使用指南

### 1. 配置MCP客户端

在你的AI助手配置文件中添加MCP服务器:

```json
{
  "mcpServers": {
    "platformio": {
      "command": "node",
      "args": ["path/to/platformio-mcp/dist/index.js"]
    },
    "kicad": {
      "command": "node",
      "args": ["path/to/kicad-mcp/dist/index.js"],
      "env": {
        "PYTHONPATH": "/path/to/kicad/python"
      }
    },
    "esp32": {
      "command": "python",
      "args": ["path/to/esp32-mcp/server.py"]
    }
  }
}
```

### 2. 加载Skills

将`.skills/`目录添加到你的AI助手skill路径中。

### 3. 项目升级流程

#### Phase 1: 数据采集改进
```bash
# 使用PlatformIO MCP初始化项目
pio_init --board esp32-s3-devkitc-1 --framework arduino

# 安装依赖库
pio_lib_install adafruit/Adafruit MPU6050
pio_lib_install ArduinoEigen  # 用于姿态解算
```

**参考Skill**: `signal-processing.md`
- 使用卡尔曼滤波器平滑MPU6050数据
- 实现50Hz时间序列采集
- 创建100帧滑动窗口

#### Phase 2: 模型训练与转换
```python
# 参考 tinyml-tensorflow.md 中的模板
# 1. 创建Conv1D + Bi-LSTM模型
# 2. 训练数据 (N, 100, 11)
# 3. 转换为TFLite INT8
# 4. 生成C++头文件
```

**参考Skill**: `tinyml-tensorflow.md`
- Conv1D + Bi-LSTM架构(参数量<100K)
- INT8量化优化
- 自动生成ESP32兼容的C++代码

#### Phase 3: ESP32部署
```cpp
// 参考 embedded-esp32.md 和 tinyml-tensorflow.md
// 1. 集成TFLite Micro
// 2. 配置FreeRTOS多任务
// 3. 实现实时推理
// 4. 添加WiFi/BLE通信
```

**参考Skill**: `embedded-esp32.md`
- FreeRTOS多任务架构
- 传感器任务 + 推理任务 + 通信任务
- OTA更新支持

#### Phase 4: PCB优化(可选)
```
使用KiCad MCP:
- 分析当前PCB设计
- 优化传感器布局
- 添加新功能模块(如压力传感器)
```

## 关键配置参数

### ESP32-S3推荐配置
```ini
[env:esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

; 使用8MB PSRAM
board_build.arduino.memory_type = qio_qspi
board_build.partitions = default_16MB.csv

build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
```

### 模型参数
- **输入**: (100, 11) - 100帧时间序列，11维特征
- **采样率**: 50Hz (20ms间隔)
- **窗口大小**: 2秒
- **Tensor Arena**: 50KB
- **模型大小**: <200KB (INT8量化)

### 传感器配置
- **弯曲传感器**: 5通道，ADC分辨率12bit
- **MPU6050**: ±8G加速度，±500deg/s陀螺仪
- **滤波**: 21Hz低通滤波
- **校准**: 1000样本平均

## 故障排除

### 常见问题

**Q: 模型在ESP32上运行缓慢?**
A: 
- 检查是否启用XTAensa DSP加速
- 使用INT8量化模型
- 减少LSTM单元数(64->32)
- 增加Tensor Arena大小

**Q: 传感器数据噪声大?**
A:
- 启用卡尔曼滤波
- 检查I2C总线速度(推荐400kHz)
- 添加去耦电容
- 使用屏蔽线缆

**Q: WiFi连接不稳定?**
A:
- 检查电源稳定性
- 使用外部天线
- 降低WiFi发射功率
- 切换到BLE通信

**Q: 内存不足?**
A:
- 升级到ESP32-S3 (8MB PSRAM)
- 减小模型大小
- 使用静态内存分配
- 优化Tensor Arena大小

## 下一步行动

1. **安装PlatformIO**: https://platformio.org/install
2. **准备ESP32-S3开发板**: 推荐使用ESP32-S3-DevKitC-1
3. **收集训练数据**: 使用改进的数据采集固件
4. **训练新模型**: 使用Conv1D + Bi-LSTM架构
5. **部署测试**: 使用FreeRTOS多任务架构

## 资源链接

- **ESP-IDF文档**: https://docs.espressif.com/
- **TensorFlow Lite Micro**: https://www.tensorflow.org/lite/microcontrollers
- **Edge Impulse社区**: https://docs.edgeimpulse.com/
- **PlatformIO文档**: https://docs.platformio.org/

## 支持

如遇到问题，请参考各Skill文件中的详细说明和代码示例。

---

**配置完成日期**: 2026-02-15
**适用项目**: Smart Glove ASL Recognition
**推荐硬件**: ESP32-S3 + MPU6050 + 5x Flex Sensors
