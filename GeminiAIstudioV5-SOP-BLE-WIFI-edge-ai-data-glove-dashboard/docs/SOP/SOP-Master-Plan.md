# SOP-Master-Plan: Edge-AI Data Glove (Claude Code Optimized)

## 1. 项目愿景
构建一个具备“端云协同”能力的数据手套，实现从底层传感器融合到高层语义理解及3D实时渲染的全栈闭环。

## 2. 核心架构设计 (Dual-Tier & Dual-Mode Comms)
- **L1 (Edge)**: ESP32-S3 负责 100Hz 采样、卡尔曼滤波、1D-CNN+Attention 简单手势识别（<3ms）。
- **L2 (Upper)**: PC/Mobile 负责 ST-GCN 复杂动态手语识别、NLP 语法纠错、ms-MANO 3D 渲染。
- **通信链路 (WiFi + BLE)**:
  - **WiFi UDP**: 100Hz 实时 3D 姿态广播（最低延迟，允许丢包）。
  - **WiFi WebSocket (TCP)**: 识别结果与动态特征批量传输（可靠传输，防丢失）。
  - **BLE 5.0**: 设备配网、20Hz 低速备用链路、手机 App 直连。

## 3. 开发阶段 (Sprints)
1. **Sprint 1: HAL & Driver Layer**: 实现 TMAG5273 和 BNO085 的稳定驱动，建立 I2C 互斥机制。
2. **Sprint 2: Comms & Data Pipeline**: 实现 WiFi (UDP + WebSocket) 主链路与 BLE 5.0 辅助链路。放弃 ESP-NOW。
3. **Sprint 3: L1 TinyML Deployment**: 训练并部署 INT8 量化模型至 TFLite Micro。
4. **Sprint 4: L2 AI & NLP**: 集成 OpenHands 框架，实现 SOV->SVO 转换。
5. **Sprint 5: Unity Rendering**: 实现 XR Hands 接入与 ms-MANO 骨骼驱动。

## 4. Claude Code 协作指令
- **代码生成**: 优先使用 C++17 特性，确保 FreeRTOS 任务优先级分配合理。
- **调试**: 遇到 I2C 阻塞时，要求 Claude 检查 `Wire` 库的超时设置及 TCA9548A 的通道切换逻辑。
- **优化**: 要求 Claude 针对 ESP32-S3 的 S3-AI 指令集优化卷积运算。
