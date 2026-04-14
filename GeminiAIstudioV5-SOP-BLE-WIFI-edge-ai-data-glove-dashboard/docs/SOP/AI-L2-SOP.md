# AI-L2-SOP: 上位机推理与语义处理

## 1. OpenHands 集成
- **输入**: 接收来自 ESP32 的 21 维特征向量。
- **映射**: 运行 `Pseudo-skeleton mapping` 线性层，将传感器数据映射为 21 个手部关键点坐标。
- **识别**: 运行 ST-GCN (Spatial-Temporal Graph Convolutional Network) 处理动态序列。

## 2. NLP 语法纠错
- **逻辑**: 针对手语 SOV (Subject-Object-Verb) 特性，建立规则库或使用轻量级 Transformer 进行纠错。
- **示例**: "我 苹果 吃" -> "我吃苹果"。

## 3. 语音合成 (TTS)
- **库**: 使用 `edge-tts` 或 `pyttsx3`。
- **模式**: 异步处理，确保翻译结果实时播报。

## 4. Claude Code 任务点
- "请根据 OpenHands 规范，编写一个 Python 脚本，实现 UDP 数据的实时接收、ST-GCN 推理以及基于规则的语法纠错逻辑。"
