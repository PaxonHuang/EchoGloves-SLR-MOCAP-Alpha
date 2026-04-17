+-----------------------------------------------------------------------+
|                                                                       |
| **Claude Code Prompt**                                               |
| **Engineering Handbook**                                              |
|                                                                       |
|                                                                       |
| Edge AI Data Glove Project                                            |
|                                                                       |
| Phased Executable Prompt Document                                     |
|                                                                       |
| **Companion Document:** SOP SPEC PLAN v2.0                            |
|                                                                       |
| **MCU:** ESP32-S3-DevKitC-1 N16R8                                    |
|                                                                       |
| **AI Framework:** Edge Impulse / TFLite Micro / PyTorch               |
|                                                                       |
| **Date:** 2026-04-16                                                  |
|                                                                       |
| Version 1.0 - Internal Use Only                                       |
+=======================================================================+

**Table of Contents**

| **#** | **Section**                                   | **Prompts**       |
|-------|-----------------------------------------------|-------------------|
| **1** | Project Initialization & Environment Setup    | P0.1 -- P0.2      |
| **2** | Phase 1: HAL & Driver Layer                   | P1.1 -- P1.4      |
| **3** | Phase 2: Signal Processing                    | P2.1 -- P2.2      |
| **4** | Phase 3: L1 Edge Inference                    | P3.1 -- P3.3      |
| **5** | Phase 4: Communication Protocol               | P4.1 -- P4.2      |
| **6** | Phase 5: L2 ST-GCN Inference                  | P5.1 -- P5.3      |
| **7** | Phase 6-7: NLP Grammar Correction & Rendering | P6.1 -- P7.1      |
| **8** | Phase 8: Integration Testing                  | P8.1              |
| **9** | Bug Fix Dedicated Prompts                     | P9.1 -- P9.2      |

---

## 1. Project Initialization & Environment Setup

This section contains all prompts for the project initialization phase, covering the creation and configuration of the PlatformIO embedded project and the Python host (upper computer) project. All tasks in this section must be completed before starting any hardware driver or algorithm development to ensure the development environment is correctly set up.

---

### P0.1 - Initialize PlatformIO Project

**Context:** *The ESP32-S3-DevKitC-1 N16R8 version requires correct configuration of PSRAM and memory mode; otherwise memory-intensive tasks such as TensorFlow Lite Micro cannot run. This prompt ensures the project skeleton and build environment are correctly established in one shot.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please initialize a PlatformIO project targeting the ESP32-S3-DevKitC-1 (N16R8, 16MB PSRAM).

Requirements:

1. In platformio.ini, configure:

   - platform = espressif32 @ ^6.5.0
   - board = esp32-s3-devkitc-1-n8r8 (Note: our board is the N16R8 version)
   - framework = arduino
   - board_build.psram = enable (enable external PSRAM)
   - board_build.arduino.memory_type = qio_opi (PSRAM mode)
   - monitor_speed = 115200
   - build_flags =
     -DBOARD_HAS_PSRAM
     -DCORE_DEBUG_LEVEL=3

2. Create the following directory structure:

   - src/ (main source code)
   - lib/Sensors/ (sensor drivers)
   - lib/Filters/ (filtering algorithms)
   - lib/Comms/ (communication modules)
   - lib/Models/ (AI models)
   - include/ (header files)
   - test/ (tests)

3. Verify that compilation passes with no errors.
```

**Expected Output:**

- platformio.ini configuration file with complete ESP32-S3 N16R8 PSRAM configuration
- Complete directory structure with all lib/ subdirectories created
- Compilation output: 0 errors, 0 warnings (or only known ignorable warnings)

**Acceptance Criteria:**

- [ ] platformio.ini contains `board_build.psram = enable`
- [ ] Directory structure includes lib/Sensors, lib/Filters, lib/Comms, lib/Models, include, test
- [ ] `pio run` compiles successfully

---

### P0.2 - Initialize Python Host Project

**Context:** *The host (upper computer) is responsible for data collection, L2 ST-GCN inference, NLP grammar correction, and TTS speech synthesis. A standardized directory structure and dependency management must be established upfront to avoid dependency conflicts later. Python version 3.9+ is required for compatibility with PyTorch 2.0 and edge-tts.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please initialize the Python host project, creating the following structure and dependencies:

1. Create project directory structure:

   - glove_upper/ (project root)
     - src/
       - sensors/ (sensor data processing)
       - models/ (ST-GCN models)
       - nlp/ (grammar correction)
       - tts/ (speech synthesis)
       - rendering/ (rendering interface)
       - utils/ (utility functions)
     - data/ (data storage)
     - configs/ (configuration files)
     - scripts/ (standalone scripts)

2. Create requirements.txt:

   torch>=2.0.0
   torchvision>=0.15.0
   numpy>=1.24.0
   protobuf>=4.24.0
   edge-tts>=6.1.0
   asyncio
   websockets>=11.0
   pyserial>=3.5

3. Create pyproject.toml configuring Python 3.9+ compatibility.
```

**Expected Output:**

- Complete project directory structure
- requirements.txt with all core dependencies and version constraints
- pyproject.toml configuring Python 3.9+ and project metadata
- `__init__.py` files in each sub-package

**Acceptance Criteria:**

- [ ] All directories exist and contain `__init__.py`
- [ ] `pip install -r requirements.txt` succeeds
- [ ] `python -c "import torch; print(torch.__version__)"` outputs normally

---

## 2. Phase 1: HAL & Driver Layer

This section covers all low-level hardware driver development, including the I2C multiplexer, 3D Hall sensors, 9-axis IMU, and FreeRTOS dual-core task scheduling. These drivers are the data foundation of the entire system; any bug will propagate to upper layers. Pay special attention to the known FreeRTOS parameter ordering bug documented in P1.4.

> **⚠️ Known Bug Warning: FreeRTOS xTaskCreatePinnedToCore Parameter Order**
>
> In the original code, the function pointer parameter and handle parameter were swapped. Incorrect usage: the first parameter passed the handle variable `TaskSensorReadHandle` instead of the function pointer `Task_SensorRead`. This bug causes compilation to pass but runtime crashes or tasks not executing. P9.1 provides the complete fix prompt.

---

### P1.1 - TCA9548A I2C Multiplexer Driver

**Context:** *The data glove uses 5 TMAG5273 Hall sensors that share the same I2C bus (same address). They must be accessed one by one by switching channels through the TCA9548A multiplexer. If the multiplexer driver has I2C timing issues, all sensor data collection will fail.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement the TCA9548A I2C multiplexer driver, file path: lib/Sensors/TCA9548A.h

Requirements:

1. Class name: TCA9548A

2. Constructor: TCA9548A(TwoWire *wire, uint8_t addr = 0x70)

3. Core methods:

   - bool begin() - Initialize, verify I2C communication
   - bool selectChannel(uint8_t ch) - Select channel 0-7
   - bool disableAll() - Disable all channels

4. I2C speed: 400kHz (Wire.setClock(400000))

5. Error handling: Check return value after every I2C operation

6. Implement all methods in the .cpp file

7. Create unit test test_tca9548a.cpp in the test/ directory

Known constraints:

- TCA9548A address is 0x70
- selectChannel() must be called before each TMAG5273 read
- Main bus pull-up resistors: 2.2kOhm; sub-channel pull-up resistors: 4.7kOhm
```

**Expected Output:**

- lib/Sensors/TCA9548A.h - Class declaration
- lib/Sensors/TCA9548A.cpp - Method implementations
- test/test_tca9548a.cpp - Unit test framework

**Acceptance Criteria:**

- [ ] begin() returns true, I2C communication verification passes
- [ ] selectChannel(0-7) all return true
- [ ] disableAll() correctly disables all channels
- [ ] I2C speed is set to 400kHz
- [ ] Error checking exists after every I2C operation

---

### P1.2 - TMAG5273 3D Hall Sensor Driver

**Context:** *The TMAG5273A1 is the core position sensor, with one installed on each finger (5 total). It calculates joint bending angles by detecting magnetic field changes from N52 magnets. With 12-bit resolution and ±40mT range, it requires 32x averaging to reduce noise. Sensors are connected through TCA9548A channels 0-4.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement the TMAG5273A1 3D Hall sensor driver, file path: lib/Sensors/TMG5273.h

Requirements:

1. Class name: TMAG5273

2. Constructor: TMAG5273(TwoWire *wire, uint8_t mux_channel, TCA9548A *mux)

3. Core methods:

   - bool begin() - Initialize sensor
     - Set CONV_MODE to trigger mode (0x01)
     - Set CONFIG register: sensing range ±40mT
     - Set AVG to 32x averaging (noise reduction)
   - bool readXYZ(float &x, float &y, float &z) - Read 12-bit raw data and convert to mT
   - bool triggerConversion() - Trigger one ADC conversion

4. Register definitions:

   - DEVICE_ID = 0x00 (expected value 0x11)
   - X_MSB = 0x01, X_LSB = 0x02
   - Y_MSB = 0x03, Y_LSB = 0x04
   - Z_MSB = 0x05, Z_LSB = 0x06
   - CONV_MODE = 0x0D
   - CONFIG = 0x0E

5. Data conversion: 12-bit signed, LSB = 0.048mT

6. Critical: Must select the correct channel via TCA9548A before each read

Known constraints:

- 5 sensors connected to TCA9548A channels 0-4 respectively
- Magnets are N52 4x2mm disks, mounted on proximal phalanges
- Sensors are mounted on the dorsal side of MCP joints
```

**Expected Output:**

- lib/Sensors/TMG5273.h - Class declaration with register constants
- lib/Sensors/TMG5273.cpp - Complete driver implementation
- readXYZ() correctly performs 12-bit signed to mT conversion
- mux->selectChannel() is called before each read

**Acceptance Criteria:**

- [ ] begin() passes DEVICE_ID verification (expected value 0x11)
- [ ] CONV_MODE set to trigger mode, AVG set to 32x averaging
- [ ] readXYZ() returns 3-axis magnetic field values (mT)
- [ ] 5 instances correspond to channels 0-4 respectively with no interference

---

### P1.3 - BNO085 9-Axis IMU Driver

**Context:** *The BNO085 provides overall hand posture information (quaternion + gyroscope), mounted at the center of the back of the hand. It uses Hillcrest Labs' SH-2 sensor fusion algorithm, outputting Game Rotation Vector (not affected by magnetic interference, suitable for indoor use). The SH-2 protocol must be correctly implemented to obtain data.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement the BNO085 9-axis IMU driver, file path: lib/Sensors/BNO085.h

Requirements:

1. Class name: BNO085

2. Constructor: BNO085(TwoWire *wire, uint8_t addr = 0x4A)

3. Core methods:

   - bool begin() - Initialize
     - Soft reset
     - Enable SH2_GAME_ROTATION_VECTOR report (100Hz)
     - Enable SH2_GYROSCOPE_CALIBRATED report (100Hz)
   - bool getQuaternion(float &w, float &x, float &y, float &z)
   - bool getGyro(float &x, float &y, float &z)
   - bool getEuler(float &roll, float &pitch, float &yaw)

4. SH-2 protocol implementation:

   - Report header parsing
   - Product ID query
   - Feature report enable

5. Quaternion to Euler angle conversion (built-in method)

Known constraints:

- BNO085 mounted at center of back of hand, must be parallel to palm plane
- Uses hardware sensor fusion, outputs Game Rotation Vector (no magnetic interference)
- I2C address: 0x4A (SDO connected to GND)
- Must have 0.1uF + 10uF decoupling capacitors
```

**Expected Output:**

- lib/Sensors/BNO085.h - Class declaration
- lib/Sensors/BNO085.cpp - SH-2 protocol implementation
- getQuaternion() returns normalized quaternion (w, x, y, z)
- getEuler() returns (roll, pitch, yaw) in degrees

**Acceptance Criteria:**

- [ ] begin() successfully soft-resets and enables Game Rotation Vector report
- [ ] SH-2 report header parsing is correct (2-byte header + variable-length payload)
- [ ] getQuaternion() return values satisfy w² + x² + y² + z² ≈ 1.0
- [ ] getEuler() output ranges: roll[-180,180], pitch[-90,90], yaw[-180,180]

---

### P1.4 - FreeRTOS Dual-Core Task Scheduling (Including Known Bug Fix)

**Context:** *The ESP32-S3 has dual cores (Core 0: Protocol CPU, Core 1: Application CPU). This prompt leverages FreeRTOS to place sensor acquisition (100Hz real-time requirement) on Core 1, and inference and communication on Core 0, achieving true parallel processing. ⚠️ The original code has an xTaskCreatePinnedToCore parameter ordering bug that must be fixed.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a FreeRTOS-based dual-core task scheduling framework, file path: src/main.cpp

⚠️ Known Bug Fix: The original code xTaskCreatePinnedToCore parameter order is wrong!

Incorrect usage: xTaskCreatePinnedToCore(TaskSensorReadHandle, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1)
Correct usage:   xTaskCreatePinnedToCore(Task_SensorRead, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1)

Parameter order: (function pointer, task name, stack size, parameter, priority, handle pointer, core ID)

Requirements:

1. Define three FreeRTOS tasks:

   - Task_SensorRead (Core 1, priority 3, stack 4096):
     100Hz sampling loop, read all sensor data
     Use vTaskDelayUntil to ensure precise 100Hz timing

   - Task_Inference (Core 0, priority 2, stack 8192):
     Receive sensor data from queue
     Execute L1 inference (Edge Impulse or TFLite Micro)

   - Task_Comms (Core 0, priority 1, stack 8192):
     BLE/WiFi communication
     Protobuf serialization and transmission

2. FreeRTOS queues:

   - inferenceQueue: sensor data -> inference task (depth 10)
   - dataQueue: inference results -> communication task (depth 10)

3. Data structures:

   - FullDataPacket: timestamp + SensorData(hall_xyz[15] + euler[3] + gyro[3]) + flex[5]
   - InferenceResult: FullDataPacket + gesture_id + confidence

4. Create tasks and queues in setup().
```

**Expected Output:**

- src/main.cpp - Contains correct task creation code (bug fixed)
- Data structures FullDataPacket and InferenceResult definitions
- Three FreeRTOS task function implementations
- Two FreeRTOS queue creation and usage

**Acceptance Criteria:**

- [ ] xTaskCreatePinnedToCore first parameter is the function pointer (not the handle variable)
- [ ] Task_SensorRead uses vTaskDelayUntil for precise 100Hz timing
- [ ] inferenceQueue and dataQueue depth is 10
- [ ] Compilation passes with no errors
- [ ] (See P9.1 for detailed fix prompt)

---

## 3. Phase 2: Signal Processing

This section includes filtering of raw sensor signals and data collection/annotation tools. The Kalman filter is used to reduce noise from Hall sensors (12-bit resolution + magnetic field interference). The data collection tool is used to build training datasets, which is the foundation for all subsequent AI model training.

---

### P2.1 - 1D Kalman Filter

**Context:** *Although the TMAG5273 Hall sensor is configured with 32x averaging, magnetic signals are still susceptible to environmental interference (motors, metal, etc.). The Kalman filter provides optimal estimation given known process noise and measurement noise models. Each of the 15 Hall signal channels needs an independent filter instance.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a 1D Kalman filter class, file path: lib/Filters/KalmanFilter1D.h

Requirements:

1. Class name: KalmanFilter1D

2. Constructor: KalmanFilter1D(float process_noise = 0.01, float measurement_noise = 0.1)

3. Core methods:

   - float update(float measurement) - Input measurement value, return filtered estimate
   - void reset() - Reset filter state

4. Internal state: estimate x, estimation error P, Kalman gain K

5. Instantiate one KalmanFilter1D for each of the 15 Hall signal channels

Key parameter tuning guidance:

- process_noise (Q): Inherent sensor noise level, starting suggestion 0.01
- measurement_noise (R): Measurement uncertainty, starting suggestion 0.1
- If filter response is too slow, increase Q or decrease R
- If output jitter is too large, decrease Q or increase R
```

**Expected Output:**

- lib/Filters/KalmanFilter1D.h - Template class implementation (header-only)
- Contains complete state prediction and update equations
- Constructor supports custom Q and R parameters

**Acceptance Criteria:**

- [ ] update() returns the filtered estimate value
- [ ] reset() can clear internal state
- [ ] First call to update() correctly initializes (does not use zero prior)
- [ ] 15 independent instances can work in parallel without interference
- [ ] Output is smooth with Q=0.01, R=0.1 default parameters

---

### P2.2 - Data Collection & Annotation Tool

**Context:** *AI model training requires large amounts of annotated data. This tool supports real-time reception of sensor data via BLE or UDP, records gesture labels, and automatically saves in NPY format (compatible with Edge Impulse CSV format). Each gesture is recommended to have 100-200 recorded samples to ensure model generalization.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a data collection Python script, file path: scripts/data_collector.py

Requirements:

1. Support both BLE serial and UDP data sources

2. Features:

   - Real-time sensor data waveform display (using matplotlib or rich)
   - Record data for specified gesture labels
   - Auto-save in NPY format: shape=(N, 30, 21) (number of samples, window length, feature count)
   - Recommended 100-200 samples per gesture

3. Gesture label management:

   - Predefined 46-class gesture label list
   - Support custom labels
   - Auto-organize file directories by label

4. Data augmentation (optional):

   - Random time shift ±5 frames
   - Gaussian noise sigma=0.01
   - Time masking (randomly zero out 10% of frames)

5. Use edge-impulse-data-forwarder compatible CSV format as alternative output
```

**Expected Output:**

- scripts/data_collector.py - Complete data collection script
- 46 predefined gesture labels
- Real-time waveform display interface
- Dual format output: NPY and CSV
- Data augmentation pipeline

**Acceptance Criteria:**

- [ ] BLE and UDP data sources can be switched
- [ ] Recorded NPY files have shape = (N, 30, 21)
- [ ] CSV output is compatible with edge-impulse-data-forwarder
- [ ] Data augmentation feature can be toggled on/off
- [ ] Label management supports add/delete/list

---

## 4. Phase 3: L1 Edge Inference

L1 Edge Inference is the real-time gesture recognition layer of the data glove, running on the ESP32-S3. Two technical routes are provided: the MVP route uses Edge Impulse (rapid prototype verification), and the production route uses PyTorch to train a 1D-CNN+Attention model quantized to TFLite INT8 for MCU deployment. Inference latency requirement: <3ms.

> **ℹ️ Dual Route Strategy**
>
> The Edge Impulse route (P3.1) is suitable for quickly verifying feasibility; a usable L1 model can be obtained within 2-3 days. The PyTorch route (P3.2 + P3.3) provides higher customization capability and quantization precision control. It is recommended to first use the EI route to verify data quality, then switch to the PyTorch route to optimize accuracy.

---

### P3.1 - Edge Impulse Data Collection & Training (MVP Route)

**Context:** *Edge Impulse provides an end-to-end edge AI development platform, seamlessly handling data collection, model training, and C++ library export. For the MVP phase, this is the fastest route: no need to manually implement a training pipeline; all work is done in the browser. The output is an Arduino Library that PlatformIO can directly reference.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please write an Edge Impulse data collection and model training guide:

I. Data Collection:

1. Install edge-impulse-data-forwarder:
   pip install edge-impulse-data-forwarder

2. Implement serial CSV output in ESP32 firmware:
   - Baud rate: 115200
   - Format: "hall_1,hall_2,...,hall_15,euler_r,euler_p,euler_y,gyro_x,gyro_y,gyro_z\n"
   - Frequency: 100Hz
   - One line per time step

3. Run the forwarder to connect to serial port and create a project

4. Record data for each gesture category in EI Studio (>=100 samples per category)

II. Model Training (EI Studio):

1. Create Impulse:
   - Input: 21 sensor features
   - Window size: 3000ms (30 frames x 100ms)
   - Window increment: 100ms

2. Processing blocks:
   - Raw Data (use raw data directly)
   - Or add Filter (low-pass filter preprocessing)

3. Learning block:
   - Classification (Keras)
   - Recommendation: 1D CNN + Attention
   - Training cycles: 100-200 epochs
   - Learning rate: 0.001 (Adam)

4. Enable EON Tuner optimization

III. Deployment Export:

1. Export as Arduino Library

2. Extract to the PlatformIO project's lib/ directory

3. In main.cpp:
   #include <edge_impulse_inferencing.h>
   Implement signal_t callback and run_classifier() call
```

**Expected Output:**

- Complete Edge Impulse usage guide document
- ESP32-side serial CSV output code snippets
- EI Studio configuration parameter checklist
- C++ integration code template

**Acceptance Criteria:**

- [ ] EI project has >=100 samples per gesture category
- [ ] Impulse window size 3000ms / increment 100ms
- [ ] Model training accuracy >90% (validation set)
- [ ] Arduino Library export successful and integrated into PlatformIO
- [ ] ESP32-side inference latency <5ms

---

### P3.2 - PyTorch 1D-CNN+Attention Model Training

**Context:** *When the Edge Impulse model accuracy cannot meet requirements, using a custom PyTorch model provides higher accuracy and more flexible architecture control. 1D-CNN extracts local temporal features, and the Attention mechanism focuses on key time steps. After training, QAT (Quantization-Aware Training) is used to convert to INT8 format to fit the ESP32-S3's compute resources.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a PyTorch 1D-CNN+Attention model training script, file path: src/models/l1_train.py

Requirements:

1. Model architecture L1EdgeModel:

   - Input: (Batch, 30, 21) - 30 frames x 21 features
   - Block1: Conv1d(21->32, kernel=5, padding=2) + BatchNorm1d + ReLU + MaxPool(2)
   - Block2: Conv1d(32->64, kernel=3, padding=1) + BatchNorm1d + ReLU + MaxPool(2)
   - Block3: Conv1d(64->128, kernel=3, padding=1) + BatchNorm1d + ReLU
   - TemporalAttention(128):
     * W_h: Linear(128, 32)
     * W_e: Linear(128, 32)
     * v: Linear(32, 1)
     * score = v(tanh(W_h(h) + W_e(e_mean)))
     * alpha = softmax(score)
     * context = sum(alpha * h)
   - FC: Linear(128, num_classes)

2. Training configuration:

   - Optimizer: AdamW(lr=1e-3, weight_decay=1e-4)
   - Scheduler: CosineAnnealingLR(T_max=200)
   - Loss: CrossEntropyLoss(label_smoothing=0.1)
   - Batch Size: 64
   - Epochs: 200 (early stopping patience=20)
   - Data augmentation: random time shift, Gaussian noise, Mixup(alpha=0.2)

3. INT8 quantization export:

   - Use torch.quantization.prepare_qat
   - QAT training for 3-5 epochs
   - Convert to TFLite INT8
   - Use repr_dataset for calibration (100-500 samples)
   - Verify accuracy degradation after quantization <2%

4. Generate model_data.h:

   - xxd -i model_quant.tflite > model_data.h
   - Verify file size <100KB

Acceptance criteria:

- Training set accuracy >98%
- Validation set accuracy >92%
- Post-quantization accuracy >90%
- Inference latency <3ms (ESP32-S3)
```

**Expected Output:**

- src/models/l1_train.py - Complete training script
- L1EdgeModel class definition (including TemporalAttention)
- Training loop (with early stopping, learning rate scheduling, data augmentation)
- QAT quantization pipeline
- TFLite INT8 export script
- model_data.h generation command

**Acceptance Criteria:**

- [ ] Training set accuracy >98%, validation set accuracy >92%
- [ ] INT8 quantized accuracy >90% (accuracy degradation <2%)
- [ ] model_quant.tflite file <100KB
- [ ] model_data.h correctly generated and compilable by ESP32
- [ ] Inference latency on ESP32-S3 <3ms

---

### P3.3 - TFLite Micro Integration (C++ Side)

**Context:** *Integrating the PyTorch-exported TFLite INT8 model into the ESP32-S3 firmware. TFLite Micro is Google's lightweight inference engine designed for microcontrollers, supporting ESP32-S3 AI vector instruction acceleration. The key challenge is memory management -- the Arena must be allocated to PSRAM to accommodate 80KB of runtime memory.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement TFLite Micro inference integration code, file path: src/TFLiteInference.h

Requirements:

1. Class name: TFLiteInference

2. Initialization:

   - Load model data from model_data.h
   - Create AllOpsResolver
   - Create Interpreter (arena_size=50KB, using PSRAM)

3. Inference method:

   - int8_t runInference(float *input_window, int window_size, float *confidence)
   - Input: 30x21 = 630 float features
   - Output: gesture_id and confidence

4. Performance optimization:

   - Use ESP32-S3 AI vector instructions
   - Allocate inference Arena to PSRAM (CONFIG_SPIRAM)
   - Stack size >=8192 bytes

5. Integration into FreeRTOS Task_Inference:

   - Get data from queue
   - Fill sliding window
   - Call runInference
   - Enqueue results

Key constraints:

- Use #include "tensorflow/lite/micro/all_ops_resolver.h"
- Use #include "tensorflow/lite/micro/micro_interpreter.h"
- Arena size recommended 80KB (including model 38KB + runtime)
```

**Expected Output:**

- src/TFLiteInference.h - Class declaration
- src/TFLiteInference.cpp - TFLite Micro integration implementation
- Code to allocate Arena to PSRAM
- Integration code with FreeRTOS Task_Inference

**Acceptance Criteria:**

- [ ] Arena successfully allocated to PSRAM (not internal SRAM)
- [ ] runInference() takes 630 floats as input, outputs gesture_id and confidence
- [ ] Inference latency <3ms (using ESP32-S3 AI instructions)
- [ ] Compiled firmware size increase <200KB
- [ ] Correctly integrated with Task_Inference, queue data flow non-blocking

---

## 5. Phase 4: Communication Protocol

The communication layer is responsible for transmitting sensor data and inference results from the ESP32 to the host. Protobuf serialization ensures cross-platform compatibility. BLE 5.0 provides low-power mobile connectivity (network provisioning + data push). WiFi/UDP provides high-bandwidth host connectivity (raw data stream + L2 inference).

---

### P4.1 - Protobuf Definition & Nanopb Generation

**Context:** *Protobuf provides efficient binary serialization, reducing transmitted data volume by approximately 60% compared to JSON. The ESP32 side uses Nanopb (minimalist C implementation); the Python side uses the standard protobuf library. A unified .proto definition ensures consistent data formats on both sides, avoiding manual parsing errors.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement Protobuf data frame definition and Nanopb generation:

1. Edit glove_data.proto:

syntax = "proto3";

message GloveData {
    uint32 timestamp = 1;
    repeated float hall_features = 2;   // 15 Hall channels (mT)
    repeated float imu_features = 3;    // 6 IMU channels (3 Euler angles deg + 3 gyroscope dps)
    repeated float flex_features = 4;   // 5 flex channels (reserved)
    uint32 l1_gesture_id = 5;
    uint32 l1_confidence_x100 = 6;     // confidence * 100 (0-10000)
}

2. Install Nanopb:
   pip install protobuf nanopb
   Generate C code: nanopb/generator/protoc --nanopb_out=. glove_data.proto

3. Integration on ESP32 side:
   #include "glove_data.pb.h"
   Use pb_encode for serialization
   Use pb_ostream_from_buffer to create output stream

4. On Python side:
   Use protoc --python_out=. glove_data.proto
   pip install protobuf
   glove_data_pb2.GloveData() for parsing
```

**Expected Output:**

- glove_data.proto - Protobuf message definition
- glove_data.pb.h / glove_data.pb.c - Nanopb-generated C code
- glove_data_pb2.py - Python protobuf code
- ESP32-side serialization example code
- Python-side deserialization example code

**Acceptance Criteria:**

- [ ] GloveData message contains all 6 fields
- [ ] Nanopb-generated code compiles on ESP32 side
- [ ] Python side can correctly parse Protobuf data sent by ESP32
- [ ] Single frame data size <200 bytes
- [ ] Serialization/deserialization round-trip test passes

---

### P4.2 - BLE 5.0 GATT Service Implementation

**Context:** *BLE is used for mobile connectivity and WiFi provisioning. The GATT service defines the data exchange interface -- the Sensor Data Characteristic is for pushing recognition results, and the Config Characteristic is for receiving WiFi credentials. BLE MTU negotiation can increase throughput to 512 bytes/frame, meeting Protobuf data transmission requirements.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement BLE 5.0 GATT service, file path: lib/Comms/BLEManager.h

Requirements:

1. Class name: BLEManager

2. GATT service definition:

   - Custom Service UUID: 0x181A
   - Sensor Data Characteristic (Notify): custom UUID
     - Data format: Protobuf-serialized GloveData
     - Maximum MTU: 512 bytes (requires MTU negotiation)
   - Config Characteristic (Read/Write): custom UUID
     - Used for network provisioning (WiFi SSID/Password delivery)

3. Provisioning flow:

   - Mobile app writes WiFi credentials via Config Characteristic
   - Format: "SSID:PASSWORD\n"
   - ESP32 calls WiFi.begin() upon receipt
   - Reply "OK" via Notify after successful connection

4. Data transmission:

   - L1 recognition results sent via Notify (on each gesture change)
   - Raw sensor data sent every 5 frames (20Hz BLE throttle)

5. Security:

   - Provisioning data only valid during initial BLE connection
   - Clear WiFi credential cache in BLE after successful provisioning
```

**Expected Output:**

- lib/Comms/BLEManager.h - Class declaration
- lib/Comms/BLEManager.cpp - GATT service implementation
- BLE server callback registration
- MTU negotiation logic
- WiFi provisioning flow code

**Acceptance Criteria:**

- [ ] BLE service successfully advertises and connects
- [ ] MTU negotiation succeeds (>=247 bytes, target 512)
- [ ] WiFi connection succeeds after writing credentials to Config Characteristic
- [ ] Provisioning credentials are not persisted in BLE callbacks
- [ ] Sensor Data Notify sends Protobuf data normally

---

## 6. Phase 5: L2 ST-GCN Inference

L2 Inference is the host-side deep gesture recognition layer, using Spatial-Temporal Graph Convolutional Network (ST-GCN) to process complete gesture action sequences. Unlike L1's single-frame classification, L2 can capture spatial relationships between fingers and temporal dynamics of actions, with a higher theoretical ceiling. The key challenge is pseudo-skeleton mapping -- mapping 21-dimensional sensor features to 2D coordinates of 21 hand keypoints.

> **⚠️ Critical Bug: Original ST-GCN Implementation is Fake**
>
> The original paper's l2_inference.py STGCNModel only uses nn.Linear(42*30, 46) -- this is just a single fully-connected layer with no graph convolution structure at all! It loses all spatial (hand skeleton topology) and temporal (action sequence) information. P5.2 and P9.2 provide complete replacement solutions.

---

### P5.1 - Pseudo-Skeleton Mapping & Data Preprocessing

**Context:** *ST-GCN's input is skeleton sequences (keypoint coordinates x time), but the data glove can only provide 21-dimensional sensor features (15 Hall + 6 IMU). Pseudo-skeleton mapping learns a linear transformation matrix that projects sensor features into the 2D coordinate space of MediaPipe Hand Landmarks' 21 keypoints, enabling ST-GCN to process them directly.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement Pseudo-skeleton Mapping and data preprocessing pipeline:

1. PseudoSkeletonMapper class:

   - Input: 21-dimensional sensor features (15 Hall + 6 IMU)
   - Output: 2D coordinates of 21 hand keypoints (referencing MediaPipe Hand Landmarks)
   - Implementation approach:
     * Learn mapping matrix W_map (21x21 linear transformation) during training
     * Each Hall sensor corresponds to one MCP joint position
     * Euler angles used to determine palm orientation and overall posture
     * Final output shape: (Batch, Time=30, Landmarks=21, Coords=2)

2. Hand skeleton graph definition:

   - 21 keypoints (referencing MediaPipe):
     0:WRIST, 1:THUMB_CMC, 2:THUMB_MCP, 3:THUMB_IP, 4:THUMB_TIP
     5:INDEX_FINGER_MCP, 6:INDEX_FINGER_PIP, 7:INDEX_FINGER_DIP, 8:INDEX_FINGER_TIP
     9:MIDDLE_FINGER_MCP, 10:MIDDLE_FINGER_PIP, 11:MIDDLE_FINGER_DIP, 12:MIDDLE_FINGER_TIP
     13:RING_FINGER_MCP, 14:RING_FINGER_PIP, 15:RING_FINGER_DIP, 16:RING_FINGER_TIP
     17:PINKY_MCP, 18:PINKY_PIP, 19:PINKY_DIP, 20:PINKY_TIP
   - Adjacency edges: WRIST -> each MCP -> each PIP -> each DIP -> each TIP (bone -> bone -> bone -> tip)

3. Adjacency matrix A construction:

   - Based on hand anatomical structure
   - Includes self-loops (diagonal=1)
   - Normalization: D^(-1/2) * A * D^(-1/2)

4. ST-GCN input data format:

   - (Batch, Time=30, Landmarks=21, Coords=2)
   - Corresponds to skeleton sequences in video action recognition
```

**Expected Output:**

- src/models/pseudo_skeleton.py - PseudoSkeletonMapper class
- Hand skeleton graph definition (21 keypoints + adjacency matrix)
- Adjacency matrix A and its normalized version A_norm
- Data preprocessing pipeline: sensor features -> pseudo-skeleton coordinates

**Acceptance Criteria:**

- [ ] Mapping matrix W_map dimensions are correct (21x21)
- [ ] Output shape = (B, 30, 21, 2)
- [ ] Adjacency matrix contains 20 edges (correct hand anatomical structure)
- [ ] A_norm = D^(-1/2) @ A @ D^(-1/2) calculated correctly
- [ ] Pseudo-skeleton coordinate distribution matches real hand keypoint distribution

---

### P5.2 - ST-GCN Model Implementation (from MS-GCN3 Paper)

**Context:** *This is the core model for L2 inference. ST-GCN captures spatial relationships between fingers (e.g., coordinated movement of thumb and index finger) through graph convolution, and captures temporal dynamics of actions (e.g., continuous change from fist to open hand) through temporal convolution. The original paper's fake implementation (single fully-connected layer) must be completely replaced with a real graph convolutional network.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a complete ST-GCN model based on the MS-GCN3 paper, file path: src/models/stgcn.py

Requirements:

⚠️ Critical: This is not just nn.Linear! You must implement real graph convolution structure.

The original paper's l2_inference.py STGCNModel only uses nn.Linear(42*30, 46),
which is not ST-GCN at all. It must be completely rewritten in this implementation.

1. Graph Convolution Layer (GraphConv):

   - Spatial domain convolution: Feature aggregation based on adjacency matrix A
   - X' = A_norm @ X @ W_spatial
   - W_spatial: Learnable spatial weight matrix

2. Temporal Convolution Layer (TemporalConv):

   - Uses 1D convolution (Temporal Convolutional Network)
   - kernel_size=9, padding=4
   - Or use dilated convolution (atrous convolution)

3. ST-Conv Block:

   - Structure: BN -> SpatialGraphConv -> BN -> ReLU -> TemporalConv -> BN -> Residual
   - Residual connection: skip connection + 1x1 conv (when dimensions need matching)
   - Channel count: [64, 64, 64, 64, 128, 128, 128, 256, 256] (9 blocks)

4. Attention Pooling:

   - Channel attention: GlobalAvgPool -> FC -> Sigmoid -> Scale
   - Temporal attention: Weighted aggregation of temporal dimension to single-frame features

5. Classification Head:

   - Global Average Pooling over time
   - FC(256 -> num_classes=46)

6. Training configuration:

   - Optimizer: AdamW(lr=1e-3, weight_decay=1e-4)
   - CosineAnnealingLR(T_max=300)
   - CrossEntropyLoss(label_smoothing=0.1)
   - Batch=32, Epochs=300 (early stopping patience=30)

7. Evaluation:

   - Top-1 Accuracy
   - Top-5 Accuracy
   - Per-class F1-Score
   - Confusion matrix visualization
```

**Expected Output:**

- src/models/stgcn.py - Complete ST-GCN model
- GraphConv class (spatial convolution based on adjacency matrix)
- TemporalConv class (temporal domain convolution)
- STConvBlock class (ST joint convolution + residual connection)
- STGCNModel main model class
- Training script and evaluation script

**Acceptance Criteria:**

- [ ] Model contains real graph convolution layers (not nn.Linear substitution)
- [ ] Input dimensions (B, 30, 21, 2), output dimensions (B, 46)
- [ ] Model parameter count in 2-5M range
- [ ] Top-1 validation accuracy >85%
- [ ] Forward propagation test passes (no errors)
- [ ] (See P9.2 for detailed replacement prompt)

---

### P5.3 - L2 Inference Pipeline

**Context:** *The L2 inference pipeline connects data reception, preprocessing, ST-GCN inference, post-processing, and NLP integration. It runs continuously on the host side, triggering L2 inference when L1 marks UNKNOWN or for periodic verification. Inference results go through moving average smoothing, confidence filtering, and duration filtering before being passed to the NLP module for grammar correction and TTS announcement.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement the L2 inference pipeline, file path: src/l2_pipeline.py

Requirements:

1. UDP receive thread:

   - Listen on 0.0.0.0:8888
   - Parse Protobuf GloveData
   - Maintain sliding window (buffer_size=30)

2. Inference trigger:

   - Trigger L2 when L1 marks UNKNOWN (gesture_id=0)
   - Or periodically (every 5 seconds) for L2 verification
   - Input: 30 frames x 21 keypoints x 2D coordinates

3. Post-processing:

   - Moving average smoothing (window=3)
   - Confidence threshold: 0.80
   - Gesture duration filter (>=300ms)

4. NLP integration:

   - Gesture ID -> Chinese vocabulary mapping
   - Sentence buffer (collects consecutive gestures)
   - SOV -> SVO grammar correction
   - Call TTS for announcement

5. Performance requirements:

   - Inference latency <20ms (CPU)
   - End-to-end latency <50ms (GPU)
   - Memory usage <500MB
```

**Expected Output:**

- src/l2_pipeline.py - Complete L2 inference pipeline
- UDP reception and Protobuf parsing thread
- Sliding window manager
- Post-processing pipeline (smoothing + filtering)
- NLP and TTS integration interface

**Acceptance Criteria:**

- [ ] UDP reception has no packet loss (LAN environment)
- [ ] Sliding window correctly maintains 30 frames of data
- [ ] L2 correctly triggers when L1 reports UNKNOWN
- [ ] Post-processing stabilizes gesture recognition (no jitter)
- [ ] End-to-end latency (including NLP) <100ms
- [ ] NLP correction correctly outputs Chinese sentences

---

## 7. Phase 6-7: NLP Grammar Correction & Rendering

Chinese Sign Language (CSL) has a different grammatical structure from standard Chinese -- CSL uses SOV (Subject-Object-Verb) word order, while Chinese uses SVO (Subject-Verb-Object). The NLP grammar correction engine is responsible for converting sign language word order to natural Chinese word order. The rendering layer uses Three.js for real-time 3D hand model display in the browser, providing intuitive interactive feedback.

---

### P6.1 - NLP Grammar Correction Engine

**Context:** *Sign language to speech translation is not just vocabulary mapping; it also needs to resolve word order differences. For example, CSL's "I yesterday apple eat" needs to be corrected to "I ate apples yesterday." A basic rule library can handle common patterns, while an advanced solution uses a Transformer model for more complex contexts.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement a Chinese Sign Language (CSL) to Chinese grammar correction engine, file path: src/nlp/grammar_corrector.py

Requirements:

1. Basic rule library implementation:

   - SOV -> SVO conversion templates
   - Time word fronting: "yesterday I apple eat" -> "yesterday I eat apple"
   - Negative sentence conversion: "I not go school" -> "I not go school"
   - Question sentence conversion: "you what want" -> "you want what"

2. Advanced: Transformer correction model (optional):

   - Input: Word sequence in CSL word order
   - Output: Word sequence in Chinese word order
   - Model size: ~10M parameters (lightweight)
   - Training data: CSL-CS parallel corpus

3. Vocabulary mapping:

   - Gesture ID -> Chinese vocabulary dictionary
   - Support polysemy disambiguation (context-based)

4. Context management:

   - Sliding window sentence-level correction
   - Conversation history maintenance
   - Automatic punctuation insertion
```

**Expected Output:**

- src/nlp/grammar_corrector.py - Grammar correction engine
- SOV -> SVO rule library (with 20+ templates)
- Gesture ID -> Chinese vocabulary dictionary
- Context manager
- Punctuation insertion logic

**Acceptance Criteria:**

- [ ] SOV -> SVO conversion accuracy >95% (within rule library coverage)
- [ ] Time word fronting rules are correct
- [ ] Negative and question sentence handling is correct
- [ ] Context management can maintain conversation history
- [ ] Output sentences conform to natural Chinese expression habits

---

### P7.1 - Three.js Browser-Side Rendering

**Context:** *3D hand rendering provides users with intuitive visual feedback -- receiving hand data in real-time via WebSocket and driving skeletal animation of a 3D hand model. Using Three.js ES Module and GLTF format models, no software installation is needed; just open in a browser.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement Three.js + WebSocket based browser-side 3D hand rendering, file path: rendering/threejs-hand.html

Requirements:

1. WebSocket connection:

   - Connect to ws://localhost:8080
   - Receive Protobuf-formatted GloveData
   - Parse and update hand model

2. 3D scene:

   - Use Three.js ES Module
   - Hand model: GLTF/GLB format with skeletal binding
   - 21 bone nodes corresponding to 21 keypoints
   - Lighting: ambient light + directional light

3. Animation updates:

   - Use requestAnimationFrame
   - Update bone rotations directly from WebSocket data
   - Quaternion smoothing (Slerp)

4. UI:

   - Left side: 3D hand view (rotatable/zoomable)
   - Right side: Recognition results panel
   - Bottom: Connection status, latency, FPS display

5. Technical constraints:

   - No software installation needed, open directly in browser
   - Support Chrome/Edge/Safari
   - Target frame rate: 60fps
```

**Expected Output:**

- rendering/threejs-hand.html - Single-file HTML application
- Three.js 3D scene (with hand GLTF model)
- WebSocket connection and Protobuf parsing
- Skeletal animation update logic
- UI panel (recognition results + status info)

**Acceptance Criteria:**

- [ ] Opening HTML file directly in browser works
- [ ] WebSocket successfully connects and receives data
- [ ] 3D hand model correctly responds to sensor data
- [ ] Quaternion smoothing has no jitter
- [ ] FPS >=30 (target 60)
- [ ] Supports mouse rotation and scroll wheel zoom

---

## 8. Phase 8: Integration Testing

Integration testing verifies the complete data flow from sensors to TTS speech output. Pre-recorded data is used to simulate sensor input, verifying the latency and accuracy of L1 inference, communication transmission, L2 inference, and NLP processing layer by layer. End-to-end latency target: <500ms (including TTS), <50ms (excluding TTS).

---

### P8.1 - End-to-End Integration Test Script

**Context:** *Integration testing is the final verification step before release. Pre-recorded NPY data is used to simulate real sensor data streams, automatically running the complete processing pipeline and generating an HTML test report containing latency distribution charts, confusion matrices, and F1-Score tables.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please implement an end-to-end integration test script, file path: tests/integration_test.py

Test flow:

1. Sensor data simulation:

   - Use pre-recorded NPY test data
   - Simulate 100Hz data stream

2. L1 inference test:

   - Verify model loading
   - Verify inference latency <3ms
   - Verify Top-1 accuracy

3. Communication test:

   - BLE connection stability (disconnect/reconnect)
   - UDP packet loss rate (<1%)
   - Protobuf serialization/deserialization consistency

4. L2 inference test:

   - Verify ST-GCN loading
   - Verify inference latency <20ms
   - Verify Top-5 accuracy >99%

5. End-to-end latency test:

   - Sensor -> L1 -> BLE/UDP -> Host -> L2 -> NLP -> TTS
   - Target: <500ms (including TTS)
   - Target: <50ms (excluding TTS)

6. Automated report generation:

   - Generate HTML test report
   - Include latency distribution chart, confusion matrix, F1-Score table
```

**Expected Output:**

- tests/integration_test.py - Complete integration test script
- Sensor data simulator
- L1/L2 inference latency test
- BLE/UDP communication test
- End-to-end latency test
- HTML test report generator

**Acceptance Criteria:**

- [ ] L1 inference latency <3ms (P99)
- [ ] L2 inference latency <20ms (P99)
- [ ] UDP packet loss rate <1% (LAN)
- [ ] End-to-end latency <50ms (excluding TTS)
- [ ] End-to-end latency <500ms (including TTS)
- [ ] HTML report contains all visualization charts

---

## 9. Bug Fix Dedicated Prompts

This section collects known critical bugs and their fix prompts. These bugs come from the review results of the original paper's code. Each bug includes detailed error analysis, root cause explanation, and fix steps. It is recommended to run the fix prompts in this section before executing development prompts to ensure the base code is correct.

---

### P9.1 - FreeRTOS xTaskCreatePinnedToCore Parameter Fix

**Context:** *The parameter definition of xTaskCreatePinnedToCore is (pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask, xCoreID). The original code incorrectly mixed up the function pointer parameter and handle variable, causing the first parameter to pass the handle variable's value (an integer address) instead of the function pointer.*

**Prompt Text (copy and paste directly into Claude Code):**

```
Please fix the parameter order bug in the following FreeRTOS task creation code:

Incorrect code (main.cpp L190-192):

xTaskCreatePinnedToCore(TaskSensorReadHandle, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);
xTaskCreatePinnedToCore(TaskInferenceHandle, "Inference", 8192, NULL, 2, &TaskInferenceHandle, 0);
xTaskCreatePinnedToCore(TaskCommsHandle, "Comms", 8192, NULL, 1, &TaskCommsHandle, 0);

Bug analysis:

The correct parameter order for xTaskCreatePinnedToCore is:
(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask, xCoreID)

The 1st parameter should be the function pointer (Task_SensorRead), not the handle variable (TaskSensorReadHandle)
The 6th parameter should be the address of the handle (&TaskSensorReadHandle)

Correct code:

xTaskCreatePinnedToCore(Task_SensorRead, "SensorRead", 4096, NULL, 3, &TaskSensorReadHandle, 1);
xTaskCreatePinnedToCore(Task_Inference, "Inference", 8192, NULL, 2, &TaskInferenceHandle, 0);
xTaskCreatePinnedToCore(Task_Comms, "Comms", 8192, NULL, 1, &TaskCommsHandle, 0);

Please fix all three occurrences and verify compilation passes.
```

**Expected Output:**

- All three xTaskCreatePinnedToCore calls fixed
- First parameter changed from handle variable to correct function pointer
- Compilation passes with 0 errors

**Acceptance Criteria:**

- [ ] All three parameter orderings are fixed
- [ ] Function pointer parameters are correct (Task_SensorRead, Task_Inference, Task_Comms)
- [ ] Handle address parameters are correct (&TaskSensorReadHandle, &TaskInferenceHandle, &TaskCommsHandle)
- [ ] Compilation passes
- [ ] Tasks run on the correct cores (verified through log output)

---

### P9.2 - ST-GCN Fake Implementation Replacement

**Context:** *The original paper's l2_inference.py STGCNModel only contains an nn.Linear(42*30, 46) -- this is not ST-GCN at all. It flattens the 1260-dimensional input and directly linearly maps it to 46 categories, losing all spatial topology information and temporal dynamic information. It must be completely replaced with the real ST-GCN architecture from P5.2.*

**Prompt Text (copy and paste directly into Claude Code):**

```
The ST-GCN implementation in the original paper (l2_inference.py) has a serious problem:

Problem code:

class STGCNModel(nn.Module):
    def __init__(self, num_classes=46):
        super(STGCNModel, self).__init__()
        self.fc = nn.Linear(42 * 30, num_classes)  # <- This is not ST-GCN!

    def forward(self, x):
        x = x.view(x.size(0), -1)
        return self.fc(x)

This is just a single fully-connected layer with no graph convolution structure at all.
It flattens the input and directly linearly maps it to the number of categories,
losing all spatial (hand skeleton topology) and temporal (action sequence) information.

Requirements:

1. Completely replace this fake implementation with the real ST-GCN architecture defined in P5.2

2. Ensure it includes:
   - Graph Convolution Layer (GraphConv)
   - Temporal Convolution Layer (TemporalConv)
   - ST-Conv Block (residual connections)
   - Attention Pooling

3. Verify model parameter count is in reasonable range (~2-5M)

4. Verify input/output dimensions match: (B, 30, 21, 2) -> (B, 46)

5. Run a forward propagation test to ensure no errors
```

**Expected Output:**

- STGCNModel in l2_inference.py completely replaced
- New model includes GraphConv, TemporalConv, STConvBlock
- Model parameter count in 2-5M range
- Forward propagation test passes

**Acceptance Criteria:**

- [ ] Old nn.Linear implementation is completely removed
- [ ] New model contains at least 9 ST-Conv Blocks
- [ ] Channel count sequence is [64, 64, 64, 64, 128, 128, 128, 256, 256]
- [ ] Input (B, 30, 21, 2) -> Output (B, 46) dimensions match
- [ ] Model parameter count in 2-5M range (verify by printing model.summary())
- [ ] torch.randn(1, 30, 21, 2) forward propagation has no errors
- [ ] Retrain with new model and verify accuracy

---

## Appendix A: Prompt Quick Index

The following table lists all prompts by number and development phase for quick reference.

| **ID**   | **Prompt Title**                        | **Section** | **Category**       |
|----------|-----------------------------------------|-------------|--------------------|
| **P0.1** | Initialize PlatformIO Project           | Section 1   | Environment Setup  |
| **P0.2** | Initialize Python Host Project          | Section 1   | Environment Setup  |
| **P1.1** | TCA9548A I2C Multiplexer Driver         | Section 2   | HAL Driver         |
| **P1.2** | TMAG5273 3D Hall Sensor Driver          | Section 2   | HAL Driver         |
| **P1.3** | BNO085 9-Axis IMU Driver                | Section 2   | HAL Driver         |
| **P1.4** | FreeRTOS Dual-Core Task Scheduling      | Section 2   | HAL Driver         |
| **P2.1** | 1D Kalman Filter                        | Section 3   | Signal Processing  |
| **P2.2** | Data Collection & Annotation Tool       | Section 3   | Signal Processing  |
| **P3.1** | Edge Impulse Data Collection & Training | Section 4   | L1 Inference       |
| **P3.2** | PyTorch 1D-CNN+Attention Training       | Section 4   | L1 Inference       |
| **P3.3** | TFLite Micro Integration                | Section 4   | L1 Inference       |
| **P4.1** | Protobuf Definition & Nanopb Generation | Section 5   | Communication      |
| **P4.2** | BLE 5.0 GATT Service Implementation     | Section 5   | Communication      |
| **P5.1** | Pseudo-Skeleton Mapping & Preprocessing | Section 6   | L2 Inference       |
| **P5.2** | ST-GCN Model Implementation             | Section 6   | L2 Inference       |
| **P5.3** | L2 Inference Pipeline                   | Section 6   | L2 Inference       |
| **P6.1** | NLP Grammar Correction Engine           | Section 7   | NLP                |
| **P7.1** | Three.js Browser-Side Rendering         | Section 7   | Rendering          |
| **P8.1** | End-to-End Integration Test Script      | Section 8   | Testing            |
| **P9.1** | FreeRTOS Parameter Fix                  | Section 9   | Bug Fix            |
| **P9.2** | ST-GCN Fake Implementation Replacement  | Section 9   | Bug Fix            |

---

## Appendix B: Prompt Dependencies

The following lists the dependencies between key prompts. It is recommended to execute them in dependency order.

```
P0.1 --> P1.1 --> P1.2 --> P1.4 --> P3.1 / P3.3
P0.2 --> P2.2 --> P3.2 --> P3.3
P4.1 --> P4.2
P5.1 --> P5.2 --> P5.3
P5.3 --> P6.1
P8.1 depends on all preceding prompts being completed
P9.1 should be executed before P1.4 (fix base code)
P9.2 should be executed before P5.2 (replace fake implementation)
```

---

**--- End of Document ---**

Claude Code Prompt Engineering Handbook v1.0 | Edge AI Data Glove Project | 2026-04-16
