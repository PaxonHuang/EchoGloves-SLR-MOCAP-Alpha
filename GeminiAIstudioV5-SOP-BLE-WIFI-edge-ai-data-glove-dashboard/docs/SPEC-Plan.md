# SPEC-Plan: Edge-AI-Powered Data Glove

## 1. 项目概述
本项目复现论文《Edge-AI-Powered Data Glove with Dual-Tier Inference》，旨在构建一个实时手语翻译与3D手部动画渲染系统。系统采用双层推理架构，平衡低延迟与高精度。

## 2. 系统架构

### 2.1 硬件层 (Hardware)
- **MCU**: ESP32-S3-DevKitC-1 (双核 240MHz, AI 向量指令集优化)。
- **传感器**: 
  - 5x TMAG5273A1 (MCP关节，I2C接口)。
  - 1x BNO085 (腕部，9轴IMU，硬件融合输出四元数)。
  - *预留*: 5路 ADC 引脚 (GPIO 4, 5, 6, 7, 15) 用于未来扩展柔性传感器。
- **I2C 拓扑**: 使用 TCA9548A 1-to-8 多路复用器解决 TMAG5273 地址冲突。

### 2.2 软件栈 (Software)
- **嵌入式 (Firmware)**: PlatformIO + Arduino, FreeRTOS 任务调度, Nanopb 序列化, TFLite Micro 推理引擎。
- **上位机 (AI & NLP)**: Python 3.9+, PyTorch, OpenHands ST-GCN, edge-tts (语音合成)。
- **渲染端 (Rendering)**: Unity 2022 LTS, XR Hands Package, ms-MANO 参数化模型。

## 3. 开发阶段规划

### Phase 1: 硬件驱动与 HAL 层
- 初始化 I2C 总线，配置 TCA9548A。
- 实现 TMAG5273 和 BNO085 的驱动逻辑。
- 建立 FreeRTOS 任务：`Task_SensorRead` (100Hz)。

### Phase 2: 信号处理与降噪
- 实现 1D 卡尔曼滤波器 (Kalman Filter)。
- 对 21 维原始数据（15路霍尔 + 6路IMU）进行实时滤波。

### Phase 3: 通信协议与数据汇聚
- 定义 `glove_data.proto`。
- 实现 UDP 传输层，将数据打包发送至上位机。

### Phase 4: 数据采集与标注
- 编写 Python 脚本接收 UDP 数据并保存为 CSV/NPY。
- 采集 46 类手势数据集。

### Phase 5: 边缘 AI 训练与部署 (L1)
- 在 PyTorch 中训练 1D-CNN+Attention 模型。
- 模型量化 (INT8) 并导出为 `model_data.h`。
- 集成 TFLite Micro 进行端侧推理。

### Phase 6: 上位机 AI 与 NLP (L2)
- 实现伪骨骼映射 (Pseudo-skeleton mapping)。
- 运行 ST-GCN 模型进行二次判定。
- 实现 SOV 到 SVO 的语法转换逻辑。

### Phase 7: Unity 3D 渲染集成
- 编写 C# UDP 接收器。
- 实现传感器数据到 MANO 姿态参数的映射。

## 4. 性能指标目标
- **L1 推理延迟**: < 3ms。
- **端到端延迟**: < 50ms。
- **识别准确率**: > 95%。
- **续航时间**: > 12 小时 (600mAh 电池)。
