# SOP-Master-Plan: Edge-AI Data Glove (Claude Code Optimized)

## 1. 项目愿景
构建一个具备“端云协同”能力的数据手套，实现从底层传感器融合到高层语义理解及3D实时渲染的全栈闭环。
- **前期 MVP**: 基于 Tauri + React Three Fiber (Web/跨端桌面) 实现快速 3D 验证与仪表盘。
- **后期高精度**: 基于 Unity + XR Hands 实现高逼真度 ms-MANO 渲染。

## 2. 核心架构设计 (Dual-Tier & Dual-Mode Comms)
- **L1 (Edge)**: ESP32-S3 负责 100Hz 采样、卡尔曼滤波、1D-CNN+Attention 简单手势识别。
- **L2 (Upper)**: PC/Mobile 负责 ST-GCN 复杂动态手语识别、NLP 语法纠错。
- **通信链路 (WiFi + BLE)**:
  - **WiFi UDP**: 100Hz 实时 3D 姿态广播（最低延迟，无 OS 蓝牙栈缓冲问题）。
  - **WiFi WebSocket (TCP)**: 识别结果与动态特征批量传输。
  - **BLE 5.0**: 设备配网、低速备用链路。
- **跨端仪表盘 (Tauri)**: Rust 底层监听 UDP/TCP，React 前端负责 UI，R3F 负责 3D 渲染。

## 3. 开发阶段 (Sprints)
1. **Sprint 1: HAL & Driver Layer**: 实现 TMAG5273 和 BNO085 的稳定驱动。
2. **Sprint 2: Comms Pipeline**: 实现 WiFi (UDP + WebSocket) 主链路与 BLE 5.0 辅助链路。
3. **Sprint 3: Edge Impulse TinyML**: 
   - 使用 `edge-impulse-data-forwarder` 采集传感器数据。
   - 在 EI Studio 训练 1D-CNN 模型（开启 EON Compiler）。
   - 导出 Arduino Library 并集成至 PlatformIO。
4. **Sprint 4: Tauri Cross-Platform Dashboard**: 构建 Rust UDP 监听器与 React Three Fiber 3D 渲染界面。
5. **Sprint 5: L2 AI & NLP**: 集成 OpenHands 框架，实现 SOV->SVO 转换。

## 4. Claude Code 协作指令 (Prompt Engineering)
在让 Claude Code 执行任务时，请严格使用以下格式的提示词：
- **查阅文档指令**: "在执行此任务前，请先使用 playwright 或文件读取工具，仔细阅读 `docs/references/TMAG5273_Datasheet.pdf` 中的寄存器配置章节。"
- **TinyML 部署指令**: "请使用 Edge Impulse 导出的 C++ SDK 进行推理。不要使用原生的 TFLite Micro 解释器。请构建 `signal_t` 结构体，并调用 `run_classifier(&signal, &result, false)` 获取推理结果。"
- **技术栈约束指令**: "请使用 Tauri 2.0 + React + TailwindCSS 构建前端。注意：必须在 Rust 端 (`src-tauri/src/main.rs`) 实现 UDP Socket 监听，并通过 Tauri Events 将数据高频发送给前端。前端必须使用 Zustand 的 transient updates 处理 100Hz 数据，防止 React 频繁重渲染。"
