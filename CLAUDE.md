# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains an **Edge-AI-Powered Data Glove** system for real-time sign language translation and 3D hand rendering. The project implements a dual-tier inference architecture:

- **L1 (Edge)**: ESP32-S3 performs 100Hz sampling, Kalman filtering, and lightweight 1D-CNN gesture recognition (~3ms latency)
- **L2 (Upper Host)**: PC performs ST-GCN complex sign language recognition, NLP grammar correction, and ms-MANO 3D rendering

## Build Commands

### Firmware (ESP32-S3)
```bash
# Build firmware
cd GeminiAIstudioV5-SOP-BLE-WIFI-edge-ai-data-glove-dashboard/firmware
pio run

# Upload to ESP32-S3
pio run -t upload

# Monitor serial output
pio device monitor -b 115200

# Build and upload with monitor
pio run -t upload && pio device monitor
```

### Web Dashboard (Gemini AI Studio App)
```bash
cd GeminiAIstudioV5-SOP-BLE-WIFI-edge-ai-data-glove-dashboard
npm install
npm run dev    # Development server on port 3000
npm run build  # Production build
```

### Python Scripts (AI Pipeline)
```bash
cd GeminiAIstudioV5-SOP-BLE-WIFI-edge-ai-data-glove-dashboard/scripts
python l1_edge_model.py    # Train L1 edge model
python data_collector.py   # Collect training data
python l2_inference.py     # L2 inference pipeline
```

## Key Architecture

### Communication Stack (Dual-Mode)
The system uses three communication channels for different latency/reliability requirements:

1. **WiFi UDP**: 100Hz real-time pose broadcast (lowest latency, packet loss acceptable)
2. **WiFi WebSocket (TCP)**: Recognition results and dynamic features (reliable transmission)
3. **BLE 5.0**: Device provisioning, 20Hz backup link, mobile app direct connection

### FreeRTOS Task Structure (Firmware)
```
Core 0:
  - Task_SensorRead (Priority 3, 10ms period): TMAG5273 + BNO085 data acquisition
  - Kalman filtering applied in SensorManager

Core 1:
  - Task_Inference (Priority 2): TFLite Micro inference on sliding window (30 frames)
  - Task_Comms (Priority 1): UDP + WebSocket + BLE communication
```

### Sensor Hardware
- **5x TMAG5273A1**: Hall effect sensors for MCP joints (managed via TCA9548A I2C multiplexer)
- **1x BNO085**: 9-axis IMU on wrist (outputs hardware-fused quaternion)
- **I2C pins**: SDA=GPIO8, SCL=GPIO9 (ESP32-S3)

### Data Protocol (Nanopb Protobuf)
```proto
message GloveData {
    uint32 timestamp = 1;
    repeated float hall_features = 2;  // 15 values (5 sensors × 3 axes)
    repeated float imu_features = 3;   // 6 values (3 euler + 3 gyro)
    repeated float flex_features = 4;  // 5 values (future flex sensors)
    uint32 l1_gesture_id = 5;          // L1 inference result
}
```

### TensorFlow Lite Micro Integration
- Model architecture: 1D-CNN + Temporal Attention (128 hidden dim, 8 attention heads)
- Input: 30-frame sliding window × 21 features (15 hall + 3 euler + 3 gyro)
- Output: 46 gesture classes with INT8 quantization
- Threshold: Confidence > 0.85 (INT8 value 108) triggers L1 gesture detection
- Tensor arena: 64KB allocated

### 3D Rendering Pipeline (Unity)
- UDP receiver parses GloveData protobuf
- ManoController maps sensor data to ms-MANO parametric hand model
- XR Hands Package for Unity 2022 LTS integration

## Development Workflow

### Adding New Sensors
1. Update `SensorManager.h` to include new sensor class
2. Modify `SensorData` struct to include new data fields
3. Update `glove_data.proto` and regenerate with nanopb: `nanopb_generator.py glove_data.proto`
4. Adjust sliding window buffer size in `main.cpp` if feature count changes

### Training New L1 Model
1. Collect data with `scripts/data_collector.py`
2. Modify `L1EdgeModel` class in `scripts/l1_edge_model.py` for architecture changes
3. Export to TFLite INT8 format
4. Convert to `model_data.h` C array for firmware inclusion
5. Rebuild firmware with new model

### Modifying Communication Protocol
- Protobuf changes require regeneration of `glove_data.pb.c` and `glove_data.pb.h`
- UDP port default: 8888
- WebSocket port default: 81
- BLE service UUID defined in `BLEManager.h`

## Key Files Reference

| File | Purpose |
|------|---------|
| `firmware/src/main.cpp` | FreeRTOS task orchestration, TFLite setup |
| `firmware/lib/Sensors/SensorManager.h` | TMAG5273 + BNO085 driver, Kalman filtering |
| `firmware/lib/Comms/glove_data.proto` | Protobuf message definition |
| `firmware/src/model_data.h` | TFLite model weights (INT8 quantized) |
| `scripts/l1_edge_model.py` | PyTorch model definition and training |
| `unity/UDPReceiver.cs` | Unity UDP socket handler |
| `unity/ManoController.cs` | Sensor → MANO parameter mapping |

## I2C Multiplexer Usage (TCA9548A)

Channel selection for TMAG5273 sensors:
```cpp
void tcaSelect(uint8_t channel) {
    Wire.beginTransmission(0x70);  // TCA9548A address
    Wire.write(1 << channel);     // Select channel 0-4
    Wire.endTransmission();
}
```

Each TMAG5273 requires channel selection before I2C communication.

## Performance Targets

- L1 inference latency: < 3ms
- End-to-end latency: < 50ms (sensor to 3D render)
- Recognition accuracy: > 95% (46 gesture classes)
- Sample rate: 100Hz (10ms interval)
- Battery life: > 12 hours (600mAh)