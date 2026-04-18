# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Project Overview

**Edge-AI Data Glove**: Dual-tier inference system for real-time sign language translation and 3D hand animation. L1 (edge/on-device ESP32-S3) handles fast simple gesture recognition; L2 (PC) handles complex dynamic sign language recognition with ST-GCN.

**Key Architecture Decisions**:
- **TinyML Dual-Path**: Edge Impulse (fast MVP) OR PyTorch→TFLite INT8 (full reproduction)
- **Self-built ST-GCN**: Not using OpenHands (unmaintained); building from MS-GCN3 paper
- **Phased Communication**: BLE only for Phase 1-3; BLE + WiFi UDP for Phase 4+
- **Hardware**: ESP32-S3-DevKitC-1 N16R8 (16MB PSRAM), TMAG5273 Hall sensors, BNO085 IMU, TCA9548A I2C mux

---

## Build Commands

### Firmware (PlatformIO)
```bash
# Build firmware
pio run

# Build and upload to device
pio run -t upload

# Monitor serial output (115200 baud)
pio device monitor -b 115200

# Clean build
pio run -t clean
```

### Frontend (React/Vite)
```bash
cd source/frontend
npm install
npm run dev      # Development server on port 3000
npm run build    # Production build
npm run lint     # TypeScript check
```

### Python Scripts (L2 Inference)
```bash
cd source/scripts
python l2_inference.py
python l1_edge_model.py
```

---

## Critical Bugs Fixed

### xTaskCreatePinnedToCore Parameter Order (main.cpp L190-192)
**WRONG**: `xTaskCreatePinnedToCore(TaskSensorReadHandle, ...)`
**CORRECT**: `xTaskCreatePinnedToCore(Task_SensorRead, ...)` — first param is function pointer, not handle variable.

This bug causes Cache Access Exception crash because RTOS executes an integer address as code.

---

## FreeRTOS Dual-Core Architecture

- **Core 1**: `Task_SensorRead` — 100Hz sensor sampling (TMAG5273 + BNO085), Kalman filtering
- **Core 0**: `Task_Inference` — Edge Impulse inference + `Task_Comms` — WiFi/BLE transmission

Task priorities: SensorRead (3) > Inference (2) > Comms (1)

---

## Edge Impulse Integration

### Data Collection
Enable serial CSV output in `main.cpp`:
```cpp
// Format: 21 features (15 hall + 3 euler + 3 gyro), comma-separated
Serial.printf("%.2f,%.2f,...\n", packet.sensors.hall_xyz[0], ...);
```
Run: `edge-impulse-data-forwarder --baud-rate 115200`

### Inference
After exporting Edge Impulse library to `/lib/`:
1. Build `signal_t` from sliding window buffer (30 frames × 21 features)
2. Call `run_classifier(&signal, &result, false)`
3. Threshold: `result.classification[i].value > 0.85` for local output

---

## Sensor Drivers

### TMAG5273 (Hall Sensors - 5x via TCA9548A)
- Address: `0x70` (TCA), default TMAG addresses
- Mode: `TMAG5273_OPERATING_MODE_CONTINUOUS`
- 12-bit resolution, ±40mT range

### BNO085 (IMU)
- Address: `0x4A`
- Enable: `SH2_GAME_ROTATION_VECTOR` (quaternion→euler), `SH2_GYROSCOPE_CALIBRATED`
- Hardware quaternion fusion via SH-2 protocol

---

## Data Protocol (Protobuf/Nanopb)

`glove_data.proto`:
```protobuf
message GloveData {
    uint32 timestamp = 1;
    repeated float hall_features = 2;  // 15 values (5 sensors × 3 axes)
    repeated float imu_features = 3;   // 6 values (3 euler + 3 gyro)
    repeated float flex_features = 4;  // 5 values (future)
    uint32 l1_gesture_id = 5;
}
```

---

## L1 Model Architecture

1D-CNN + TemporalAttention (Eq.11-13 in paper):
- 3 Conv1d blocks (21→32→64→128 channels)
- TemporalAttention: `e_t = v^T * tanh(W_h * h_t + W_e * e_mean)`
- ~34K parameters, fits in ESP32-S3 PSRAM

---

## Dependencies

### Firmware (platformio.ini)
- `nanopb/Nanopb @ ^0.4.7`
- `adafruit/Adafruit BNO08x @ ^1.2.5`
- `sparkfun/SparkFun TMAG5273 @ ^1.0.0`
- `h2zero/NimBLE-Arduino @ ^1.4.1`
- `links2004/WebSockets @ ^2.4.1`
- Edge Impulse exported library → `/lib/edge-impulse-sdk/`

### Frontend
- React 19 + Vite 6 + TailwindCSS 4
- For 3D rendering: React Three Fiber (R3F) + Zustand

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `source/firmware/src/main.cpp` | FreeRTOS tasks, Edge Impulse integration |
| `source/firmware/lib/Sensors/SensorManager.h` | TMAG5273 + BNO085 drivers |
| `source/firmware/lib/Filters/Kalman1D.h` | 1D Kalman filter |
| `source/scripts/l1_edge_model.py` | PyTorch L1 model definition |
| `source/scripts/l2_inference.py` | ST-GCN inference (needs rebuild from MS-GCN3) |
| `PROJECT_CONTEXT_AND_DECISIONS.md` | Full ADR and analysis |

---

## Development Phases

1. **P0**: Fix critical bugs (xTaskCreatePinnedToCore)
2. **P1**: HAL & drivers (TMAG5273, BNO085, Kalman)
3. **P2**: BLE + Serial CSV (Edge Impulse data collection)
4. **P3**: Edge Impulse training → L1 inference
5. **P4**: WiFi UDP + WebSocket + Tauri dashboard
6. **P5**: L2 ST-GCN (self-built) + NLP + TTS

---

## Performance Targets

| Metric | Target |
|--------|--------|
| L1 inference latency | <3ms |
| L2 inference latency | <20ms |
| End-to-end latency | <100ms (with TTS) |
| L1 accuracy (20 classes) | >90% Top-1 |
| Sensor sampling rate | 100Hz |
| Battery life | >12h (600mAh) |

---

## Hardware Context Required

Before any firmware implementation/debug:
1. Target board: ESP32-S3-DevKitC-1 N16R8
2. Pin mappings: I2C on GPIO 8/9 (SDA/SCL)
3. Connected devices: 5× TMAG5273 (via TCA9548A), 1× BNO085
4. PSRAM: 16MB available for model + buffers

---

## Skill Agents Available

- `esp32-firmware-engineer`: ESP-IDF/Arduino firmware, FreeRTOS, peripherals
- `embedded-systems`: General embedded patterns, RTOS, memory optimization

Invoke with `/agent esp32-firmware-engineer` for firmware-specific tasks.