# Edge AI Data Glove V3 — Complete Code Framework

**项目全称：** Edge-AI-Powered Data Glove with Dual-Tier Inference for Real-Time Sign Language Translation and 3D Hand Animation Rendering

**版本：** V3.0（完全重写，移除全部 Tauri/Rust 相关内容）

**日期：** 2026-04-24

---

## 项目概述

本项目是一套「端云协同」双层推理数据手套系统，包含四个子系统：

| 子系统 | 目录 | 技术栈 | 职责 |
|--------|------|--------|------|
| **嵌入式固件** | `glove_firmware/` | PlatformIO + Arduino + FreeRTOS | 传感器驱动、信号处理、L1 边缘推理、通信 |
| **Python 中继** | `glove_relay/` | FastAPI + WebSockets + PyTorch | UDP 接收、Protobuf 解析、WebSocket 推送、L2 推理、NLP、TTS |
| **Web 前端** | `glove_web/` | React 18 + Vite + R3F + Zustand | 3D 手部骨架可视化、响应式 UI、PWA |
| **Unity Pro** | `glove_unity/` | Unity 2022 LTS + XR Hands | 高逼真度手部渲染（L3 阶段） |

---

## 目录结构

```
EdgeAI_DataGlove_V3_CodeFramework/
├── README.md                          ← 本文件
├── docs/
│   ├── SOP_SPEC_PLAN_V3.md            ← 全阶段 SOP 规范计划
│   └── CLAUDE_CODE_PROMPTS_V3.md      ← Claude Code 提示词手册
│
├── glove_firmware/                     ← ESP32-S3 嵌入式固件 (PlatformIO)
│   ├── platformio.ini                  ← PSRAM + 依赖配置
│   ├── src/
│   │   └── main.cpp                    ← FreeRTOS 双核任务入口 (Bug 已修复!)
│   ├── lib/
│   │   ├── Sensors/
│   │   │   ├── TCA9548A.h/.cpp         ← I2C 多路复用器驱动
│   │   │   ├── TMG5273.h/.cpp          ← 3D 霍尔传感器驱动 (12-bit)
│   │   │   ├── SensorManager.h         ← 统一传感器管理器
│   │   │   └── FlexManager.h           ← 弯曲传感器 (ADC 预留)
│   │   ├── Filters/
│   │   │   └── KalmanFilter1D.h        ← 1D 卡尔曼滤波 (header-only)
│   │   ├── Comms/
│   │   │   ├── glove_data.proto        ← Protobuf 序列化定义
│   │   │   ├── BLEManager.h            ← BLE 5.0 配网 + 低速备份
│   │   │   └── UDPTransmitter.h        ← WiFi UDP 发送 (端口 8888)
│   │   └── Models/
│   │       ├── BaseModel.h             ← 统一模型接口 (虚函数)
│   │       ├── TFLiteModel.h           ← TFLite Micro 实现
│   │       └── ModelRegistry.h         ← 模型注册表 + 热切换
│   ├── include/
│   │   └── data_structures.h           ← 共享数据结构 + 常量
│   └── spiffs/
│       └── model_config.yaml           ← 模型热切换配置
│
├── glove_relay/                        ← Python 中继服务器
│   ├── requirements.txt                ← pip 依赖
│   ├── pyproject.toml                  ← 项目元数据
│   ├── configs/
│   │   ├── model_config.yaml           ← L1/L2 模型配置
│   │   └── relay_config.yaml           ← 中继服务配置 (端口/CORS/推理)
│   ├── src/
│   │   ├── main.py                     ← FastAPI 入口
│   │   ├── udp_server.py              ← asyncio UDP (端口 8888)
│   │   ├── ws_server.py               ← WebSocket (端口 8765)
│   │   ├── protobuf_parser.py         ← Protobuf → JSON
│   │   ├── models/
│   │   │   ├── base_model.py          ← BaseModel 抽象接口
│   │   │   ├── l1_cnn_attention.py    ← 1D-CNN+Attention (~34K params)
│   │   │   ├── l1_ms_tcn.py           ← MS-TCN (~12K params)
│   │   │   ├── stgcn_model.py         ← ST-GCN (~280K params)
│   │   │   └── model_registry.py      ← 模型注册表
│   │   ├── nlp/
│   │   │   └── grammar_corrector.py   ← CSL 语法纠错
│   │   ├── tts/
│   │   │   └── tts_engine.py          ← edge-tts 语音合成
│   │   └── utils/
│   │       ├── config.py              ← YAML 配置加载
│   │       └── logger.py              ← 日志工具
│   ├── scripts/
│   │   ├── data_collector.py          ← 数据采集工具
│   │   └── run_benchmark.py           ← 模型 Benchmark
│   ├── data/
│   │   └── gesture_labels.json        ← 46 类手势标签
│   └── proto/
│       └── glove_data.proto           ← Protobuf 定义 (与固件一致)
│
├── glove_web/                          ← React 前端 (Vite + R3F)
│   ├── package.json                    ← npm 依赖
│   ├── vite.config.ts                  ← Vite + TailwindCSS 配置
│   ├── tsconfig.json                   ← TypeScript 配置
│   ├── public/
│   │   ├── manifest.json               ← PWA 配置
│   │   └── favicon.svg                 ← 手套图标
│   └── src/
│       ├── main.tsx                    ← 入口
│       ├── App.tsx                     ← 根组件 (响应式布局)
│       ├── index.css                   ← TailwindCSS v4
│       ├── types/index.ts              ← TS 类型定义
│       ├── hooks/
│       │   ├── useWebSocket.ts         ← WebSocket 客户端 (自动重连)
│       │   └── useHandAnimation.ts     ← 手部动画驱动
│       ├── stores/
│       │   ├── useSensorStore.ts       ← 传感器数据 Zustand
│       │   ├── useGestureStore.ts      ← 手势结果 Zustand
│       │   └── useSettingsStore.ts     ← 用户设置 Zustand
│       ├── components/
│       │   ├── Hand3D/
│       │   │   ├── HandCanvas.tsx      ← R3F Canvas
│       │   │   ├── HandSkeleton.tsx    ← 21 关键点骨架
│       │   │   └── FingerBone.tsx      ← 骨骼连接
│       │   ├── Dashboard/
│       │   │   ├── GestureResult.tsx   ← 手势显示面板
│       │   │   ├── SensorDataPanel.tsx ← 传感器数据面板
│       │   │   └── StatsPanel.tsx      ← 性能统计
│       │   ├── Settings/
│       │   │   └── SettingsSidebar.tsx ← 设置侧边栏
│       │   └── Layout/
│       │       ├── Header.tsx           ← 顶部导航
│       │       └── MobileNav.tsx        ← 移动端底栏
│       └── utils/
│           ├── constants.ts            ← 常量 (骨骼/颜色/标签)
│           └── quaternion.ts           ← 四元数工具
│
└── glove_unity/                        ← Unity L3 渲染 (骨架)
    ├── ProjectSettings/
    │   └── ProjectVersion.txt          ← Unity 2022.3 LTS
    └── Assets/
        ├── Scripts/
        │   ├── Networking/
        │   │   ├── WebSocketReceiver.cs ← WS 客户端
        │   │   └── UDPReceiver.cs       ← UDP 备选方案
        │   ├── Hand/
        │   │   ├── HandController.cs     ← 主手控制器
        │   │   ├── FingerController.cs  ← 手指控制器
        │   │   ├── HandPoseData.cs      ← 数据结构
        │   │   └── MANOHandModel.cs     ← ms-MANO 集成
        │   └── UI/
        │       ├── GestureDisplay.cs     ← 手势显示 UI
        │       └── ConnectionStatusUI.cs ← 连接状态
        └── StreamingAssets/
            ├── hand_skeleton.json       ← 骨骼定义
            └── gesture_labels.json     ← 46 类标签
```

---

## 系统架构

```
ESP32-S3 (Layer 1)                    Python Relay (Layer 2)              渲染前端 (Layer 3)
┌─────────────────┐   WiFi UDP:8888   ┌──────────────────┐  WS:8765     ┌─────────────────┐
│ TMAG5273 ×5     │                   │                  │               │                 │
│ BNO085 ×1       │──Nanopb Proto──→ │ FastAPI Server   │──JSON Push──→│ React + R3F     │
│ TCA9548A        │   (100Hz)         │ UDP Receiver     │  (100Hz)     │ 3D Hand Skeleton│
│                 │                   │ Protobuf Parser  │               │ Zustand Store   │
│ FreeRTOS        │                   │ L2 ST-GCN        │               │ TailwindCSS UI  │
│ Core1: 100Hz    │                   │ NLP Correction   │               │ PWA             │
│ Core0: L1+Comm  │                   │ edge-tts         │               │                 │
└─────────────────┘                   └──────────────────┘               └─────────────────┘
```

---

## 快速开始

### 1. 嵌入式固件 (glove_firmware)

```bash
cd glove_firmware
pio run -e esp32-s3-devkitc-1        # 编译
pio device monitor                    # 串口监视
```

### 2. Python 中继 (glove_relay)

```bash
cd glove_relay
pip install -r requirements.txt
uvicorn src.main:app --host 0.0.0.0 --port 8765   # 启动
```

### 3. Web 前端 (glove_web)

```bash
cd glove_web
npm install
npm run dev                           # 开发 http://localhost:5173
npm run build                         # 构建
```

### 4. Unity (glove_unity)

1. 用 Unity 2022.3 LTS 打开
2. 安装 XR Hands Package
3. 导入 ms-MANO 模型
4. 配置 WebSocket URL

---

## 关键设计决策 (V3)

| 决策点 | 选择 | 理由 |
|--------|------|------|
| D1: 渲染框架 | React + R3F (MVP) → Unity (Pro) | 移除 Tauri/Rust，纯 Web 零安装 |
| D2: L2 图神经网络 | 自研 ST-GCN | 移除 OpenHands (已停维护) |
| D3: 通信架构 | UDP 8888 + WS 8765 | 前端只接收 JSON，不理解 Protobuf |
| D4: 模型管理 | BaseModel + YAML 热切换 | 运行时切换 L1 模型无需重启 |
| D5: BLE 角色 | 仅配网 + 低速备份 | 不使用 Web Bluetooth API |

---

## 已知 Bug 修复 (V2 → V3)

**FreeRTOS xTaskCreatePinnedToCore 参数顺序：**
- ❌ 错误：`xTaskCreatePinnedToCore(TaskSensorReadHandle, ...)`
- ✅ 修复：`xTaskCreatePinnedToCore(Task_SensorRead, ...)`
- 添加了 `static_assert` 编译期验证

---

## 配套文档

| 文档 | 文件 | 说明 |
|------|------|------|
| SOP 规范计划 | `docs/SOP_SPEC_PLAN_V3.md` | 全阶段标准操作规程 |
| Claude Code 提示词 | `docs/CLAUDE_CODE_PROMPTS_V3.md` | 28 个分阶段提示词 (P0-P8) |

---

## 许可证

本项目为学术研究项目，用于毕业论文 / 学术发表。
