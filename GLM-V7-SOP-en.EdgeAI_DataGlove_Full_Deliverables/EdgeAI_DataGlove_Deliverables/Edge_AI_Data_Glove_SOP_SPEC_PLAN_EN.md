-----------------------------------------------

**Edge AI Data Glove**

**Full-Phase SOP SPEC PLAN**

*Standard Operating Procedure & Specification Plan v2.0*

-----------------------------------------------

Project Type: Sign Language Translation / Motion Capture / Data Glove

MCU: ESP32-S3-DevKitC-1 N16R8

Date: 2026-04-16

**Table of Contents**

**1 Project Vision & Objectives** ···················· 3

**2 Architecture Decision Records (ADR)** ···················· 4

**3 System Architecture Overview** ···················· 6

> 3.1 Hardware Architecture ···················· 6
>
> 3.2 Software Stack ···················· 7
>
> 3.3 Dual-Tier Inference Architecture ···················· 7
>
> 3.4 Phased Communication Architecture ···················· 8

**4 Phase 1: HAL & Driver Layer Development** ···················· 9

> 4.1 TMAG5273 Driver ···················· 9
>
> 4.2 BNO085 Driver ···················· 10
>
> 4.3 TCA9548A Multiplexer ···················· 10
>
> 4.4 FreeRTOS Task Framework ···················· 10
>
> 4.5 Acceptance Criteria ···················· 11

**5 Phase 2: Signal Processing & Data Collection** ···················· 12

**6 Phase 3: L1 Edge Inference TinyML** ···················· 14

> 6.1 Path A: Edge Impulse MVP ···················· 14
>
> 6.2 Path B: PyTorch → TFLite INT8 ···················· 15
>
> 6.3 Inference Trigger Logic ···················· 16
>
> 6.4 Acceptance Criteria ···················· 16

**7 Phase 4: Communication Protocol Implementation** ···················· 17

**8 Phase 5: L2 ST-GCN Upper-Computer Inference** ···················· 19

**9 Phase 6: NLP Grammar Correction & TTS** ···················· 22

**10 Phase 7: Rendering Layer Development** ···················· 24

**11 Phase 8: System Integration & Testing** ···················· 27

**12 Known Issues & Risks** ···················· 29

**13 Appendices** ···················· 30

**1 Project Vision & Objectives**

This project aims to build a complete "edge-cloud collaborative" dual-tier inference data glove system, providing a high-precision, low-latency, low-cost embedded AI solution for sign language translation, motion capture, and human-computer interaction. The core philosophy is to deploy lightweight inference tasks on the edge (Edge), while offloading complex temporal modeling and semantic understanding tasks to the upper computer (Upper), thereby achieving optimal allocation of computing resources and maximizing overall system performance.

In the early MVP (Minimum Viable Product) stage, the project adopts the Tauri + R3F (React Three Fiber) technology stack for rapid 3D visualization verification. Tauri, as a Rust-based cross-platform desktop application framework, provides an extremely small installation footprint and excellent system-level API access, capable of directly listening to serial port and UDP data streams. R3F wraps Three.js's powerful 3D rendering capabilities as React declarative components, greatly improving development efficiency and maintainability. This combination enables end-to-end prototype verification from sensor data to 3D hand visualization within weeks, laying a solid foundation for subsequent high-precision rendering.

In the later high-precision stage, the project will evolve to the Unity 2022 LTS + ms-MANO parametric hand model solution. Unity, as the industry-leading game engine, provides mature XR Hands Package and physics engine support. ms-MANO (Multi-Scale MANO) is based on the MANO hand model, using 48-dimensional pose_parameters to control 21 keypoint 3D coordinates, achieving highly realistic hand rendering and precise joint motion restoration. This solution is particularly suitable for academic paper publication and professional demonstration scenarios, showcasing the system's outstanding performance in sign language translation accuracy and visual naturalness.

**1.1 Five Core Objectives**

This project is systematically designed and developed around the following five core objectives. Each objective corresponds to specific quantitative metrics and acceptance criteria, ensuring that project deliverables are measurable and reproducible:

> • Real-time Performance: L1 edge inference latency is controlled within 3 milliseconds, and end-to-end system latency (from gesture completion to text output) is controlled within 100 milliseconds. Sensor sampling frequency reaches 100Hz, ensuring the ability to capture rapid dynamic changes in sign language. The communication layer uses the UDP protocol for real-time rendering data transmission, allowing packet loss but pursuing minimum latency.
>
> • Accuracy: The L1 edge model achieves a Top-1 accuracy of no less than 90% for 20 simple gesture categories. The L2 upper-computer model achieves a Top-1 accuracy of no less than 95% and a Top-5 accuracy of no less than 99% for the full 46 Chinese sign language vocabulary. The NLP grammar correction module accuracy is no less than 85%, and the TTS speech synthesis naturalness MOS (Mean Opinion Score) is no less than 4.0.
>
> • Low Power Consumption: Through FreeRTOS task scheduling optimization, sensor sleep strategies, and ESP32-S3's Light Sleep mode, the target battery life exceeds 12 hours (based on a 600mAh lithium battery). Average system operating power consumption is controlled within 150mW, and standby power consumption is below 10mW.
>
> • Scalability: The software architecture adopts a modular design, with the HAL layer, signal processing layer, inference layer, communication layer, and rendering layer interacting through well-defined interfaces. Adding new sensor channels (such as flex sensors) only requires modifying the HAL layer and feature extraction modules, without affecting upper-layer inference and communication logic. Models support hot updates and can be pushed via OTA (Over-The-Air).
>
> • Cost Control: Single-unit hardware BOM (Bill of Materials) cost is controlled within $40. Core sensors use Texas Instruments TMAG5273A1 3D Hall sensors and Bosch BNO085 9-axis IMU, both mature mass-produced chips with stable supply chains. PCB design uses a two-layer board solution to reduce fabrication costs.

**2 Architecture Decision Records (ADR)**

Architecture Decision Records (ADR) are the core governance tool for this project's technical decisions, used to document the context, options, rationale, and consequences of each key technical decision. The following five ADRs cover critical areas including model training strategy, graph neural network approach, communication architecture evolution, rendering technology selection, and hardware platform selection. These serve as mandatory reference documents for subsequent project development. Each ADR has been reviewed and confirmed by the team; any subsequent change requires re-initiating the ADR review process.

**2.1 ADR-1: TinyML Dual-Path Strategy**

Decision: Adopt a dual-path TinyML development strategy. Path A uses the Edge Impulse platform for MVP rapid validation. Path B uses native PyTorch training and exports TFLite INT8 models for paper reproduction. Both paths are advanced in parallel, sharing the data collection pipeline and sensor driver code.

Rationale: Edge Impulse provides a complete end-to-end machine learning development platform, including data collection (edge-impulse-data-forwarder), automated feature engineering, model training (supporting 1D-CNN), EON Compiler optimization, and Arduino Library export. This platform can reduce the model development cycle from weeks to days, making it ideal for rapid iterative validation in the MVP phase. However, Edge Impulse's model architecture is limited to the modules provided by the platform, making it impossible to implement custom advanced structures such as TemporalAttention, and the reproducibility of the training process is insufficient for academic paper publication.

Therefore, the paper reproduction phase adopts the native PyTorch training approach: build a 1D-CNN model with TemporalAttention modules (Eq.11-13) using PyTorch, export TFLite INT8 models through ai_edge_torch or a custom quantization pipeline, and finally integrate into the ESP32-S3's TFLite Micro interpreter as a model_data.h header file. This approach is fully controllable and reproducible, with the quantized model size (~38KB) and inference latency (<3ms) both satisfying embedded deployment requirements.

**2.2 ADR-2: Custom ST-GCN Approach**

Decision: Do not use the OpenHands library; build a Spatio-Temporal Graph Convolutional Network (ST-GCN) from scratch based on the MS-GCN3 original paper ("Multi-Step Graph Convolutional Network for 3D Human Pose Estimation"). Although OpenHands provides a PyTorch Geometric-based hand graph convolution implementation, the library has been unmaintained since 2023, does not support wearable sensor data formats (only MediaPipe keypoint input), and its graph convolution implementation differs from the MS-GCN3 paper description.

The custom ST-GCN includes the following core components: First, a Pseudo-skeleton Mapping layer (Eq.16) that uses a linear layer to map 21-dimensional sensor features (15 Hall + 6 IMU channels) into 21×2D skeleton keypoint coordinates, providing structured input for graph convolution. Second, a Spatial Graph Convolution layer that builds an adjacency matrix based on hand skeleton topology, enabling information transfer between joints. Third, a Temporal Convolution layer using TCN (Temporal Convolutional Network) to capture temporal dependencies of sign language gestures. Fourth, ST-Conv Blocks that combine spatial and temporal convolutions into residual blocks with skip connections to mitigate gradient vanishing. Fifth, an Attention Pooling layer that uses channel attention mechanisms to aggregate temporal features and generate final gesture classification logits.

**2.3 ADR-3: Phased Communication Architecture**

Decision: Adopt a phased communication architecture. Phase 1-3 (HAL development, data collection, L1 edge inference) use only BLE 5.0 communication to simplify development complexity. Phase 4 and beyond (L2 upper-computer inference, 3D rendering) add WiFi UDP for high-frequency real-time data transmission to the desktop. BLE in later phases degrades to a network provisioning channel and low-speed backup channel (20Hz).

Rationale: BLE 5.0's stable transmission bandwidth on ESP32-S3 is approximately 50-100 KB/s, sufficient to meet the transmission needs of L1 inference results (~50 bytes per frame) and low-speed sensor data (20Hz × 30 bytes = 600 bytes/second). However, 3D rendering requires a 100Hz full sensor data stream (100Hz × ~100 bytes = 10 KB/s). While BLE bandwidth can barely support this, BLE's connection stability and latency jitter cannot meet the strict requirements of real-time rendering. WiFi UDP latency in LAN environments is typically below 1ms (compared to BLE's 20-50ms), and its packet-loss-tolerant design philosophy aligns well with real-time rendering's fault-tolerance characteristics. Therefore, Phase 4+ adopts a BLE + WiFi dual-channel architecture, with BLE handling WiFi credential transmission and low-speed control commands, and WiFi UDP handling high-frequency sensor data broadcasting.

**2.4 ADR-4: Three-Stage Rendering Evolution**

Decision: The rendering layer adopts a three-stage progressive evolution strategy. In the Phase 2 MVP stage, Tauri 2.0 + R3F (React Three Fiber) is used for cross-platform desktop 3D rendering, rapidly verifying the complete pipeline from sensor data to 3D hand visualization. In the Phase 2.5 stage, migration to a Three.js + WebSocket browser-only solution reduces the barrier to use (no desktop app installation required, browser access only). In Phase 4+, upgrade to Unity 2022 LTS + XR Hands Package + ms-MANO parametric hand model for high-fidelity professional-grade rendering.

Rationale: The core idea of the three-stage strategy is "validate the core pipeline with minimum cost first, then progressively improve visual quality." The Tauri + R3F development cycle is approximately 2-3 weeks with a package size of ~10MB, enabling rapid verification of the data glove's complete workflow. The Three.js + WebSocket solution further reduces deployment barriers, suitable for demonstration and remote access scenarios. The Unity + ms-MANO solution has a longer development cycle (~6-8 weeks) but delivers photorealistic visual quality and precise joint control, representing the final form for paper publication and productization. The 3D skeleton definitions and joint rotation interfaces remain consistent across all three stages, ensuring smooth rendering layer evolution.

**2.5 ADR-5: Hardware Platform Retention**

Decision: Continue using ESP32-S3-DevKitC-1 N16R8 as the main MCU, without switching to Seeed Studio XIAO ESP32S3. This decision has been verified through detailed hardware parameter comparison and actual testing, with sufficient technical basis.

Rationale: DevKitC-1 is equipped with 16MB Octal PSRAM (in the N16R8 model, N8 represents 8MB Flash, R8 represents 8MB PSRAM, totaling 16MB PSRAM), which is critical for TFLite Micro model inference and sensor data buffering. Actual testing shows that ST-GCN model intermediate activations (activations) require approximately 2-3MB of runtime memory. Combined with the sensor ring buffer (30 frames × 21 features × 4 bytes = ~2.5KB) and Protobuf serialization buffer, the system's peak memory usage is approximately 4-5MB, far exceeding the XIAO ESP32S3's 8MB PSRAM available space (actual usable approximately 5-6MB after system overhead). Additionally, the DevKitC-1's PCB layout has been validated, with I2C bus signal integrity and antenna performance both meeting design requirements. Switching platforms would require PCB redesign, increasing project risk and cost.

**3 System Architecture Overview**

This chapter comprehensively describes the Edge AI Data Glove system's technical architecture from four dimensions: hardware architecture, software stack, dual-tier inference architecture, and phased communication architecture. The system adopts a layered decoupling design principle, with each layer interacting through well-defined interfaces, ensuring module replaceability and system maintainability. The hardware layer handles sensor data acquisition and low-level communication. The software layer handles signal processing, model inference, and application logic. The inference layer handles gesture recognition. The communication layer handles data transmission. The rendering layer handles 3D visualization. This layered architecture allows each layer to be independently developed and tested, greatly reducing system integration risk.

**3.1 Hardware Architecture**

The system hardware centers on the ESP32-S3-DevKitC-1 N16R8 as the core controller. This chip is manufactured using TSMC's 40nm process, featuring a dual-core Xtensa LX7 processor (240MHz main frequency), 512KB built-in SRAM, and AI vector extension instructions (supporting INT8 multiply-accumulate acceleration), making it Espressif's flagship MCU designed specifically for AIoT scenarios. The N16R8 model is equipped with 8MB Flash and 8MB Octal PSRAM. Flash stores firmware code and model weights, while PSRAM is used for runtime data buffering and model inference intermediate result storage.

The sensor module consists of three parts: First, 5 Texas Instruments TMAG5273A1 3D Hall sensors (I2C interface), installed at the proximal interphalangeal (PIP) joints of the thumb, index, middle, ring, and little fingers respectively, for detecting finger bending angles. Each sensor outputs 3-axis magnetic field strength data (X/Y/Z, 12-bit resolution) with a range of ±40mT, using Set/Reset trigger mode for temperature drift compensation. The 5 sensors share the same I2C bus through the TCA9548A I2C multiplexer, all with I2C address 0x22 (accessed through TCA9548A channels 0-4 respectively). Second, 1 Bosch BNO085 9-axis IMU (I2C interface), installed at the glove wrist position, integrating a 3-axis accelerometer, 3-axis gyroscope, and 3-axis magnetometer, with built-in SensorHub processor implementing hardware-level sensor fusion, directly outputting Game Rotation Vector quaternions (no geomagnetic influence) at 100Hz. Third, the TCA9548A I2C multiplexer (address 0x70), responsible for switching between the 5 TMAG5273 sensors, resolving I2C address conflicts.

The system also reserves 5 ADC channels (GPIO 4, 5, 6, 7, 15) for future expansion of flex sensors. The I2C bus operates at 400kHz Fast Mode, with main bus pull-up resistors of 2.2kΩ and TCA9548A sub-channel pull-up resistors of 4.7kΩ, ensuring signal integrity. The actual BOM cost is approximately $38.60 (including DevKitC-1 development board, sensor modules, battery, PCB, and enclosure), significantly higher than the $18 claimed in the paper (which represents bulk die pricing excluding PCB and assembly costs).

**3.1.1 Core BOM List**

| **Component** | **Model** | **Qty** | **Unit Price (USD)** | **Notes** |
|---|---|---|---|---|
| MCU Dev Board | ESP32-S3-DevKitC-1 N16R8 | 1 | $13.50 | Dual-core 240MHz, 8MB PSRAM |
| 3D Hall Sensor | TMAG5273A1 | 5 | $2.80 | I2C, 12-bit, ±40mT |
| 9-axis IMU | BNO085 | 1 | $9.50 | Hardware quaternion fusion |
| I2C Multiplexer | TCA9548A | 1 | $0.85 | 8-channel |
| Lithium Battery | 600mAh 3.7V | 1 | $3.50 | JST-PH connector |
| PCB + Enclosure | Custom | 1 | $5.00 | Two-layer board, 3D-printed enclosure |
| Other (resistors/capacitors/connectors) | --- | --- | $3.45 | Pull-up resistors, ribbon cables, etc. |

**Total: Approximately $38.60**

**3.2 Software Stack**

The system software stack is divided into three main layers: the embedded layer, the upper-computer AI layer, and the rendering layer. The embedded layer is developed based on PlatformIO + Arduino framework, running the FreeRTOS real-time operating system, using Nanopb (Nano version of Protocol Buffers) for data serialization, and integrating the TFLite Micro inference engine. PlatformIO provides a powerful build system and dependency management, supporting automatic optimization for ESP32-S3's AI vector instruction set. FreeRTOS handles task scheduling and resource management, ensuring that the three core tasks of sensor sampling, model inference, and communication transmission can execute in parallel without interference.

The upper-computer AI layer is developed based on Python 3.9+, using PyTorch as the deep learning framework, with a custom ST-GCN model for L2 upper-computer inference. The data preprocessing pipeline includes Kalman filtering, Min-Max normalization, and sliding window buffering. Speech synthesis uses Microsoft's edge-tts library, which is completely free and requires no API key, supporting multiple Chinese voices (zh-CN-XiaoxiaoNeural female, zh-CN-YunxiNeural male), with natural and smooth audio quality. The NLP grammar correction module initially uses a rule-based approach, with potential future upgrade to a lightweight Transformer model.

The rendering layer is implemented following the three-stage evolution strategy: Phase 2 MVP uses the Tauri 2.0 + React + R3F (React Three Fiber) + Zustand + TailwindCSS technology stack, with the Rust backend responsible for listening to BLE serial and UDP data streams and parsing Protobuf messages. Phase 2.5 migrates to a Three.js + WebSocket browser-only solution. Phase 4+ upgrades to Unity 2022.3 LTS + XR Hands Package + ms-MANO parametric hand model. The 3D skeleton definitions remain compatible across all three stages, ensuring smooth evolution.

**3.3 Dual-Tier Inference Architecture**

The dual-tier inference architecture is one of the most innovative designs of this system. The L1 layer (Edge inference) runs on the ESP32-S3, using a lightweight 1D-CNN + TemporalAttention model with approximately 34K total parameters (~38KB after INT8 quantization) and inference latency below 3ms. The L1 model input is a sliding window of 30 frames × 21-dimensional features (630 dimensions), outputting classification probabilities for 46 gesture categories. The core advantage of the L1 model is its ultra-low latency and offline availability—even when BLE/WiFi is disconnected, the data glove can still independently complete basic gesture recognition.

The L2 layer (Upper-computer inference) runs on a PC, using a custom ST-GCN model to process dynamic sign language sequences. The L2 model receives the complete temporal sensor data stream, converts 21-dimensional sensor features into 21×2D skeleton keypoint coordinates through a pseudo-skeleton mapping layer, then extracts spatio-temporal features through multiple ST-Conv Blocks, and finally generates gesture classification results through an attention pooling layer. The L2 model has far greater parameter count and computational cost than L1, but thanks to GPU acceleration, inference latency can still be controlled within 20ms.

The core logic of dual-tier inference is confidence-driven intelligent routing: when the L1 model's output confidence exceeds 0.85, it directly outputs the gesture ID and Chinese name without L2 involvement, thereby minimizing end-to-end latency. When L1 confidence is at or below 0.85, it is marked as UNKNOWN, and the current sliding window's raw sensor data is forwarded to the upper computer via BLE/WiFi, triggering L2 inference. This design guarantees recognition accuracy while maximizing the use of edge computing capabilities, reducing unnecessary communication and upper-computer computational overhead.

**3.4 Phased Communication Architecture**

The communication architecture evolves progressively according to project phases. In Phase 1-3 (HAL development, data collection, L1 edge inference), the system uses only BLE 5.0 communication. The BLE GATT service provides two main functions: a provisioning service (receiving WiFi SSID and Password from the upper computer, stored in the NVS partition) and a data broadcast service (pushing sensor data or inference results to connected clients via Notification). Additionally, ESP32-S3 outputs CSV-formatted sensor data via serial port for the edge-impulse-data-forwarder tool, which automatically uploads CSV data to the Edge Impulse cloud for model training.

From Phase 4 onward, the system adds a WiFi UDP communication channel. The WiFi UDP target port is 8888, with a transmission frequency of 100Hz. Each frame is serialized using Protobuf (GloveData message, containing timestamp, 15 Hall features, 6 IMU features, 5 flex sensor features (reserved), L1 gesture ID, and L1 confidence). UDP's connectionless, packet-loss-tolerant nature aligns well with real-time rendering's fault-tolerance requirements—occasional frame drops do not affect rendering continuity but can trade for lower transmission latency (typically below 1ms in LAN). BLE in this phase degrades to a provisioning channel and low-speed backup channel (20Hz), used for degraded communication when WiFi is unavailable.

**4 Phase 1: HAL & Driver Layer Development**

Phase 1 is the foundation of the entire project. The goal is to complete the Hardware Abstraction Layer (HAL) and all sensor driver development, ensuring that ESP32-S3 can stably acquire raw data from sensors. The quality of work in this phase directly impacts data quality and model performance in all subsequent phases. Therefore, it must be executed strictly according to acceptance criteria. Any issues not meeting standards must be resolved in this phase and must not be carried into subsequent phases. The development tool for this phase is PlatformIO + Arduino framework, and the debugging tools are ESP-IDF's built-in logic analyzer component and serial monitor.

**4.1 TMAG5273 Driver Development**

TMAG5273A1 is a high-precision linear 3D Hall effect sensor from Texas Instruments, supporting I2C interface communication and providing 12-bit resolution X/Y/Z three-axis magnetic field strength output. The core function of the driver is to implement the ReadXYZ() method, which reads the sensor's magnetic field data registers (MAG_X, MAG_Y, MAG_Z, 2 bytes each, Little-Endian format) via the I2C bus. The sensor is connected to the ESP32-S3's I2C main bus through the TCA9548A multiplexer, with an I2C device address of 0x22 (default configuration, A0/A1 pins grounded).

The driver initialization process is as follows: first select the target channel through TCA9548A (channels 0-4 correspond to sensors on thumb to little finger), then write the sensor configuration register to set trigger mode to Set/Reset (automatic temperature drift compensation), range to ±40mT (default), and conversion mode to continuous conversion. After initialization, the ReadXYZ() method executes as follows: select TCA9548A channel → wait 1ms for bus stabilization → read magnetic field data registers (6 bytes) → combine into X/Y/Z three-axis 12-bit signed integer values → convert to physical units (mT). Note that TCA9548A channel switching must be performed before each read; otherwise, incorrect sensor data will be read. This is one of the most common bugs in I2C multiplexer architectures.

**4.2 BNO085 Driver Development**

BNO085 is Bosch's next-generation smart 9-axis IMU, integrating a 3-axis accelerometer, 3-axis gyroscope, 3-axis magnetometer, and a 32-bit ARM Cortex-M0+ SensorHub processor. Unlike the previous generation BNO055, BNO085 uses Bosch's proprietary SH-2 (Sensor Hub 2) protocol for configuration and data reading, supporting rich sensor fusion algorithms. This project's driver needs to enable the following two Sensor Reports: SH2_GAME_ROTATION_VECTOR (Report ID 0x05), outputting quaternions without geomagnetic influence (X/Y/Z/W) at 100Hz; and SH2_GYROSCOPE_CALIBRATED (Report ID 0x0D), outputting calibrated 3-axis angular velocity (rad/s) at 100Hz.

Using hardware sensor fusion is one of the key design decisions. The BNO085's built-in SensorHub processor executes sensor fusion algorithms at 1kHz frequency, outputting stable quaternions processed through Kalman filtering and complementary filtering. This is more efficient and accurate than software-based fusion on the ESP32-S3 (which would require approximately 200μs/frame computation time). The driver implementation needs to handle SH-2 protocol details, including product ID queries, pin assignment (INT pin connected to GPIO 21 for data-ready interrupt), report enable/disable commands, and data reading. Quaternion data reading can be done in two ways: polling mode and interrupt mode. This project uses interrupt mode—when the BNO085's INT pin goes low, sensor data is read from the FIFO in a FreeRTOS task, ensuring no sampling points are lost.

**4.3 TCA9548A Multiplexer Driver**

TCA9548A is an 8-channel I2C multiplexer/switch from Texas Instruments, controlling up to 8 sub-I2C channels through the I2C main bus. This project uses 5 channels (channels 0-4), connecting 5 TMAG5273 sensors respectively. The TCA9548A's I2C device address is 0x70 (A0/A1/A2 pins all grounded), and the control register is a single byte where each bit corresponds to one channel's on/off state (1=on, 0=off).

The driver's core method is selectChannel(uint8_t channel), which writes (1 << channel) to the TCA9548A's control register to activate the specified channel. Note that TCA9548A's channel switching is not mutually exclusive—writing a new channel selection value does not automatically close previously opened channels. Therefore, before selecting a new channel, it is recommended to first write 0x00 to close all channels, then write the target channel value. Although this two-step operation adds an extra I2C write, it effectively avoids bus conflicts caused by multiple channels being open simultaneously. Additionally, at least 1ms of bus stabilization time (t_CHG) is required after channel switching (according to the datasheet the typical value is 0.6μs, but considering PCB trace and cable capacitance, 1ms is recommended in practice) before performing subsequent sensor read/write operations.

**4.4 FreeRTOS Task Framework**

ESP32-S3 runs the FreeRTOS real-time operating system. The task allocation in this project follows the principle of "separating sensor sampling from inference, with independent communication," dividing the system into three core tasks bound to different CPU cores. ESP32-S3's dual-core architecture (Core 0 and Core 1) enables sensor sampling and model inference to truly execute in parallel without missing sampling time points due to inference computation.

| **Task Name** | **Pinned Core** | **Priority** | **Frequency** | **Function Description** |
|---|---|---|---|---|
| Task_SensorRead | Core 1 | 3 (highest) | 100Hz | I2C sensor data acquisition |
| Task_Inference | Core 0 | 2 | On-demand trigger | TFLite Micro L1 inference |
| Task_Comms | Core 0 | 1 (lowest) | 100Hz/event-driven | BLE/WiFi data transmission |

Task_SensorRead runs at a fixed 100Hz frequency on Core 1 and is the highest-priority task in the system. This task performs the following operations in a fixed order: read BNO085 quaternion and gyroscope data → iterate through TCA9548A channels 0-4 to read 5 TMAG5273 magnetic field data → write raw data to a shared ring buffer → wake Task_Inference via task notification (xTaskNotify) when the buffer is filled with 30 frames. The total execution time of this task is approximately 5-8ms (including I2C communication and channel switching), which can be stably completed within the 10ms sampling period (100Hz).

⚠️ Critical Bug Fix Record: During the initial implementation of FreeRTOS task creation, a serious parameter order error was discovered in the xTaskCreatePinnedToCore() function. The correct parameter order is: xTaskCreatePinnedToCore(taskFunction, name, stackSize, parameter, priority, taskHandle, coreID). In the early code, the function pointer (taskFunction) and task handle (taskHandle) positions were swapped, causing FreeRTOS to treat the task handle (an integer pointer) as a function pointer for execution, triggering a Cache Access Exception and immediate system crash. The error message was "Guru Meditation Error: Core 0 panic'ed (CacheAccessError)", and the troubleshooting process was very difficult because the error message did not clearly point to the parameter order issue. Fix: strictly follow the ESP-IDF API documentation to ensure correct parameter order, and add compile-time static assertions (static_assert) to validate function pointer validity.

**4.5 Acceptance Criteria**

Phase 1 acceptance criteria ensure the correctness and stability of all sensor drivers, providing a reliable data foundation for subsequent phases. Specific acceptance criteria are as follows: First, all 5 TMAG5273 sensors can stably read X/Y/Z three-axis magnetic field data, with no I2C bus conflicts or data corruption during TCA9548A channel switching, and no errors during continuous 24-hour operation. Second, BNO085 quaternion output is stable, rotation accuracy meets hand pose detection requirements, and calibration functions work properly (automatic calibration completes within 5 seconds of system startup). Third, the overall sampling frequency stably reaches 100Hz, with sampling interval standard deviation less than 0.5ms. Fourth, FreeRTOS task scheduling is normal, with no deadlocks, priority inversions, or stack overflows across all three tasks. Fifth, serial port CSV output format is correct and can be properly collected and uploaded by the edge-impulse-data-forwarder tool.

**5 Phase 2: Signal Processing & Data Collection**

The core objective of Phase 2 is to establish the complete signal processing pipeline from raw sensor data to machine learning model input, and to develop efficient data collection and annotation tools. The quality of work in this phase directly determines the data quality for subsequent model training, embodying the project's "data is king" philosophy. The signal processing pipeline includes three main modules: Kalman filtering, data normalization, and sliding window buffering. The design of each module must fully consider the embedded side's computational resource constraints (ESP32-S3's floating-point computation capability is limited; fixed-point operations or lookup tables should be used as much as possible to replace floating-point operations).

**5.1 Kalman Filtering**

TMAG5273 Hall sensor raw output inevitably contains environmental magnetic field noise, electromagnetic interference (EMI), and quantization noise. To improve signal quality, this system uses a 1D Kalman Filter for independent filtering of all 15 Hall signal channels (5 sensors × 3 axes). The advantage of the Kalman filter is its ability to adaptively adjust filtering strength based on sensor noise characteristics—reducing filtering during rapid signal changes (such as fast finger bending) to maintain responsiveness, and increasing filtering during stable signals to suppress noise.

The Kalman filter's state equation and observation equation are defined as follows: State equation x(k) = x(k-1) + w(k), where x(k) is the true value at the current time, and w(k) is the process noise following N(0, Q) distribution. Observation equation z(k) = x(k) + v(k), where z(k) is the sensor measurement, and v(k) is the measurement noise following N(0, R) distribution. The filter update steps include a Predict step and an Update step: the Predict step calculates the prior state estimate x̂(k|k-1) = x̂(k-1|k-1) and prior error covariance P(k|k-1) = P(k-1|k-1) + Q. The Update step calculates the Kalman gain K(k) = P(k|k-1) / (P(k|k-1) + R), then updates the state estimate x̂(k|k) = x̂(k|k-1) + K(k) × (z(k) - x̂(k|k-1)) and error covariance P(k|k) = (1 - K(k)) × P(k|k-1).

Tuning notes: The process noise Q value determines how fast the filter tracks signal changes; larger Q means faster tracking but weaker filtering. The measurement noise R value determines how much the filter trusts the measurements; larger R means stronger filtering but slower response. For Hall signals of sign language gestures, recommended initial parameters are Q = 1e-4 and R = 1e-2, with fine-tuning through actual signal analysis. When implementing on ESP32-S3, fixed-point operations (Q15 or Q31 format) are recommended to replace floating-point operations, reducing single-channel filter computation time from approximately 10μs to approximately 3μs.

**5.2 Data Normalization & Feature Vector Assembly**

Kalman-filtered data must be normalized before being used as model input. Hall data uses Min-Max normalization, mapping 12-bit raw values (range 0-4095) to the [0, 1] interval. The normalization formula is x_norm = (x - x_min) / (x_max - x_min), where x_min and x_max are global minimum and maximum values derived from statistics of all gesture actions during the data collection phase. Note that different fingers' Hall sensors may have different ranges (the thumb's magnetic field variation range is typically larger than the little finger's), so x_min and x_max should be calculated separately for each sensor channel.

BNO085's quaternion output (Qx, Qy, Qz, Qw) needs to be converted to Euler angles (Roll, Pitch, Yaw) to unify the format with Hall data. The quaternion-to-Euler-angle conversion formula uses standard three-axis rotation matrix decomposition, but the gimbal lock issue must be noted—when Pitch approaches ±90°, the Roll and Yaw calculations become numerically unstable. This system avoids the problem by limiting the hand's range of motion (wrist Pitch in sign language gestures rarely approaches ±90°) and using high-precision floating-point operations. The final feature vector is 21-dimensional: 15-dimensional normalized Hall data + 3-dimensional Euler angles (Roll, Pitch, Yaw) + 3-dimensional gyroscope data (Gx, Gy, Gz, also requiring normalization).

**5.3 Sliding Window Buffer**

Gesture recognition models require a continuous period of sensor data as input, not single-frame data. This project uses a Sliding Window mechanism to organize temporal data: the window size is 30 frames (corresponding to 300ms of data at 100Hz sampling rate), and the window step is 1 frame (i.e., the window slides by one position each time new data arrives). The 30-frame window size has been experimentally verified to achieve a good balance between temporal resolution and computational efficiency—smaller windows (e.g., 15 frames) may cause the model to fail to capture complete gesture dynamics, while larger windows (e.g., 60 frames) increase inference latency and memory usage.

A Ring Buffer is an efficient data structure for implementing sliding windows. On the ESP32-S3, a contiguous memory area of 30 × 21 × sizeof(float) = 2520 bytes is allocated as the ring buffer, using a head pointer and tail pointer to manage data writing and reading. Each time a new frame arrives, it is written to the position pointed to by head, then head = (head + 1) % 30. When head == tail, the buffer is full, and tail = (tail + 1) % 30, automatically overwriting the oldest data. This implementation requires no data movement, with O(1) time complexity. When the buffer fills with 30 frames, it triggers the inference task or notifies the upper computer that data is ready.

**5.4 Data Collection & Annotation Tools**

Data collection is the most time-consuming but most critical component of a machine learning project. This project provides two sets of data collection tools: The first is an automated collection solution based on edge-impulse-data-forwarder, suitable for the Edge Impulse path (Path A). This tool automatically collects sensor data through the ESP32-S3's serial port CSV output and uploads it to the Edge Impulse cloud, supporting online annotation and model training. The second is a custom collection solution based on Python, suitable for the PyTorch path (Path B). The Python script receives sensor data via BLE or serial port and saves in both CSV (human-readable) and NPY (NumPy binary format, fast loading) dual formats.

The annotation tool supports labeling of 46 Chinese sign language vocabulary entries, using a "record first, annotate later" workflow. During recording, the collection script continuously records sensor data at 100Hz. Annotators press a hotkey (or click a GUI button) after each gesture action is completed to add a timestamp marker and gesture label. After annotation, the script splits the continuous data stream into independent gesture samples based on timestamps (each sample includes a 5-frame silence segment before the gesture + gesture segment + 5-frame silence segment after the gesture) and saves them as structured files. It is recommended to collect at least 100 samples per gesture category (recorded by 3-5 different sign language users) to ensure data diversity and model generalization capability.

**6 Phase 3: L1 Edge Inference TinyML**

Phase 3 is the core AI module development stage of the project. The goal is to deploy a lightweight gesture classification model (L1 inference) on the ESP32-S3, enabling real-time on-device gesture recognition. This phase adopts a dual-path development strategy: Path A uses the Edge Impulse platform for MVP rapid validation. Path B uses native PyTorch training and exports TFLite INT8 models for paper reproduction. Both paths share the same dataset and sensor driver code, but differ in model training and deployment methods.

**6.1 Path A: Edge Impulse MVP (Phase 2-3)**

The Edge Impulse path aims to complete the full pipeline verification from data collection to edge deployment in the shortest possible time. Specific steps are as follows: Step 1, use the edge-impulse-data-forwarder tool to collect sensor data through the ESP32-S3's serial port CSV output and automatically upload to the Edge Impulse cloud project. Step 2, perform data annotation in the EI Studio's Data Acquisition page, marking each sample with the corresponding gesture category. Step 3, configure signal processing parameters (Raw Data input, no additional feature engineering needed) and model parameters (1D-CNN classifier, 200 training epochs, 0.001 learning rate) in the EI Studio's Impulse Design page. Step 4, after training, enable EON (Edge Optimized Neural) Compiler optimization, which automatically performs operator fusion, memory optimization, and INT8 quantization to generate a model optimized for ESP32-S3. Step 5, export as an Arduino Library (C++ library) and integrate into the project through PlatformIO's lib_deps mechanism.

The advantage of the Edge Impulse path is fast development speed—from data collection to edge deployment typically takes only 2-3 days, making it ideal for rapid validation of data quality, sensor layout, and model architecture feasibility in the early project phase. However, this path also has obvious disadvantages: the model architecture is limited to the platform's provided modules (custom advanced structures like attention mechanisms cannot be implemented), training hyperparameter controllability is relatively low, and model training executes in the cloud with insufficient reproducibility. Therefore, the Edge Impulse path is only used for MVP validation, not for final productization or paper publication.

**6.2 Path B: PyTorch → TFLite INT8 (Phase 3+)**

The PyTorch path is the formal approach for paper reproduction and productization, providing a fully controllable model training and deployment pipeline. The model architecture is 1D-CNN + TemporalAttention, specifically optimized for embedded deployment. Input dimensions are (Batch, Channels=21, Time=30), and output dimensions are (Batch, Classes=46). The model structure is defined as follows:

| **Layer Name** | **Structure** | **Output Dimensions** | **Parameters** |
|---|---|---|---|
| Input | --- | (B, 21, 30) | 0 |
| Block1 | Conv1d(21→32, k=5, p=2) + BN + ReLU + MaxPool(2) | (B, 32, 14) | ~3.5K |
| Block2 | Conv1d(32→64, k=3, p=1) + BN + ReLU + MaxPool(2) | (B, 64, 6) | ~6.4K |
| Block3 | Conv1d(64→128, k=3, p=1) + BN + ReLU | (B, 128, 4) | ~25K |
| TempAttention | Eq.11-13: Channel Attention | (B, 128, 4) | ~0.3K |
| GlobalAvgPool | AdaptiveAvgPool1d(1) | (B, 128, 1) | 0 |
| FC | Linear(128→46) | (B, 46) | ~6K |

The TemporalAttention module (Eq.11-13) is the core innovation of this model. Its implementation is as follows: perform global average pooling over the time dimension on Block3's output feature map (dimensions B×128×4) to obtain a channel descriptor c ∈ R^128, then generate channel attention weights a ∈ R^128 through a two-layer fully connected network (128→16→128, with ReLU activation and Sigmoid gating), and finally multiply the attention weights with the original feature map channel by channel. This mechanism enables the model to adaptively focus on the most discriminative feature channels, thereby improving classification accuracy.

Model quantization is a critical step for embedded deployment. This project uses INT8 Post-Training Quantization, quantizing the PyTorch FP32 model's weights and activations from 32-bit floating point to 8-bit integers. The quantization flow is: PyTorch model → ONNX intermediate format → TFLite FP32 model → TFLite INT8 model (using representative_dataset to calibrate quantization parameters). The quantized model size is reduced from approximately 140KB (FP32) to approximately 38KB (INT8), inference speed increases approximately 3-4 times, and accuracy loss is typically controlled within 1-2%. Finally, the xxd tool is used to convert the TFLite FlatBuffer file to a C language header file model_data.h, integrated into the PlatformIO project and loaded and executed by the TFLite Micro interpreter.

**6.3 Inference Trigger Logic**

The L1 inference trigger logic is based on two core mechanisms: the sliding window buffer and confidence threshold. When the ring buffer fills with 30 frames of data (corresponding to 300ms of sensor data), Task_Inference is awakened to execute TFLite Micro inference. The inference output is a 46-dimensional softmax probability vector, where the maximum probability value is the predicted confidence. The system determines subsequent actions based on the relationship between the confidence and a preset threshold (0.85):

High confidence (> 0.85): Directly output the gesture ID and corresponding Chinese name, send to the upper computer via BLE/WiFi, and trigger TTS speech synthesis or 3D animation playback.

Low confidence (≤ 0.85): Mark as UNKNOWN, and simultaneously pack the current sliding window's 630-dimensional feature vector as a Protobuf message, forward to the upper computer via BLE/WiFi to trigger L2 ST-GCN inference.

To prevent consecutive frames from repeatedly outputting the same gesture, the system also implements a Debouncing mechanism: a gesture is only confirmed as output when N consecutive frames (N=5, i.e., 50ms) output the same gesture ID and all confidences exceed the threshold. The debouncing time window is 50ms, effectively avoiding gesture boundary jitter misrecognition while maintaining response speed. Additionally, the system implements gesture interval detection—when a gesture switch is detected (current frame ID differs from previous frame ID), a 100ms silence period is forcibly inserted to avoid misrecognition at gesture transition moments.

**6.4 Acceptance Criteria**

Phase 3 acceptance criteria ensure that the L1 edge inference model meets design requirements for real-time performance, accuracy, and resource efficiency. Specific metrics are as follows:

Inference latency: The total latency for a single inference on ESP32-S3 (including data preprocessing, model inference, and post-processing) must be below 3ms, ensuring it does not affect the 100Hz sensor sampling task.

Top-1 accuracy: Top-1 accuracy for 20 simple gesture categories (including digits 0-9 and 10 basic gesture vocabulary) must be no less than 90% (evaluated on an independent test set).

Memory usage: Total memory usage of model weights + TFLite Micro interpreter + runtime buffers must not exceed 200KB (PSRAM), ensuring it does not affect memory requirements of other tasks.

Continuous operation stability: The model maintains normal inference after 24 hours of continuous operation, with no memory leaks or accuracy degradation.

**7 Phase 4: Communication Protocol Implementation**

Phase 4 aims to implement a stable and efficient sensor data transmission protocol connecting the embedded side and the upper computer. This phase requires simultaneous implementation of both BLE 5.0 and WiFi UDP communication methods, using Protobuf (Protocol Buffers) as the unified data serialization format. The communication protocol design must balance bandwidth utilization, latency, reliability, and implementation complexity.

**7.1 Protobuf Data Frame Definition**

Protobuf is Google's efficient binary serialization protocol, offering smaller size (typically 3-10 times smaller) and faster serialization/deserialization speed compared to JSON format. This project uses Nanopb (Nano version of Protobuf) for serialization on the ESP32-S3. Nanopb is deeply optimized for embedded systems, with a code size of less than 2KB and RAM usage of less than 512 bytes. The following is the core data frame structure defined for this project, glove_data.proto:

```protobuf
message GloveData {
  uint32 timestamp = 1;              // System timestamp (ms)
  repeated float hall_features = 2;   // 15-channel Hall sensor features
  repeated float imu_features = 3;    // 6-channel IMU features (3 Euler + 3 gyro)
  repeated float flex_features = 4;   // 5-channel flex sensors (reserved)
  uint32 l1_gesture_id = 5;          // L1 inference result: gesture ID
  uint32 l1_confidence_x100 = 6;      // L1 confidence × 100 (integer)
}
```

The GloveData message design considers backward compatibility and extensibility. Field numbers, once assigned, must not be changed; new fields only need new numbers. The `repeated` keyword represents variable-length arrays, and Nanopb pre-allocates memory based on maximum lengths defined in the .options file. The l1_confidence_x100 field uses integer encoding (confidence × 100) to avoid floating-point precision differences between embedded and desktop sides. The Nanopb compiler automatically generates C language structs and serialization/deserialization functions based on the .proto file.

**7.2 BLE 5.0 Implementation**

BLE 5.0 implementation is based on ESP-IDF's built-in Bluedroid protocol stack, defining two GATT services. The first is the standardized Environmental Sensing Service (UUID 0x181A), containing two Characteristics: a Sensor Data Characteristic (supporting Notify for proactive sensor data pushing) and a Config Characteristic (supporting Read/Write for configuring sampling frequency, filtering parameters, etc.). The second is a custom provisioning service, containing a Write Characteristic for receiving WiFi SSID and Password from the upper computer (JSON format, AES-128 encrypted), stored in ESP32-S3's NVS (Non-Volatile Storage) partition. MTU (Maximum Transmission Unit) negotiation target is 512 bytes, ensuring a single Protobuf frame can be transmitted in one BLE transfer.

Key technical considerations for BLE communication include: connection parameter optimization (Connection Interval set to 10ms, Slave Latency set to 0, ensuring minimum latency); data fragmentation handling (automatic fragmentation when Protobuf data exceeds current MTU); automatic reconnection on disconnection (exponential backoff strategy, maximum retry interval of 30 seconds); and multi-client management (supporting simultaneous connection of one data receiving client and one configuration client). After Phase 4+, BLE primarily handles provisioning and low-speed backup (20Hz) roles, with high-frequency data transmission taken over by WiFi UDP.

**7.3 WiFi UDP Implementation (Phase 4+)**

WiFi UDP implementation is based on ESP-IDF's lwIP protocol stack, using non-blocking UDP sockets for data transmission. The target port is 8888, with a transmission frequency of 100Hz (synchronized with sensor sampling frequency). Each frame is a Nanopb-serialized GloveData message (typical size approximately 80-120 bytes). UDP's connectionless nature means the sender does not need to establish or maintain connection state; each frame is sent independently, allowing occasional packet loss without affecting subsequent data transmission. This design philosophy aligns well with real-time rendering scenarios—the rendering side can tolerate occasional frame drops (the next frame arrives immediately) but cannot tolerate TCP's retransmission delays and head-of-line blocking.

WiFi initialization flow: System startup → Read WiFi credentials from NVS → Connect to AP → Obtain IP address → Create UDP socket → Bind local port → Start sending data. If WiFi connection fails (e.g., incorrect credentials or AP unreachable), the system automatically falls back to BLE-only mode and notifies the upper computer via BLE that WiFi is unavailable. WiFi transmission executes in Core 0's Task_Comms, running in parallel with Core 1's sensor sampling task. Actual testing shows that ESP32-S3's WiFi UDP transmission latency (from calling sendto() to data leaving the antenna) is typically below 1ms (LAN environment, AP and PC on the same switch), fully meeting real-time rendering requirements.

**7.4 Acceptance Criteria**

Communication protocol acceptance criteria ensure the stability, real-time performance, and completeness of data transmission. BLE connection maintains stability in normal use scenarios, MTU negotiation successfully reaches 512 bytes, and there are no disconnections during 24 hours of continuous operation. WiFi UDP single-frame latency in LAN environments (from ESP32-S3 transmission to upper-computer reception) is below 5ms (including total latency of WiFi transmission, network switching, and socket reception). Data frame completeness rate exceeds 99% (at 100Hz transmission frequency, allowing no more than 1 frame loss per second), and no consecutive frame drops occur (3 or more consecutive dropped frames is considered a communication anomaly). Protobuf serialization/deserialization execution time on ESP32-S3 is below 0.5ms. Dual-channel switchover time (automatic degradation from WiFi to BLE when WiFi is unavailable) is below 5 seconds.

**8 Phase 5: L2 ST-GCN Upper-Computer Inference**

Phase 5 is the advanced stage of the project's AI capabilities. The goal is to deploy a custom Spatio-Temporal Graph Convolutional Network (ST-GCN) model on the upper computer (PC) for processing complex sign language sequences that L1 cannot accurately recognize. L2 inference is not constrained by the embedded side's strict limitations on computational resources and model complexity, allowing deeper network structures and larger input windows to achieve higher recognition accuracy. The L2 model's input data format is (Batch, Time=30, Landmarks=21, Coords=2), where 21 keypoints correspond to the standard MediaPipe Hand Landmarks definition, and 2D coordinates are generated by the pseudo-skeleton mapping layer from 21-dimensional sensor features.

**8.1 Building ST-GCN from MS-GCN3 Paper**

This project's ST-GCN construction is based on the MS-GCN3 original paper ("Multi-Step Graph Convolutional Network for 3D Human Pose Estimation", published at CVPR), but with multiple adaptations and improvements for wearable sensor data characteristics. It must be emphasized that this project does not use the OpenHands library—this library has been unmaintained since 2023, its latest version does not support wearable sensor data formats (only MediaPipe keypoint coordinates as input), and its graph convolution implementation has multiple inconsistencies with the MS-GCN3 paper description. Therefore, we build the ST-GCN from scratch using PyTorch and PyTorch Geometric, starting from the paper's mathematical definitions.

The custom ST-GCN includes the following five core components: First, a Pseudo-skeleton Mapping layer (Eq.16) that uses a linear layer nn.Linear(21, 21×2) to map 21-dimensional sensor features (15 Hall + 6 IMU channels) into 2D coordinates (x, y) of 21 keypoints. This mapping layer automatically learns the optimal mapping relationship from sensor features to skeleton keypoints through backpropagation during training, without manual calibration. Second, a Spatial Graph Convolution layer that builds an adjacency matrix A ∈ R^21×21 (including self-loop connections) based on hand skeleton topology, using graph convolution operations to transfer feature information between adjacent joints. The adjacency matrix is defined according to hand anatomy: thumb (keypoints 1-4) → index (5-8) → middle (9-12) → ring (13-16) → little (17-20) → wrist (0), with each finger's MCP joint connected to the wrist.

Third, a Temporal Convolution layer using 1D dilated convolutions to capture temporal dependencies at different time scales. Dilation rates are set to [1, 2, 4], with corresponding receptive fields of 3, 5, and 9 frames, capable of simultaneously capturing short-term gesture details and long-term gesture context. Fourth, ST-Conv Blocks that combine spatial graph convolution and temporal convolution into residual blocks with the structure: Input → BN → ReLU → SpatialConv → BN → ReLU → TemporalConv → BN → + Residual Connection → ReLU. The residual connection adjusts channel dimensions through 1×1 convolution (when input and output dimensions differ), effectively mitigating gradient vanishing in deep networks. Fifth, an Attention Pooling layer that uses channel attention mechanisms (SE-Net style) to aggregate temporal features, output dimensions of (Batch, Hidden), and then maps to 46-dimensional gesture classification logits through a fully connected layer.

**8.2 Hand Skeleton Graph Definition**

The hand skeleton graph is the foundation of the ST-GCN spatial graph convolution, defining the connections (edges of the graph) between 21 keypoints. This project's keypoint definition references the MediaPipe Hand Landmarks standard, including: WRIST (ID=0), Thumb CMC (1), Thumb MCP (2), Thumb IP (3), Thumb TIP (4), Index MCP (5), Index PIP (6), Index DIP (7), Index TIP (8), Middle MCP (9), Middle PIP (10), Middle DIP (11), Middle TIP (12), Ring MCP (13), Ring PIP (14), Ring DIP (15), Ring TIP (16), Little MCP (17), Little PIP (18), Little DIP (19), Little TIP (20).

The adjacency matrix A ∈ R^21×21 is constructed as follows: diagonal elements A[i][i] = 1 (self-loop connections, allowing each node to retain its own information); adjacent joints A[i][j] = A[j][i] = 1 (undirected edges, bidirectional information propagation). Specific edge connections are: Wrist-ThumbCMC, ThumbCMC-ThumbMCP-ThumbIP-ThumbTIP (thumb chain); Wrist-IndexMCP-IndexPIP-IndexDIP-IndexTIP (index chain); Wrist-MiddleMCP-MiddlePIP-MiddleDIP-MiddleTIP (middle chain); Wrist-RingMCP-RingPIP-RingDIP-RingTIP (ring chain); Wrist-LittleMCP-LittlePIP-LittleDIP-LittleTIP (little chain). Additionally, palm-internal connections (e.g., IndexMCP-MiddleMCP-RingMCP-LittleMCP) can be added to enhance spatial information propagation. The adjacency matrix is fixed at model initialization and does not participate in training (or optionally as learnable parameters).

**8.3 Training Data Preparation**

The L2 model's training data format is (Batch, Time=30, Landmarks=21, Coords=2), where each sample contains 30 time steps, 21 keypoints, and 2D coordinates per keypoint. Data comes from sensor data collected in Phase 2, generated as skeleton keypoint sequences through the pseudo-skeleton mapping layer (or using raw sensor features directly for end-to-end training). The dataset is split into training, validation, and test sets at a 70/15/15 ratio. The split ensures that data from the same user does not appear simultaneously in both the training and test sets (Stratified Split by Subject), to evaluate the model's cross-person generalization capability.

Data augmentation strategies are key to improving model generalization. This project uses three augmentation methods: Random Temporal Shift—randomly shifting the entire gesture sequence by ±5 frames on the time axis to simulate natural variation in gesture start times; Gaussian Noise—adding N(0, 0.01) random noise to coordinate values to simulate sensor measurement errors; Temporal Masking—randomly masking 3-5 consecutive frames of data (setting to zero) to simulate occasional sensor data loss. All three augmentation methods are randomly applied with 50% probability and can be stacked. Data augmentation is applied only to the training set; validation and test sets use original data.

**8.4 Model Training Configuration**

| **Hyperparameter** | **Value** | **Description** |
|---|---|---|
| Optimizer | AdamW | Adam + Decoupled Weight Decay |
| Learning Rate | 1e-3 | Initial learning rate |
| Weight Decay | 1e-4 | Prevent overfitting |
| LR Scheduler | CosineAnnealingLR | Cosine annealing, T_max=200 |
| Loss Function | CrossEntropyLoss + LabelSmoothing(0.1) | Label smoothing prevents overfitting |
| Training Epochs | 200 epochs | Including early stopping mechanism |
| Early Stopping Patience | 20 | Stop when validation loss does not improve |
| Batch Size | 64 | When GPU memory allows |
| GPU | NVIDIA RTX 3060+ | Recommended 12GB+ VRAM |

Training uses Weights & Biases (W&B) for experiment tracking and visualization, recording each epoch's training loss, validation loss, Top-1/Top-5 accuracy, learning rate, and gradient norm. Model checkpoints are saved after each epoch. When validation set Top-1 accuracy exceeds the historical best, it is saved as best_model.pt. After training, the final model's performance is evaluated on an independent test set, generating a confusion matrix and classification report (precision, recall, F1 score). Additionally, ablation studies are conducted to verify the contribution of each component (pseudo-skeleton mapping, spatial graph convolution, temporal convolution, attention pooling).

**8.5 Acceptance Criteria**

L2 ST-GCN model acceptance criteria are as follows:

Top-1 accuracy: Top-1 accuracy for the complete 46 Chinese sign language vocabulary must be no less than 95% (evaluated on an independent test set, containing data from at least 5 different users).

Top-5 accuracy: No less than 99%.

Inference latency: Single inference latency below 10ms on GPU (RTX 3060); single inference latency below 20ms on CPU (Intel i5-12400).

Model size: FP32 model not exceeding 50MB, ONNX exported model not exceeding 25MB.

Cross-person generalization: Top-1 accuracy on unseen user data must be no less than 85% (a decrease of no more than 10 percentage points compared to Top-1 accuracy on training set users).

**9 Phase 6: NLP Grammar Correction & TTS**

Phase 6 aims to convert gesture recognition results (discrete gesture vocabulary sequences) into natural, fluent Chinese sentences, and perform voice output through a Text-to-Speech (TTS) module. This phase is the core interaction link between the system and end users, directly impacting user experience and system practicality. Sign language translation is not simple "word-for-word" translation but a natural language processing task involving grammatical structure conversion and semantic understanding. Chinese Sign Language (CSL) and Mandarin Chinese have significant differences in grammatical structure, requiring a dedicated grammar correction module for conversion.

**9.1 Sign Language Grammar Characteristics Analysis**

As a visual-spatial language, Chinese Sign Language (CSL) has fundamental differences in grammatical structure from spoken languages (such as Mandarin Chinese). The most significant difference is word order: CSL primarily uses SOV (Subject-Object-Verb) order, while Mandarin Chinese uses SVO (Subject-Verb-Object) order. For example, the sign language expression "I-apple-eat" corresponds to the Chinese "I eat apples" (我吃苹果). However, CSL's grammatical rules extend far beyond simple word order rearrangement—CSL also has the following characteristics: Topic-prominent Structure, where the topic is frequently placed at the beginning of the sentence followed by a comment; time/location adverbials placed before the verb (e.g., "yesterday-school-I-go" corresponds to Chinese "Yesterday I went to school"); negation words placed before the verb (e.g., "not-go" corresponds to Chinese "not go" or "did not go"); and rich facial expressions and body postures as grammatical markers (e.g., questioning expression indicates an interrogative sentence, head-shaking indicates negation).

This system's NLP module needs to achieve reasonable conversion from CSL grammar to Chinese grammar under limited computational resources (running in the upper-computer's Python environment, without relying on large cloud-based NLP models). The initial goal is to handle the most common grammatical differences (SOV→SVO word order conversion, negation word position adjustment, tense marker addition), with potential future expansion to a more comprehensive Transformer-based grammar correction model.

**9.2 Grammar Correction Strategy**

The grammar correction module uses a "rule base + model" hybrid strategy, implemented in three layers:

Base layer: Template matching based on a rule base. Establish an SOV→SVO word order conversion template library containing the 50-100 most common sentence patterns. For example, when a "noun-noun-verb" pattern is detected, automatically rearrange to "noun-verb-noun." Negation word position templates: when a negation word is detected after the verb, move it before the verb. Tense templates: add time words (e.g., "了", "过", "正在") based on context. The base layer's advantages are speed (O(n) complexity), strong interpretability, and no training data requirement; its disadvantage is limited coverage, unable to handle complex nested sentence structures.

Advanced layer: Lightweight Transformer model. Using DistilBERT or similar small pre-trained models (~66M parameters), fine-tuned on CSL→Chinese parallel corpora. Model input is the gesture recognition result sequence (gesture IDs or vocabulary separated by spaces), and output is the grammar-corrected Chinese sentence. The advanced layer can handle complex grammatical phenomena not covered by the base rule base but requires a certain scale of parallel corpora (recommended at least 5,000 CSL-Chinese parallel sentence pairs).

Context layer: Sliding window sentence-level correction. Maintain a sliding window of the most recent N gestures (N=50), performing cross-sentence context correction within the window. For example, if the previous gesture is "you" and the next gesture is "thank you," the system can infer this is a Q&A scenario and output "Thank you." Additionally, a sign language vocabulary → Chinese vocabulary mapping dictionary is established (containing Chinese translations of approximately 500 common sign language vocabulary entries), used to convert gesture IDs to Chinese vocabulary.

**9.3 TTS Implementation**

The TTS module uses Microsoft's edge-tts library, which is based on Azure Cognitive Services' neural network speech synthesis technology, completely free and requiring no API key (using Microsoft Edge browser's online TTS service). edge-tts supports multiple high-quality Chinese voices: zh-CN-XiaoxiaoNeural (female, natural and fluent, suitable for daily scenarios), zh-CN-YunxiNeural (male, warm and friendly), zh-CN-XiaoyiNeural (female, lively and cute). Users can select preferred voice and speech rate in settings.

TTS uses an asynchronous queue architecture to avoid blocking the main thread. When the grammar correction module outputs a Chinese sentence, the sentence is pushed to the TTS task queue. A background thread takes sentences from the queue and calls edge-tts to generate speech audio (MP3 format), then plays it through the system's audio output device. Voice audio for common phrases (e.g., "Hello", "Thank you", "Goodbye" and other high-frequency sign language vocabulary) is pre-cached to memory at system startup, avoiding the need to call the edge-tts API each time, thereby reducing TTS latency for common phrases to near zero. edge-tts typical latency is 200-500ms (depending on sentence length and network conditions), and for general sign language conversation scenarios (sentence length 5-15 characters), end-to-end latency is fully within acceptable range.

**9.4 Acceptance Criteria**

Phase 6 acceptance criteria ensure that the NLP and TTS modules provide users with a smooth, natural sign language translation experience. Grammar correction module accuracy for common CSL sentence patterns must be no less than 85% (evaluated on a manually annotated test set containing 200 CSL-Chinese parallel sentences). TTS latency—from grammar correction completion to speech playback start must be below 500ms (below 50ms for common phrases). Speech naturalness—TTS output voice naturalness MOS (Mean Opinion Score) must be no less than 4.0 (5-point scale, averaged from ratings by 10+ evaluators). End-to-end translation latency—from gesture completion (L2 inference output) to speech playback start must be below 1 second.

**10 Phase 7: Rendering Layer Development**

Phase 7 is the project's visual presentation layer development stage. The goal is to present sensor data and gesture recognition results to users in real-time 3D visualization. The rendering layer is not just the system's "face" but also a key tool for verifying the data glove's correct operation—by observing whether the 3D hand model matches actual hand movements, developers can quickly identify issues in sensor calibration, feature mapping, and model inference. This phase's rendering solution follows a three-stage progressive evolution strategy, gradually improving visual quality and interaction experience from rapid MVP to professional-grade rendering.

**10.1 Phase 2 MVP: Tauri + R3F**

The Tauri + R3F solution is the first implementation of the rendering layer, aiming to complete an end-to-end 3D visualization prototype in the shortest development cycle (estimated 2-3 weeks). The technology stack includes: Tauri 2.0 (Rust-based desktop application framework, responsible for system-level API access), React (frontend UI framework), R3F (React Three Fiber, React wrapper for Three.js), Zustand (lightweight state management library), and TailwindCSS (styling framework). The Rust backend is responsible for listening to ESP32-S3's BLE serial data and WiFi UDP data, parsing Protobuf messages, and pushing parsed sensor data to the frontend through Tauri's event system.

The core design philosophy of the frontend architecture is "data-driven rendering, separation of state and view." The Zustand store maintains the latest sensor data (21-dimensional feature vector) and gesture recognition results (gesture ID, confidence), using transient update mechanisms (zustand/temporal) to avoid frequent data updates triggering React re-renders. Three.js's useFrame hook directly reads the latest data from the store and updates skeleton rotations, completely bypassing React's render cycle, ensuring that the 60fps rendering frame rate is not affected by React rendering overhead. The 3D hand model uses a simplified skeletal mesh (wireframe model based on 21 keypoints and connecting line segments), with each joint represented by a sphere and bones connected by cylinders. The UI panel uses the shadcn/ui component library, displaying current gesture, confidence, connection status, sensor values, etc.

**10.2 Phase 2.5: Three.js + WebSocket (Browser)**

The Phase 2.5 solution migrates 3D rendering to a pure browser environment, eliminating the barrier of desktop application installation. Users simply open a browser and visit a specified URL to see real-time 3D hand visualization. The technical architecture is: a Python/Node.js WebSocket server receives ESP32-S3's BLE/WiFi data, parses Protobuf messages, and pushes to all connected browser clients via WebSocket. The browser side uses native Three.js (no React wrapper) to directly render the 3D hand model.

The advantage of this solution is extremely simple deployment—the WebSocket server can run on any machine with a Python/Node.js environment (including embedded Linux devices like Raspberry Pi), and the user side only needs a modern browser (Chrome, Firefox, Edge) to access. This is particularly valuable for demonstration scenarios and remote debugging—developers can remotely view the data glove's working status without installing the Tauri application locally. The disadvantage is that WebSocket data transmission latency is slightly higher than Tauri's IPC channel (adding approximately 1-3ms), but this has no practical impact on 3D rendering frame rates above 30fps.

**10.3 Phase 4+: Unity + ms-MANO**

The Unity + ms-MANO solution is the final form of the rendering layer, providing photorealistic-quality hand rendering and precise joint control. The technology stack includes: Unity 2022.3 LTS (long-term support version, ensuring stability), XR Hands Package (Unity's official hand tracking SDK), and ms-MANO parametric hand model (Multi-Scale extension of the MANO hand model). ms-MANO uses 48-dimensional pose_parameters (15 joints × 3D rotation + 3D global translation + 3D global rotation) to control the hand's complete pose, capable of generating high-fidelity 3D hand meshes (778 vertices, 1538 triangular faces).

Mapping sensor data to ms-MANO parameters is the core technical challenge of this phase. The mapping strategy is as follows: 15-channel TMAG5273 Hall data is mapped to 15 finger joint bending angles (5 fingers × 3 joints: MCP/PIP/DIP) through a calibration matrix. The calibration process uses least squares fitting to establish a linear relationship between sensor readings and actual joint angles. BNO085 wrist quaternion data is directly mapped to ms-MANO's global rotation parameters. Finger MCP joints also support Abduction/Adduction motion, but TMAG5273 cannot directly measure this degree of freedom, requiring indirect estimation through adjacent finger Hall data.

Data smoothing is key to ensuring rendering fluidity. This project uses two smoothing techniques: Quaternion.Slerp (spherical linear interpolation) for smoothing quaternion rotations, with interpolation factor α = 0.3 (i.e., 30% of target value + 70% of current value), effectively eliminating model jitter caused by sensor noise. Vector3.Lerp (linear interpolation) is used for smoothing position changes. Additionally, velocity-based adaptive smoothing is implemented—when rapid gesture switching is detected (joint angle change between two frames exceeds a threshold), the smoothing factor is automatically reduced to maintain response speed; when gestures are steady, the smoothing factor is increased to improve visual stability.

**10.4 Acceptance Criteria**

Rendering layer acceptance criteria ensure the fluidity, accuracy, and visual quality of 3D visualization.

3D rendering frame rate: Maintain rendering frame rate above 30fps on mainstream hardware (Intel i5 + integrated graphics), and achieve 60fps on discrete graphics.

End-to-end hand motion latency: Latency from sensor acquisition to hand model response must be below 50ms (including total latency of sensor sampling, communication transmission, data parsing, and rendering pipeline).

Visual naturalness: 3D hand model motion naturalness score (5-point scale, rated by 10+ evaluators) must be no less than 4.0, requiring finger bending, wrist rotation, and other movements to appear natural and smooth, without obvious jitter or mesh clipping.

Three-stage rendering consistency: 3D hand movements across Tauri, Three.js, and Unity versions must remain consistent, with the same sensor input producing the same rendering results (allowing ±5% visual variation).

**11 Phase 8: System Integration & Testing**

Phase 8 is the final integration and verification stage of the project. The goal is to integrate the modules developed in the previous seven phases into a complete end-to-end system and verify the system's overall performance, stability, and usability through systematic testing. System integration is not simple module assembly—it requires resolving interface compatibility issues between modules, timing synchronization issues, resource contention issues, and exception handling issues. Testing in this phase covers four dimensions: functional testing, performance testing, user testing, and stress testing, ensuring the system operates stably and reliably in real-world usage scenarios.

**11.1 End-to-End Test Flow**

End-to-end testing verifies the complete pipeline from sensor data acquisition to final output: Sensor acquisition (5 TMAG5273 + BNO085) → I2C read → Kalman filtering → Data normalization → Sliding window buffer → L1 inference → Confidence determination → L1 output or L2 forward → BLE/WiFi transmission → Protobuf parsing → L2 inference (if triggered) → NLP grammar correction → TTS speech synthesis → Audio playback. Simultaneously, sensor data is sent to the rendering side via WiFi UDP, parsed and used to drive real-time 3D hand model updates. The test set includes 46 gesture categories, each tested 20 times, recording recognition results, response times, and confidence for each attempt. Continuous sign language testing uses 10 predefined sign language sentence groups (each containing 3-5 gestures), testing the system's recognition accuracy and latency performance under continuous input scenarios.

**11.2 Performance Benchmark Testing**

| **Test Item** | **Target Metric** | **Test Method** |
|---|---|---|
| L1 Inference Latency | < 3ms | ESP32-S3 GPIO toggle timing |
| L2 Inference Latency (GPU) | < 10ms | PyTorch CUDA Event timing |
| End-to-End Latency (L1) | < 100ms | Gesture completion → text display timing |
| End-to-End Latency (L2) | < 500ms | Gesture completion → text display timing |
| BLE Transmission Latency | < 20ms | Timestamp difference method |
| UDP Transmission Latency | < 5ms | Timestamp difference method |
| Recognition Accuracy (L1) | > 90% | 46 classes × 20 times confusion matrix |
| Recognition Accuracy (L2) | > 95% | 46 classes × 20 times confusion matrix |
| Operating Power Consumption | < 150mW | INA219 power meter measurement |
| Battery Life | > 12h | 600mAh battery discharge test |

**11.3 User Testing**

User testing is the key step in verifying the system's actual usability. Test participants include 5-10 hearing-impaired individuals (Chinese sign language users) and 5 hearing individuals (as communication partners). Test scenarios simulate real sign language translation situations: hearing-impaired users wear the data glove and express predefined sentences through sign language (covering daily greetings, shopping, medical visits, transportation, etc.). The system translates sign language into Chinese text and speech. The hearing communication partner receives the translation results and responds. Test metrics include: task completion rate (number of successfully translated sentences / total sentences), translation accuracy (number of correctly translated sentences / number of translated sentences), user satisfaction (5-point Likert scale), and system usability score (SUS, System Usability Scale). Additionally, A/B testing is conducted—comparing the system's translation results with human translation results to evaluate the gap between machine and human translation.

**11.4 Stress Testing**

Stress testing verifies system stability and robustness under extreme conditions. Long-duration operation stability testing—the system runs continuously for 72 hours, with functional tests every 4 hours, checking for memory leaks, accuracy degradation, or communication disconnections. BLE/WiFi connection stability testing—testing BLE and WiFi connection stability in electromagnetic interference environments (near WiFi routers, microwave ovens, Bluetooth speakers, etc.), recording disconnection counts and reconnection times. Battery consumption curve testing—using electronic loads to simulate actual usage scenarios (30% sampling, 50% inference, 20% communication), recording voltage and current changes over time, calculating actual battery life and battery aging. Temperature testing—testing system performance in the 0°C to 45°C ambient temperature range, ensuring sensor accuracy and inference accuracy do not significantly degrade due to temperature changes.

**12 Known Issues & Risks**

The following lists key issues and potential risks identified during project development, along with corresponding mitigation strategies. These issues and risks require continuous monitoring and tracking throughout the project advancement, with timely preventive measures and corrective actions.

| **ID** | **Issue Description** | **Impact Scope** | **Mitigation Strategy** |
|---|---|---|---|
| R-01 | L2 ST-GCN implementation in the paper is a pseudo-implementation (nn.Linear rather than real graph convolution) | Phase 5, L2 Inference | Custom ST-GCN approach planned; re-implementing from MS-GCN3 paper |
| R-02 | FreeRTOS xTaskCreatePinnedToCore parameter order bug (function pointer and handle swapped) | Phase 1, System Stability | Fixed and documented. Added compile-time static assertions to validate function pointer |
| R-03 | OpenHands library is unmaintained and does not support wearable sensor data formats | Phase 5, Model Development | OpenHands deprecated; using PyTorch Geometric for custom ST-GCN |
| R-04 | BOM cost discrepancy ($18 vs $38.60); paper's $18 is bulk/die pricing | Cost Control, Project Budget | Actual $38.60 applies; optimize PCB design to reduce costs |
| R-05 | BLE bandwidth limitation (~100KB/s), unable to meet 100Hz high-frequency data transmission | Phase 4+, Real-time Rendering | WiFi UDP introduced in Phase 4+; BLE downgraded to provisioning + low-speed backup |
| R-06 | NLP grammar correction module is oversimplified; rule base coverage is limited | Phase 6, Translation Quality | Phased implementation: rule base → lightweight Transformer → context correction |
| R-07 | I2C bus data corruption may occur during 5-channel TMAG5273 switching | Phase 1, Data Quality | Added stabilization wait time (1ms) after channel switching; added CRC verification |
| R-08 | ms-MANO hand model pose space to sensor data space mapping accuracy | Phase 7, Rendering Quality | Establish optimal mapping matrix through calibration experiments; use nonlinear fitting |
| R-09 | Insufficient cross-person generalization; model accuracy drops on new users | Phase 5, Model Performance | Increase training data diversity; use Domain Adaptation techniques |
| R-10 | edge-tts depends on network; unavailable in offline environments | Phase 6, TTS Availability | Pre-cache common phrases; integrate local lightweight TTS engine as backup |

Risk tracking mechanism: Each risk item is tracked and managed by a designated Risk Owner, with risk status (new/active/mitigated/closed) updated weekly at project meetings. When a risk's impact scope expands or mitigation strategy needs adjustment, the Risk Owner must initiate a risk assessment meeting to reformulate the mitigation plan. High-risk items (such as R-01, R-03, R-04) require dedicated review at project milestones to ensure risks are effectively controlled.

**13 Appendices**

**Appendix A: Complete BOM Hardware List**

| **No.** | **Component** | **Model/Specification** | **Qty** | **Unit Price (USD)** | **Subtotal (USD)** | **Supplier** |
|---|---|---|---|---|---|---|
| 1 | MCU Dev Board | ESP32-S3-DevKitC-1 N16R8 | 1 | $13.50 | $13.50 | Espressif |
| 2 | 3D Hall Sensor | TMAG5273A1 | 5 | $2.80 | $14.00 | TI |
| 3 | 9-axis IMU | BNO085 | 1 | $9.50 | $9.50 | Bosch |
| 4 | I2C Multiplexer | TCA9548A | 1 | $0.85 | $0.85 | TI |
| 5 | Lithium Battery | 600mAh 3.7V LiPo | 1 | $3.50 | $3.50 | Generic |
| 6 | Battery Protection Board | TP4056 + DW01A | 1 | $0.80 | $0.80 | Generic |
| 7 | USB-UART | CH340C | 1 | $0.50 | $0.50 | WCH |
| 8 | Voltage Regulator | ME6211C33 (3.3V) | 2 | $0.20 | $0.40 | Microne |
| 9 | Pull-up Resistors | 2.2kΩ/4.7kΩ 0402 | 20 | $0.01 | $0.20 | Yageo |
| 10 | Decoupling Capacitors | 100nF 0402 | 20 | $0.01 | $0.20 | Yageo |
| 11 | Ribbon Cable/Connectors | FPC + JST-PH | 1 set | $1.50 | $1.50 | Generic |
| 12 | PCB | Two-layer board 5×5cm | 1 | $2.00 | $2.00 | JLCPCB |
| 13 | 3D Printed Enclosure | TPU/PLA | 1 | $1.50 | $1.50 | In-house |

**Total: Approximately $38.60**

**Appendix B: ESP32-S3 Pin Assignment Table**

| **Pin** | **Function** | **Peripheral** | **Notes** |
|---|---|---|---|
| GPIO 8 (SDA) | I2C Data Line | TMAG5273×5, BNO085, TCA9548A | Main I2C Bus |
| GPIO 9 (SCL) | I2C Clock Line | TMAG5273×5, BNO085, TCA9548A | 400kHz Fast Mode |
| GPIO 21 | BNO085 INT | External Interrupt | Data ready interrupt |
| GPIO 4 | ADC1_CH3 | Flex Sensor 1 (reserved) | Thumb |
| GPIO 5 | ADC1_CH4 | Flex Sensor 2 (reserved) | Index |
| GPIO 6 | ADC1_CH5 | Flex Sensor 3 (reserved) | Middle |
| GPIO 7 | ADC1_CH6 | Flex Sensor 4 (reserved) | Ring |
| GPIO 15 | ADC2_CH4 | Flex Sensor 5 (reserved) | Little |
| GPIO 43 | UART TX | Debug Serial Port | USB-UART CH340C |
| GPIO 44 | UART RX | Debug Serial Port | USB-UART CH340C |
| GPIO 19 | BLE TX/RX | BLE Antenna | Built-in antenna |

**Appendix C: Protobuf Message Definitions**

The following are all Protobuf message definitions used in this project, divided into two parts: sensor data frame and configuration frame.

```protobuf
// glove_data.proto - Sensor Data Frame
syntax = "proto3";

message GloveData {
  uint32 timestamp = 1;              // System timestamp (ms)
  repeated float hall_features = 2;   // 15 channels
  repeated float imu_features = 3;    // 6 channels
  repeated float flex_features = 4;   // 5 channels (reserved)
  uint32 l1_gesture_id = 5;
  uint32 l1_confidence_x100 = 6;
}

// glove_config.proto - Configuration Frame
message GloveConfig {
  uint32 sample_rate = 1;            // Sampling frequency
  bool kalman_enable = 2;            // Kalman filter toggle
  float kalman_q = 3;                // Process noise
  float kalman_r = 4;                // Measurement noise
  float confidence_threshold = 5;    // L1 confidence threshold
}
```

**Appendix D: FreeRTOS Task Allocation Table**

| **Task** | **Function** | **Core** | **Priority** | **Stack Size** | **Trigger Method** | **Period** |
|---|---|---|---|---|---|---|
| Sensor Sampling | Task_SensorRead | Core 1 | 3 | 4096B | Timer (10ms) | 100Hz |
| Edge Inference | Task_Inference | Core 0 | 2 | 8192B | TaskNotify | On-demand |
| Communication | Task_Comms | Core 0 | 1 | 4096B | Semaphore | 100Hz |
| IDLE | idle task | Core 0/1 | 0 | 4096B | System | Automatic |

**Appendix E: Glossary**

| **Term/Abbreviation** | **Full English Name** | **Chinese Translation** |
|---|---|---|
| ADR | Architecture Decision Record | 架构决策记录 |
| BLE | Bluetooth Low Energy | 低功耗蓝牙 |
| BOM | Bill of Materials | 物料清单 |
| CSL | Chinese Sign Language | 中国手语 |
| EON | Edge Optimized Neural | 边缘优化神经网络 |
| HAL | Hardware Abstraction Layer | 硬件抽象层 |
| IMU | Inertial Measurement Unit | 惯性测量单元 |
| I2C | Inter-Integrated Circuit | 内部集成电路总线 |
| MCU | Micro Controller Unit | 微控制器单元 |
| MOS | Mean Opinion Score | 平均意见评分 |
| MVP | Minimum Viable Product | 最小可行产品 |
| PCB | Printed Circuit Board | 印刷电路板 |
| PSRAM | Pseudo Static RAM | 伪静态随机存取存储器 |
| R3F | React Three Fiber | React 3D渲染库 |
| ST-GCN | Spatio-Temporal GCN | 时空图卷积网络 |
| TFLite | TensorFlow Lite | 轻量级推理引擎 |
| TTS | Text-to-Speech | 文本转语音 |
| MANO | Hand Model | 参数化手部模型 |
| NVS | Non-Volatile Storage | 非易失性存储 |
| OTA | Over-The-Air | 空中升级 |
