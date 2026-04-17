# Firmware-L1-SOP: 嵌入式端开发指南

## 1. 驱动层 (Driver Layer)
- **TMAG5273**: 实现 `ReadXYZ()`，利用 12-bit 原始数据。
- **BNO085**: 开启 `SH2_GAME_ROTATION_VECTOR` (100Hz) 和 `SH2_GYROSCOPE_CALIBRATED`。

## 2. 算法层 (DSP & AI)
- **滤波**: 1D 卡尔曼滤波处理 15 路霍尔信号。
- **特征提取**: 归一化霍尔数据，将四元数转为欧拉角。
- **L1 推理 (Edge Impulse)**: 
  - 框架: Edge Impulse C++ SDK (EON Compiler 优化)。
  - 触发: 滑动窗口 (30 frames, 300ms)。
  - 逻辑: 填充 `signal_t`，调用 `run_classifier()`。若 `result.classification[i].value > 0.85` 则本地输出，否则标记为 `UNKNOWN` 触发 L2 转发。

## 3. 通信层 (Comms: WiFi + BLE)
- **WiFi UDP (主链路-3D渲染)**: 100Hz 广播传感器原始/滤波数据，允许丢包，追求极低延迟，供 Unity 实时渲染。
- **WiFi WebSocket (主链路-语义识别)**: TCP 可靠传输，用于发送 L1 识别结果及 75 帧动态特征序列，确保手语翻译不丢失。
- **BLE 5.0 (辅助链路)**:
  - **配网**: 接收手机下发的 SSID 和 Password。
  - **低速备用**: 在 WiFi 断开时降级为 20Hz 姿态广播。
  - **直连**: 供原生 App (iOS/Android) 直接获取数据。
- **数据帧**: 统一使用 Protobuf (Nanopb) 序列化，减小开销。

## 4. Claude Code 任务点
- "请实现一个基于 FreeRTOS 的双核调度方案：Core 1 负责 100Hz 采样与滤波，Core 0 负责 Edge Impulse 推理、WiFi (UDP+WebSocket) 传输以及 BLE GATT 服务。"
- "请编写一个串口打印函数，格式严格遵循 `edge-impulse-data-forwarder` 要求的 CSV 格式（逗号分隔，换行结尾），以便于直接推送到云端训练。"
