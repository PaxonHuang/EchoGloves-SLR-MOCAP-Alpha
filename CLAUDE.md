# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Project Overview

**Edge-AI Data Glove V3**: Dual-tier inference system for real-time sign language translation and 3D hand animation. 

**Architecture**:
- **Layer 1 (Edge)**: ESP32-S3 with 1D-CNN+Attention L1 model (<3ms latency)
- **Layer 2 (Relay)**: Python FastAPI + ST-GCN + NLP + TTS
- **Layer 3a (Web MVP)**: React 18 + Vite + R3F (3D hand skeleton)
- **Layer 3b (Unity Pro)**: Unity 2022 LTS + XR Hands + ms-MANO

**Key Decisions**:
- **No Rust/Tauri**: V3 uses pure Web (React + R3F), no desktop framework
- **Python Relay**: Unified hub for UDP→WebSocket conversion + L2 inference
- **Model Hot-Switch**: BaseModel interface + YAML config switching
- **BLE for Provisioning Only**: Frontend uses WiFi→Relay→WebSocket

---

## Directory Structure

```
├── glove_firmware/      # ESP32-S3 PlatformIO firmware
├── glove_relay/         # Python FastAPI relay server
├── glove_web/           # React + R3F frontend
├── glove_unity/         # Unity L3 Pro skeleton
├── docs/                # SOP and Prompts
│   ├── SOP_SPEC_PLAN_V3.md
│   └── CLAUDE_CODE_PROMPTS_V3.md
├── archive/             # Old code backup (can be deleted)
└── CLAUDE.md
```

---

## Build Commands

### Firmware (glove_firmware)
```powershell
cd glove_firmware
# Build (use PowerShell on Windows — bash produces no output)
pio run

# Upload to ESP32-S3
pio run -t upload

# Serial monitor (115200 baud)
pio device monitor

# Build with debug configuration
pio run -e esp32-s3-devkitc-1-n16r8_debug
```

### Python Relay (glove_relay)
```bash
cd glove_relay
pip install -r requirements.txt
uvicorn src.main:app --host 0.0.0.0 --port 8000 --reload
```

### Web Frontend (glove_web)
```bash
cd glove_web
npm install
npm run dev                # Dev server http://localhost:5173
npm run build              # Production build
npm run preview            # Preview production build
```

**Vite Config:** `vite.config.ts` includes path alias `@/` pointing to `src/`

---

## Critical Bug Fix (FreeRTOS)

**V2 Bug**: `xTaskCreatePinnedToCore(TaskSensorReadHandle, ...)` — passed handle variable as function pointer

**V3 Fix**: `xTaskCreatePinnedToCore(Task_SensorRead, ...)` — correct function pointer + `static_assert` validation

---

## FreeRTOS Task Architecture

| Task | Core | Priority | Frequency | Purpose |
|------|------|----------|-----------|---------|
| Task_SensorRead | 1 | 3 | 100Hz | I2C sampling + Kalman filter |
| Task_Inference | 0 | 2 | ~30Hz | L1 model inference |
| Task_Comms | 0 | 1 | 100Hz | BLE provisioning + UDP send |

---

## Data Flow

```
ESP32-S3                    Python Relay              Web Frontend
┌─────────────┐             ┌────────────┐           ┌───────────┐
│ Sensors     │ UDP:8888    │ FastAPI    │ WS:8765   │ React+R3F │
│ L1 Inference │──Protobuf─→│ Protobuf→  │──JSON──→ │ 3D Hand   │
│ FreeRTOS    │             │ JSON       │           │ Skeleton  │
└─────────────┘             │ L2 ST-GCN  │           └───────────┘
                            │ NLP + TTS  │
                            └────────────┘
```

---

## Model Hot-Switch Architecture

- **BaseModel Interface**: Python (`glove_relay/src/models/base_model.py`) + C++ (`glove_firmware/lib/Models/BaseModel.h`)
- **Model Registry**: Dynamically load/switch models at runtime
- **YAML Config**: `glove_relay/configs/model_config.yaml` defines active model

---

## Key Files

| File | Purpose |
|------|---------|
| `glove_firmware/src/main.cpp` | FreeRTOS tasks with static_assert fix |
| `glove_firmware/lib/Models/ModelRegistry.h` | L1 model hot-switch |
| `glove_relay/src/main.py` | FastAPI + WebSocket relay |
| `glove_relay/src/models/stgcn_model.py` | L2 ST-GCN implementation |
| `glove_web/src/hooks/useWebSocket.ts` | WebSocket client with auto-reconnect |
| `glove_web/src/components/Hand3D/HandSkeleton.tsx` | 21-keypoint 3D hand |

---

## Hardware Context

- **MCU**: ESP32-S3-DevKitC-1 N16R8 (8MB Flash + 8MB PSRAM)
- **I2C**: GPIO 8 (SDA), GPIO 9 (SCL), 400kHz
- **Sensors**: 5× TMAG5273 (via TCA9548A mux), 1× BNO085 (address 0x4A)

---

## Performance Targets

| Metric | Target |
|--------|--------|
| L1 inference latency | <3ms |
| L2 inference latency | <20ms |
| End-to-end latency | <100ms |
| Sensor sampling rate | 100Hz |
| L1 accuracy (46 classes) | >90% Top-1 |
| L2 accuracy (46 classes) | >95% Top-1 |

---

## Development Phases

1. **P0**: Project init (PlatformIO + React + FastAPI)
2. **P1**: HAL & drivers (TMAG5273, BNO085, TCA9548A)
3. **P2**: Signal processing (Kalman filter)
4. **P3**: L1 Edge Impulse / PyTorch training
5. **P3.5**: Model Benchmark comparison
6. **P4**: Communication (BLE provisioning + WiFi UDP)
7. **P5**: Python Relay + L2 ST-GCN + NLP + TTS
8. **P6**: Web rendering (React + R3F) / Unity Pro
9. **P7**: Integration testing

---

## Documentation

- **SOP**: `docs/SOP_SPEC_PLAN_V3.md` — Full phase specification (938 lines)
- **Prompts**: `docs/CLAUDE_CODE_PROMPTS_V3.md` — 28 executable prompts

---

## Skill Agents Available

- `esp32-firmware-engineer`: ESP-IDF/Arduino firmware, FreeRTOS
- `embedded-systems`: RTOS, memory optimization
- `feature-dev`: Guided feature development

Invoke with `/agent esp32-firmware-engineer` for firmware tasks.

---

## PlatformIO Dependency Notes

- **lib_deps syntax**: Use `owner/libname @ version` (space before @), NOT `owner/libname=@version`
- **TMAG5273**: Local driver in `lib/Sensors/TMG5273.h/.cpp` — do NOT add SparkFun TMAG5273 library to lib_deps (already in lib_deps for reference, but local implementation is used)
- **TFLite Micro**: Use `tensorflow/tflite-micro-esp32` (ESP32 optimized)
- **ESPAsyncUDP**: Use `mathieucarbou/ESPAsyncUDP` (note: registry availability may vary)
- **Build on Windows**: Use PowerShell for `pio run` — bash produces no output on this system
- **pio path**: `C:\Users\QuenchKidney\.platformio\penv\Scripts\pio.exe`

## Library Architecture

### Firmware (`glove_firmware/lib/`)
| Directory | Purpose |
|-----------|---------|
| `Sensors/` | TMAG5273 driver, TCA9548A mux, SensorManager |
| `Models/` | BaseModel interface, TFLiteModel, ModelRegistry |
| `Comms/` | BLEManager, UDPTransmitter, Protobuf |
| `Filters/` | Kalman filter implementations |

### Relay (`glove_relay/src/`)
| Module | Purpose |
|--------|---------|
| `models/` | ST-GCN L2 inference, ModelRegistry, base_model interface |
| `nlp/` | CSL→Mandarin grammar correction |
| `tts/` | edge-tts voice synthesis |
| `utils/` | Config loading, logging |

### Web (`glove_web/src/`)
| Directory | Purpose |
|-----------|---------|
| `components/Hand3D/` | 21-keypoint hand skeleton (R3F) |
| `hooks/` | useWebSocket with auto-reconnect |
| `stores/` | Zustand state management |
| `types/` | TypeScript type definitions |

---

## Gotchas

- Session continuation `.txt` files appear in `glove_firmware/` root — add `*this-session-is-being-continued*` to `.gitignore`
- `pio pkg search` crashes with UnicodeEncodeError on Windows — use `Out-File` + `Get-Content` workaround in PowerShell
- **WebSocket Port**: Relay uses port **8765** for WebSocket (not 8000)
- **UDP Port**: ESP32 sends to port **8888** (configured in `platformio.ini`)
- **Relay Host**: Web frontend connects to `ws://${relayHost}:8765` — default is `localhost`

## Testing

### Firmware Tests
```powershell
cd glove_firmware
pio test
```

### Relay Tests
```bash
cd glove_relay
python -m pytest tests/
```