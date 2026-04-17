# Edge AI Data Glove — Project Context, Analysis & Decision Records

> This file captures the critical thinking, analysis process, and key decisions from the project brainstorming session. It is intended as context for Claude Code to understand the project background and rationale behind architectural choices.

---

## 1. Project Overview

**Project**: Edge-AI-Powered Data Glove with Dual-Tier Inference for Real-Time Sign Language Translation and 3D Hand Animation Rendering

**Goal**: Build a complete system with dual-tier inference architecture: L1 (edge/on-device) for fast simple gesture recognition + L2 (upper-computer/PC) for complex dynamic sign language recognition, with 3D hand animation rendering.

**Original Paper**: "Edge-AI-Powered Data Glove with Dual-Tier Inference for Real-Time Sign Language Translation and 3D Hand Animation Rendering" — the project is a reproduction and upgrade of this paper.

---

## 2. Source Material Analysis — Problems Found ("糟粕")

### 2.1 Critical: Fake ST-GCN Implementation (l2_inference.py)

**Problem**: The paper claims to use OpenHands ST-GCN for L2 inference, but the actual implementation in `l2_inference.py` is completely fake:

```python
class STGCNModel(nn.Module):
    def __init__(self, num_classes=46):
        super(STGCNModel, self).__init__()
        self.fc = nn.Linear(42 * 30, num_classes)  # This is NOT ST-GCN!

    def forward(self, x):
        x = x.view(x.size(0), -1)
        return self.fc(x)
```

This is a single `nn.Linear` layer that flattens the input and directly maps to class logits. There is **zero graph convolution structure** — no spatial graph convolution, no temporal convolution, no adjacency matrix, no residual connections. All spatial (hand skeleton topology) and temporal (action sequence) information is destroyed.

**Decision**: Build a real ST-GCN from scratch based on the MS-GCN3 original paper.

### 2.2 Critical: FreeRTOS xTaskCreatePinnedToCore Parameter Order Bug (main.cpp L190-192)

```cpp
// WRONG — function pointer and handle are SWAPPED:
xTaskCreatePinnedToCore(TaskSensorReadHandle, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);

// CORRECT:
xTaskCreatePinnedToCore(Task_SensorRead, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);
```

**API signature**: `xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char* const pcName, const uint32_t usStackDepth, void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pvCreatedTask, const BaseType_t xCoreID)`

Parameter 1 must be the **function pointer** (`Task_SensorRead`), not the handle variable. Passing the handle variable as the function pointer causes the RTOS to execute an integer address as code, triggering a Cache Access Exception crash.

### 2.3 High: OpenHands Library — Unmaintained and Incompatible

**Finding**: OpenHands (the ST-GCN library referenced in the paper) stopped maintenance in 2023. It only supports MediaPipe keypoint input format, not wearable sensor data. The library's graph convolution implementation also diverges from the MS-GCN3 paper description.

**Decision**: Completely abandon OpenHands. Build ST-GCN from MS-GCN3 paper from scratch.

### 2.4 Medium: BOM Cost Discrepancy

**Finding**: The paper claims a BOM cost of $18, but actual calculation shows ~$38.60 (individual purchase, module form factor including DevKitC-1 board at ~$13.50, TMAG5273 at ~$2.80 each, BNO085 at ~$9.50, etc.)

**Note**: The $18 figure likely represents bulk/naked die pricing without PCB, assembly, or casing costs.

### 2.5 Medium: Over-engineered Communication Architecture

**Finding**: Original SOP-Master-Plan specified 3 simultaneous communication links (WiFi UDP + WiFi WebSocket + BLE), which is unnecessarily complex for MVP.

**Decision**: Simplified to phased approach — BLE only for Phase 1-3, add WiFi UDP for Phase 4+.

### 2.6 Medium: Oversimplified NLP Module

**Finding**: The NLP module in `l2_inference.py` is a trivial 3-word SOV→SVO reordering:
```python
if len(words) >= 3:
    s, o, v = words[0], words[1], words[2]
    return f"{s} {v} {o}"
```

Chinese Sign Language (CSL) grammar is far more complex than simple three-word reordering. Need proper rule-based system or lightweight Transformer model.

### 2.7 Low: Missing Edge Impulse CSV Serial Output

**Finding**: The `main.cpp` has the sliding window buffer and callback function for Edge Impulse, but the serial CSV output is commented out (`// Serial.printf(...)`), so edge-impulse-data-forwarder cannot actually receive data.

---

## 3. Source Material Analysis — Strengths Found ("精华")

### 3.1 Dual-Tier Inference Architecture Concept
The L1 (edge, fast, simple) + L2 (upper-computer, accurate, complex) architecture is a sound design. L1 provides <3ms inference for common gestures with offline capability; L2 handles complex sign language sequences.

### 3.2 Hardware Selection — ESP32-S3 + TMAG5273 + BNO085
- ESP32-S3-DevKitC-1 N16R8: Dual-core 240MHz, AI vector instructions, 16MB PSRAM — excellent for TinyML
- TMAG5273A1: Linear 3D Hall sensor with Set/Reset trigger, far more durable and linear than flex sensors
- BNO085: Hardware quaternion fusion (SH-2 protocol) offloads computation from ESP32

### 3.3 I2C Multiplexer Architecture
TCA9548A for 5x TMAG5273 address conflict resolution — clean, proven approach.

### 3.4 Nanopb Serialization
Using Protocol Buffers (Nanopb variant) for data framing is efficient and extensible.

### 3.5 L1 1D-CNN + Attention Model Architecture
The `l1_edge_model.py` model is well-designed: 3 Conv1d blocks + TemporalAttention (Eq.11-13) + FC. ~34K params fits comfortably in ESP32-S3 PSRAM.

### 3.6 FreeRTOS Dual-Core Task Scheduling
The concept of Core 1 for sensor sampling (100Hz) and Core 0 for inference + communication is correct (implementation had the parameter order bug, but the architecture is sound).

---

## 4. Confirmed Architecture Decisions (ADR)

### ADR-1: TinyML Dual-Path Strategy
- **Path A (MVP)**: Edge Impulse — fast validation, 2-3 day cycle, but model structure limited
- **Path B (Paper Reproduction)**: PyTorch native training → TFLite INT8 — fully controllable, reproducible, supports custom TemporalAttention
- Both paths share data collection pipeline and sensor drivers

### ADR-2: Self-Built ST-GCN from MS-GCN3 Paper
- **Rejected**: OpenHands (unmaintained, incompatible with wearable data)
- **Built from scratch**: 
  - Pseudo-skeleton Mapping layer (Eq.16): 21 sensor features → 21×2D skeleton keypoints
  - Spatial Graph Convolution: adjacency matrix based on hand anatomy
  - Temporal Convolution: TCN for sequential dependency
  - ST-Conv Block: spatial → temporal → residual connection
  - Attention Pooling: channel attention for temporal feature aggregation

### ADR-3: Phased Communication Architecture
- **Phase 1-3**: BLE 5.0 only (simplifies development)
  - BLE GATT: provisioning (WiFi SSID/Password), data notification
  - Serial CSV: for edge-impulse-data-forwarder
- **Phase 4+**: BLE + WiFi UDP
  - WiFi UDP: 100Hz sensor data broadcast for 3D rendering (<1ms latency)
  - BLE degrades to: provisioning + low-speed backup (20Hz)

### ADR-4: Three-Stage Rendering Evolution
- **Phase 2 MVP**: Tauri 2.0 + R3F — fast cross-platform desktop validation (2-3 weeks)
- **Phase 2.5**: Three.js + WebSocket — browser-based, zero installation
- **Phase 4+**: Unity 2022 LTS + XR Hands + ms-MANO — high-fidelity professional rendering
- All stages use compatible bone definitions for smooth evolution

### ADR-5: Hardware Platform Retention (ESP32-S3-DevKitC-1)
- **Rejected**: Switch to Seeed XIAO ESP32S3
- **Reason**: DevKitC-1 has 16MB PSRAM (vs XIAO's 8MB); ST-GCN intermediate activations need 2-3MB runtime; total system peak ~4-5MB. XIAO's usable PSRAM (~5-6MB after OS overhead) is insufficient margin. Also, DevKitC-1 PCB layout is already validated.

---

## 5. Key Technical Specifications

### Hardware
| Component | Model | Qty | Interface | Key Spec |
|-----------|-------|-----|-----------|----------|
| MCU | ESP32-S3-DevKitC-1 N16R8 | 1 | — | Dual-core 240MHz, 16MB PSRAM, AI vector ext |
| Hall Sensor | TMAG5273A1 | 5 | I2C (via TCA9548A) | 12-bit, ±40mT, Set/Reset trigger |
| IMU | BNO085 | 1 | I2C (0x4A) | 9-axis, HW quaternion fusion, SH-2 |
| I2C Mux | TCA9548A | 1 | I2C (0x70) | 8-channel |
| Battery | 600mAh 3.7V Li-Po | 1 | — | Target >12h runtime |

### Software Stack
| Layer | Technology |
|-------|-----------|
| Embedded | PlatformIO + Arduino, FreeRTOS, Nanopb, TFLite Micro |
| AI/ML | Python 3.9+, PyTorch, custom ST-GCN, edge-tts |
| Rendering (MVP) | Tauri 2.0 + React + R3F + Zustand + TailwindCSS |
| Rendering (Mid) | Three.js + WebSocket (browser) |
| Rendering (Final) | Unity 2022 LTS + XR Hands + ms-MANO |

### Performance Targets
| Metric | Target |
|--------|--------|
| L1 inference latency | <3ms |
| L2 inference latency | <20ms (GPU/CPU) |
| End-to-end latency | <100ms (with TTS), <50ms (without) |
| L1 accuracy (20 classes) | >90% Top-1 |
| L2 accuracy (46 classes) | >95% Top-1, >99% Top-5 |
| Battery life | >12h (600mAh) |
| Sensor sampling rate | 100Hz |
| System weight | <50g |

---

## 6. File Reference Map

### Original Source Files (from zip attachment)
| File | Description | Issue Found |
|------|-------------|-------------|
| `main.cpp` | FreeRTOS task framework, Edge Impulse integration | xTaskCreatePinnedToCore param order bug |
| `l2_inference.py` | L2 ST-GCN inference + NLP | Fake ST-GCN (just nn.Linear), trivial NLP |
| `l1_edge_model.py` | L1 1D-CNN+Attention model | Good design, use as reference |
| `data_collector.py` | UDP data collection script | Functional, needs BLE support |
| `glove_data.proto` | Protobuf data frame definition | Good, minor additions needed |
| `model_data.h` | Placeholder TFLite model header | Placeholder only |
| `platformio.ini` | PlatformIO project config | Functional |
| `SensorManager.h` | TMAG5273 + BNO085 driver header | Good reference |
| `FlexManager.h` | Flex sensor driver | Future use |
| `Kalman1D.h` | 1D Kalman filter | Good reference |
| `BLEManager.h` | BLE GATT service | Functional |
| `WSManager.h` | WebSocket manager | Phase 4+ only |
| `UDPTransmitter.h` | UDP sender | Phase 4+ only |
| `SOP-Master-Plan.md` | Original master plan | Over-complex comms |
| `SPEC-Plan.md` | Original spec plan | References OpenHands (obsolete) |
| `Firmware-L1-SOP.md` | Firmware SOP | Good reference |
| `AI-L2-SOP.md` | AI upper-computer SOP | Uses OpenHands (obsolete) |
| `Rendering-SOP.md` | Rendering SOP | Good reference |
| `Dashboard-Tauri-SOP.md` | Tauri dashboard SOP | Good reference |
| `Hardware-Spec.md` | Hardware specification | Good reference |
| `BOM.md` | Bill of materials | $38.60 actual vs $18 claimed |
| `PDF Paper` | Original research paper | Contains fake implementations |

### Generated Output Files
| File | Description |
|------|-------------|
| `Edge_AI_Data_Glove_SOP_SPEC_PLAN.docx` | Full-phase SOP SPEC PLAN (Chinese, 77KB) |
| `Edge_AI_Data_Glove_SOP_SPEC_PLAN.md` | Markdown version (Chinese) |
| `Edge_AI_Data_Glove_SOP_SPEC_PLAN_EN.md` | English Markdown version |
| `Claude_Code_Prompt_数据手套项目.docx` | Claude Code Prompt handbook (Chinese, 68KB) |
| `Claude_Code_Prompt_数据手套项目.md` | Markdown version (Chinese) |
| `Claude_Code_Prompt_DataGlove_EN.md` | English Markdown version |
| `PROJECT_CONTEXT_AND_DECISIONS.md` | This file — analysis & decision context |

---

## 7. References Evaluated

| Reference | URL | Verdict | Rationale |
|-----------|-----|---------|-----------|
| Edge Impulse | edgeimpulse.com | **Adopted (MVP path)** | Fast prototyping, EON Compiler, good for validation |
| OpenHands | github.com/OpenHands | **Rejected** | Unmaintained since 2023, no wearable sensor support |
| CASA0018-Gloves-Edge-AI | GitHub repo | **Referenced** | Similar project for architecture reference |
| XIAO ESP32S3 | Seeed Studio | **Rejected** | Insufficient PSRAM (8MB vs needed 16MB) |
| MS-GCN3 Paper | CVPR/ArXiv | **Adopted** | Foundation for self-built ST-GCN |

---

## 8. Recommended Claude Code Usage

For best results when using the Claude Code Prompt document:

1. **Follow phase order** — P0.x → P1.x → P2.x → ... → P9.x (bug fixes can be done anytime)
2. **Execute one prompt at a time** — Each prompt is self-contained but depends on previous phases
3. **Use the English prompt file** (`Claude_Code_Prompt_DataGlove_EN.md`) for Claude Code — English prompts produce better results
4. **Reference this context file** — When Claude Code asks about design decisions, point to the ADR sections
5. **Verify after each phase** — Run the acceptance criteria before moving to the next phase
