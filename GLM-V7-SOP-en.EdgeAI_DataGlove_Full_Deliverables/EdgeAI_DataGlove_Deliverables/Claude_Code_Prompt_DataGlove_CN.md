+-----------------------------------------------------------------------+
| ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━                              |
|                                                                       |
| **Claude Code Prompt**\                                               |
| **工 程 手 册**                                                       |
|                                                                       |
| ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━                              |
|                                                                       |
| Edge AI 数据手套项目                                                  |
|                                                                       |
| 分阶段可执行提示词文档                                                |
|                                                                       |
| **配套文档：** SOP SPEC PLAN v2.0                                     |
|                                                                       |
| **MCU：** ESP32-S3-DevKitC-1 N16R8                                    |
|                                                                       |
| **AI 框架：** Edge Impulse / TFLite Micro / PyTorch                   |
|                                                                       |
| **日 期：** 2026-04-16                                                |
|                                                                       |
| Version 1.0 · Internal Use Only                                       |
+=======================================================================+
+-----------------------------------------------------------------------+

**▎ 目 录**

-------------------------------------------------------------------------------
  **1**                   项目初始化与环境配置            P0.1 -- P0.2
----------------------- ------------------------------- -----------------------
  **2**                   Phase 1：HAL 与驱动层           P1.1 -- P1.4

  **3**                   Phase 2：信号处理               P2.1 -- P2.2

  **4**                   Phase 3：L1 边缘推理            P3.1 -- P3.3

  **5**                   Phase 4：通信协议               P4.1 -- P4.2

  **6**                   Phase 5：L2 ST-GCN 推理         P5.1 -- P5.3

  **7**                   Phase 6-7：NLP 语法纠错与渲染   P6.1 -- P7.1

  **8**                   Phase 8：集成测试               P8.1

  **9**                   Bug 修复专用 Prompts            P9.1 -- P9.2

**▎ 1 项目初始化与环境配置**

本节包含项目初始化阶段的所有提示词，涵盖 PlatformIO 嵌入式项目和 Python 上位机项目的创建与配置。在开始任何硬件驱动或算法开发之前，必须先完成本节的所有任务，确保开发环境正确搭建。

**P0.1 初始化 PlatformIO 项目**

**💡 上下文说明：***ESP32-S3-DevKitC-1 N16R8 版本需要正确配置 PSRAM 和内存模式，否则无法运行 TensorFlow Lite Micro 等内存密集型任务。本 Prompt 确保项目骨架和编译环境一次性正确建立。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

--------------------------------------------------------------------------------
  请初始化一个PlatformIO项目，目标平台为ESP32-S3-DevKitC-1 (N16R8, 16MB PSRAM)。

  要求：

  1. 在platformio.ini中配置:

     - platform = espressif32 @ \^6.5.0

       - board = esp32-s3-devkitc-1-n8r8 (注意: 我们的板子是N16R8版本)

       - framework = arduino

         - board_build.psram = enable (启用外部PSRAM)

         - board_build.arduino.memory_type = qio_opi (PSRAM模式)

           - monitor_speed = 115200

           - build_flags = 

       -DBOARD_HAS_PSRAM

       -DCORE_DEBUG_LEVEL=3

  2. 创建以下目录结构:

     - src/ (主源码)

       - lib/Sensors/ (传感器驱动)

       - lib/Filters/ (滤波算法)

         - lib/Comms/ (通信模块)

         - lib/Models/ (AI模型)

           - include/ (头文件)

           - test/ (测试)

  3. 验证编译通过，无错误。
--------------------------------------------------------------------------------

**🎯 预期输出：**

• platformio.ini 配置文件，包含完整的 ESP32-S3 N16R8 PSRAM 配置

• 完整的目录结构，所有 lib/ 子目录已创建

• 编译输出 0 errors, 0 warnings（或仅有已知可忽略的 warning）

**✅ 验收标准：**

☑ platformio.ini 包含 board_build.psram = enable

☑ 目录结构包含 lib/Sensors, lib/Filters, lib/Comms, lib/Models, include, test

☑ pio run 编译通过

───────────────────────────────────────────────────────────────────────────

**P0.2 初始化 Python 上位机项目**

**💡 上下文说明：***上位机负责数据采集、L2 ST-GCN 推理、NLP 语法纠错和 TTS 语音合成。需要预先建立规范的目录结构和依赖管理，避免后期依赖冲突。Python 版本要求 3.9+ 以兼容 PyTorch 2.0 和 edge-tts。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请初始化Python上位机项目，创建以下结构和依赖:

  1. 创建项目目录结构:

     - glove_upper/ (项目根)

         - src/

           - sensors/ (传感器数据处理)

             - models/ (ST-GCN模型)

             - nlp/ (语法纠错)

               - tts/ (语音合成)

               - rendering/ (渲染接口)

                 - utils/ (工具函数)

               - data/ (数据存储)

                 - configs/ (配置文件)

                 - scripts/ (独立脚本)

  2. 创建requirements.txt:

     torch\>=2.0.0

     torchvision\>=0.15.0

     numpy\>=1.24.0

     protobuf\>=4.24.0

     edge-tts\>=6.1.0

     asyncio

     websockets\>=11.0

     pyserial\>=3.5

  3. 创建pyproject.toml配置Python 3.9+兼容性。
-----------------------------------------------------------------------

**🎯 预期输出：**

• 完整的项目目录结构

• requirements.txt 包含所有核心依赖及版本约束

• pyproject.toml 配置 Python 3.9+ 和项目元数据

• \_\_init\_\_.py 文件在每个子包中

**✅ 验收标准：**

☑ 所有目录存在且包含 \_\_init\_\_.py

☑ requirements.txt 可成功 pip install -r

☑ python -c \"import torch; print(torch.\_\_version\_\_)\" 正常输出

───────────────────────────────────────────────────────────────────────────

**▎ 2 Phase 1：HAL 与驱动层**

本节覆盖所有底层硬件驱动开发，包括 I2C 多路复用器、3D 霍尔传感器、9 轴 IMU 以及 FreeRTOS 双核任务调度。这些驱动是整个系统的数据基石，任何 bug 都会传播到上层。特别注意 P1.4 中记录的已知 FreeRTOS 参数顺序 Bug。

+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| **⚠️ 已知 Bug 警告：FreeRTOS xTaskCreatePinnedToCore 参数顺序**                                                                                                                                               |
|                                                                                                                                                                                                               |
| 原始代码中将函数指针参数和句柄参数搞反了。错误写法：第一个参数传入了句柄变量 TaskSensorReadHandle 而不是函数指针 Task_SensorRead。此 Bug 会导致编译通过但运行时崩溃或任务不执行。P9.1 提供了完整修复 Prompt。 |
+===============================================================================================================================================================================================================+
+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

**P1.1 TCA9548A I2C 多路复用器驱动**

**💡 上下文说明：***数据手套使用 5 个 TMAG5273 霍尔传感器，它们共享同一 I2C 总线（地址相同），必须通过 TCA9548A 多路复用器切换通道来逐一访问。如果多路复用器驱动存在 I2C 时序问题，将导致所有传感器数据采集失败。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现TCA9548A I2C多路复用器驱动，文件路径: lib/Sensors/TCA9548A.h

  要求：

  1. 类名: TCA9548A

  2. 构造函数: TCA9548A(TwoWire \*wire, uint8_t addr = 0x70)

  3. 核心方法:

     - bool begin() - 初始化,验证I2C通信

       - bool selectChannel(uint8_t ch) - 选择通道0-7

       - bool disableAll() - 禁用所有通道

  4. I2C速率: 400kHz (Wire.setClock(400000))

  5. 错误处理: 每次I2C操作后检查返回值

  6. 在.cpp中实现所有方法

  7. 在test/目录创建单元测试test_tca9548a.cpp

  已知约束:

  - TCA9548A地址为0x70

  - 需要在每次TMAG5273读取前调用selectChannel()

  - 主总线上拉电阻2.2kΩ,子通道4.7kΩ
-----------------------------------------------------------------------

**🎯 预期输出：**

• lib/Sensors/TCA9548A.h - 类声明

• lib/Sensors/TCA9548A.cpp - 方法实现

• test/test_tca9548a.cpp - 单元测试框架

**✅ 验收标准：**

☑ begin() 返回 true，I2C 通信验证通过

☑ selectChannel(0-7) 均返回 true

☑ disableAll() 正确关闭所有通道

☑ I2C 速率设置为 400kHz

☑ 每次 I2C 操作后有错误检查

───────────────────────────────────────────────────────────────────────────

**P1.2 TMAG5273 3D 霍尔传感器驱动**

**💡 上下文说明：***TMAG5273A1 是核心位置传感器，每根手指安装 1 个（共 5 个），通过检测 N52 磁铁的磁场变化来计算关节弯曲角度。12-bit 分辨率、±40mT 量程，需要 32 次平均来降低噪声。传感器通过 TCA9548A 通道 0-4 接入。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

--------------------------------------------------------------------------------
  请实现TMAG5273A1 3D霍尔传感器驱动，文件路径: lib/Sensors/TMG5273.h

  要求：

  1. 类名: TMAG5273

  2. 构造函数: TMAG5273(TwoWire \*wire, uint8_t mux_channel, TCA9548A \*mux)

  3. 核心方法:

     - bool begin() - 初始化传感器

         - 设置CONV_MODE为触发模式(0x01)

         - 设置CONFIG寄存器: 感应范围±40mT

           - 设置AVG为32次平均(降低噪声)

         - bool readXYZ(float &x, float &y, float &z) - 读取12-bit原始数据并转换为mT

           - bool triggerConversion() - 触发一次ADC转换

  4. 寄存器定义:

     - DEVICE_ID = 0x00 (期望值0x11)

       - X_MSB = 0x01, X_LSB = 0x02

       - Y_MSB = 0x03, Y_LSB = 0x04

         - Z_MSB = 0x05, Z_LSB = 0x06

         - CONV_MODE = 0x0D

           - CONFIG = 0x0E

  5. 数据转换: 12-bit signed, LSB = 0.048mT

  6. 关键: 每次读取前必须通过TCA9548A选择正确通道

  已知约束:

  - 5个传感器分别连接TCA9548A通道0-4

  - 磁铁为N52 4x2mm圆盘,安装在近节指骨

  - 传感器安装在MCP关节背侧
--------------------------------------------------------------------------------

**🎯 预期输出：**

• lib/Sensors/TMG5273.h - 类声明，含寄存器常量

• lib/Sensors/TMG5273.cpp - 完整驱动实现

• readXYZ() 正确进行 12-bit signed → mT 转换

• 每次读取前调用 mux-\>selectChannel()

**✅ 验收标准：**

☑ begin() 通过 DEVICE_ID 验证（期望值 0x11）

☑ CONV_MODE 设为触发模式，AVG 设为 32 次平均

☑ readXYZ() 返回 3 轴磁场值（mT）

☑ 5 个实例分别对应通道 0-4 且互不干扰

───────────────────────────────────────────────────────────────────────────

**P1.3 BNO085 9 轴 IMU 驱动**

**💡 上下文说明：***BNO085 提供手部整体姿态信息（四元数 + 陀螺仪），安装在手背中心。使用 Hillcrest Labs 的 SH-2 传感器融合算法，输出 Game Rotation Vector（不受磁场干扰，适合室内使用）。必须正确实现 SH-2 协议才能获取数据。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现BNO085 9轴IMU驱动，文件路径: lib/Sensors/BNO085.h

  要求：

  1. 类名: BNO085

  2. 构造函数: BNO085(TwoWire \*wire, uint8_t addr = 0x4A)

  3. 核心方法:

     - bool begin() - 初始化

         - 软复位

         - 启用SH2_GAME_ROTATION_VECTOR报告(100Hz)

           - 启用SH2_GYROSCOPE_CALIBRATED报告(100Hz)

         - bool getQuaternion(float &w, float &x, float &y, float &z)

           - bool getGyro(float &x, float &y, float &z)

           - bool getEuler(float &roll, float &pitch, float &yaw)

  4. SH-2协议实现:

     - 报告头解析

       - 产品ID查询

       - 特征报告使能

  5. 四元数到欧拉角转换(内置方法)

  已知约束:

  - BNO085安装在手背中心,必须与手掌平面平行

  - 使用硬件传感器融合,输出Game Rotation Vector(无磁场干扰)

  - I2C地址: 0x4A (SDO接GND)

  - 必须配备0.1uF+10uF去耦电容
-----------------------------------------------------------------------

**🎯 预期输出：**

• lib/Sensors/BNO085.h - 类声明

• lib/Sensors/BNO085.cpp - SH-2 协议实现

• getQuaternion() 返回归一化四元数 (w,x,y,z)

• getEuler() 返回 (roll, pitch, yaw) 单位为度

**✅ 验收标准：**

☑ begin() 成功软复位并启用 Game Rotation Vector 报告

☑ SH-2 报告头解析正确（2字节头 + 可变长度 payload）

☑ getQuaternion() 返回值满足 w²+x²+y²+z² ≈ 1.0

☑ getEuler() 输出范围: roll\[-180,180\], pitch\[-90,90\], yaw\[-180,180\]

───────────────────────────────────────────────────────────────────────────

**P1.4 FreeRTOS 双核任务调度（含已知 Bug 修复）**

**💡 上下文说明：***ESP32-S3 拥有双核（Core 0: Protocol CPU, Core 1: Application CPU）。本 Prompt 利用 FreeRTOS 将传感器采集（100Hz 实时性要求）放在 Core 1，推理和通信放在 Core 0，实现真正的并行处理。⚠️ 原始代码存在 xTaskCreatePinnedToCore 参数顺序 Bug，必须修复。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

------------------------------------------------------------------------------------------------------------------
  请实现基于FreeRTOS的双核任务调度框架，文件路径: src/main.cpp

  ⚠️ 已知Bug修复: 原始代码xTaskCreatePinnedToCore参数顺序错误!

  错误写法: xTaskCreatePinnedToCore(TaskSensorReadHandle, \"SensorRead\", 4096, NULL, 3, &TaskSensorReadHandle, 1)

  正确写法: xTaskCreatePinnedToCore(Task_SensorRead, \"SensorRead\", 4096, NULL, 3, &TaskSensorReadHandle, 1)

  参数顺序: (函数指针, 任务名, 栈大小, 参数, 优先级, 句柄指针, 核心编号)

  要求：

  1. 定义三个FreeRTOS任务:

     - Task_SensorRead (Core 1, 优先级3, 栈4096):

       100Hz采样循环,读取所有传感器数据

       使用vTaskDelayUntil确保精确100Hz

     - Task_Inference (Core 0, 优先级2, 栈8192):

       从队列接收传感器数据

       执行L1推理(Edge Impulse或TFLite Micro)

     - Task_Comms (Core 0, 优先级1, 栈8192):

       BLE/WiFi通信

       Protobuf序列化与发送

  2. FreeRTOS队列:

     - inferenceQueue: 传感器数据→推理任务 (深度10)

       - dataQueue: 推理结果→通信任务 (深度10)

  3. 数据结构:

     - FullDataPacket: timestamp + SensorData(hall_xyz\[15\] + euler\[3\] + gyro\[3\]) + flex\[5\]

       - InferenceResult: FullDataPacket + gesture_id + confidence

  4. 在setup()中创建任务和队列。
------------------------------------------------------------------------------------------------------------------

**🎯 预期输出：**

• src/main.cpp - 包含正确的任务创建代码（Bug 已修复）

• 数据结构 FullDataPacket 和 InferenceResult 定义

• 三个 FreeRTOS 任务函数实现

• 两个 FreeRTOS 队列创建和使用

**✅ 验收标准：**

☑ xTaskCreatePinnedToCore 第 1 个参数为函数指针（非句柄变量）

☑ Task_SensorRead 使用 vTaskDelayUntil 实现 100Hz 精确定时

☑ inferenceQueue 和 dataQueue 深度为 10

☑ 编译通过，无错误

☑ （详细修复 Prompt 见 P9.1）

───────────────────────────────────────────────────────────────────────────

**▎ 3 Phase 2：信号处理**

本节包含原始传感器信号的滤波处理和数据采集标注工具。卡尔曼滤波器用于降低霍尔传感器的噪声（12-bit 分辨率 + 磁场干扰），数据采集工具用于构建训练数据集，是后续所有 AI 模型训练的基础。

**P2.1 1D 卡尔曼滤波器**

**💡 上下文说明：***TMAG5273 霍尔传感器虽然设置了 32 次平均，但磁场信号仍受环境干扰（电机、金属等）。卡尔曼滤波器在已知过程噪声和测量噪声模型的前提下，能提供最优估计。15 路霍尔信号各需要一个独立滤波器实例。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

----------------------------------------------------------------------------------------
  请实现1D卡尔曼滤波器类，文件路径: lib/Filters/KalmanFilter1D.h

  要求：

  1. 类名: KalmanFilter1D

  2. 构造函数: KalmanFilter1D(float process_noise = 0.01, float measurement_noise = 0.1)

  3. 核心方法:

     - float update(float measurement) - 输入测量值,返回滤波后估计值

       - void reset() - 重置滤波器状态

  4. 内部状态: 估计值x, 估计误差P, 卡尔曼增益K

  5. 对15路霍尔信号各实例化一个KalmanFilter1D

  关键参数调优指导:

  - process_noise (Q): 传感器固有噪声水平,起始建议0.01

  - measurement_noise (R): 测量不确定性,起始建议0.1

  - 如果滤波器响应太慢,增大Q或减小R

  - 如果输出抖动太大,减小Q或增大R
----------------------------------------------------------------------------------------

**🎯 预期输出：**

• lib/Filters/KalmanFilter1D.h - 模板类实现（头文件-only）

• 包含完整的状态预测和更新方程

• 构造函数支持自定义 Q 和 R 参数

**✅ 验收标准：**

☑ update() 返回滤波后的估计值

☑ reset() 能将内部状态清零

☑ 首次调用 update() 时正确初始化（不使用零先验）

☑ 15 个独立实例可并行工作，互不干扰

☑ Q=0.01, R=0.1 默认参数下输出平滑

───────────────────────────────────────────────────────────────────────────

**P2.2 数据采集与标注工具**

**💡 上下文说明：***AI 模型训练需要大量标注数据。本工具支持通过 BLE 或 UDP 实时接收传感器数据，录制手势标签，自动保存为 NPY 格式（兼容 Edge Impulse CSV 格式）。每个手势建议录制 100-200 个样本以确保模型泛化能力。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现数据采集Python脚本，文件路径: scripts/data_collector.py

  要求：

  1. 支持BLE串口和UDP两种数据源

  2. 功能:

     - 实时显示传感器数据波形(使用matplotlib或rich)

       - 录制指定手势标签的数据

       - 自动保存为NPY格式: shape=(N, 30, 21) (样本数, 窗口长度, 特征数)

         - 每个手势建议录制100-200个样本

  3. 手势标签管理:

     - 预定义46类手势标签列表

       - 支持自定义标签

       - 按标签自动组织文件目录

  4. 数据增强(可选):

     - 随机时移 ±5帧

       - 高斯噪声 σ=0.01

       - 时间遮蔽(随机零化10%帧)

  5. 使用edge-impulse-data-forwarder兼容的CSV格式作为备选输出
-----------------------------------------------------------------------

**🎯 预期输出：**

• scripts/data_collector.py - 完整的数据采集脚本

• 46 类预定义手势标签

• 实时波形显示界面

• NPY 和 CSV 双格式输出

• 数据增强管道

**✅ 验收标准：**

☑ BLE 和 UDP 两种数据源可切换

☑ 录制的 NPY 文件 shape = (N, 30, 21)

☑ CSV 输出兼容 edge-impulse-data-forwarder

☑ 数据增强功能可开关

☑ 标签管理支持新增/删除/列表

───────────────────────────────────────────────────────────────────────────

**▎ 4 Phase 3：L1 边缘推理**

L1 边缘推理是数据手套的实时手势识别层，运行在 ESP32-S3 上。提供两条技术路线：MVP 路线使用 Edge Impulse（快速原型验证），正式路线使用 PyTorch 训练 1D-CNN+Attention 模型并量化为 TFLite INT8 部署到 MCU。推理延迟要求 \<3ms。

+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| **ℹ️ 双路线策略**                                                                                                                                                                                             |
|                                                                                                                                                                                                               |
| Edge Impulse 路线（P3.1）适合快速验证 feasibility，2-3 天内可得到可用的 L1 模型。PyTorch 路线（P3.2 + P3.3）提供更高的定制化能力和量化精度控制。建议先走 EI 路线验证数据质量，再切换到 PyTorch 路线优化精度。 |
+===============================================================================================================================================================================================================+
+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

**P3.1 Edge Impulse 数据采集与训练（MVP 路线）**

**💡 上下文说明：***Edge Impulse 提供端到端的边缘 AI 开发平台，从数据采集、模型训练到 C++ 库导出一气呵成。对于 MVP 阶段，这是最快的路线：无需手动实现训练管道，直接在浏览器中完成所有工作。输出为 Arduino Library，PlatformIO 可直接引用。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-------------------------------------------------------------------------------------------
  请编写Edge Impulse数据采集和模型训练指南:

  一、数据采集:

  1. 安装edge-impulse-data-forwarder:

     pip install edge-impulse-data-forwarder

  2. 在ESP32固件中实现串口CSV输出:

     - 波特率: 115200

       - 格式: \"hall_1,hall_2,\...,hall_15,euler_r,euler_p,euler_y,gyro_x,gyro_y,gyro_z\\n\"

       - 频率: 100Hz

         - 每行一个时间步

  3. 运行forwarder连接串口,创建项目

  4. 在EI Studio中为每个手势类别录制数据(每类≥100样本)

  二、模型训练(EI Studio):

  1. 创建Impulse:

     - 输入: 21个传感器特征

       - 窗口大小: 3000ms (30帧×100ms)

       - 窗口增量: 100ms

  2. 处理块:

     - Raw Data (直接使用原始数据)

       - 或添加Filter (低通滤波预处理)

  3. 学习块:

     - Classification (Keras)

       - 建议: 1D CNN + Attention

       - 训练周期: 100-200 epochs

         - 学习率: 0.001 (Adam)

  4. 启用EON Tuner优化

  三、部署导出:

  1. 导出为Arduino Library

  2. 解压到PlatformIO项目的lib/目录

  3. 在main.cpp中:

     #include \<edge_impulse_inferencing.h\>

     实现 signal_t callback 和 run_classifier() 调用

**🎯 预期输出：**

• 完整的 Edge Impulse 使用指南文档

• ESP32 端串口 CSV 输出代码片段

• EI Studio 配置参数清单

• C++ 集成代码模板

**✅ 验收标准：**

☑ EI 项目中每类手势 ≥100 样本

☑ Impulse 窗口大小 3000ms / 增量 100ms

☑ 模型训练准确率 \>90%（验证集）

☑ Arduino Library 导出成功并集成到 PlatformIO

☑ ESP32 端推理延迟 \<5ms

───────────────────────────────────────────────────────────────────────────

**P3.2 PyTorch 1D-CNN+Attention 模型训练**

**💡 上下文说明：***当 Edge Impulse 的模型精度不能满足需求时，使用自定义 PyTorch 模型可以获得更高的精度和更灵活的架构控制。1D-CNN 提取局部时序特征，Attention 机制聚焦关键时间步。训练完成后通过 QAT（量化感知训练）转为 INT8 格式以适配 ESP32-S3 的计算资源。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------------------
  请实现PyTorch 1D-CNN+Attention模型训练脚本，文件路径: src/models/l1_train.py

  要求：

  1. 模型架构 L1EdgeModel:

     - 输入: (Batch, 30, 21) - 30帧×21特征

       - Block1: Conv1d(21→32, kernel=5, padding=2) + BatchNorm1d + ReLU + MaxPool(2)

       - Block2: Conv1d(32→64, kernel=3, padding=1) + BatchNorm1d + ReLU + MaxPool(2)

         - Block3: Conv1d(64→128, kernel=3, padding=1) + BatchNorm1d + ReLU

         - TemporalAttention(128):

       \* W_h: Linear(128, 32)

       \* W_e: Linear(128, 32)

       \* v: Linear(32, 1)

       \* score = v(tanh(W_h(h) + W_e(e_mean)))

       \* alpha = softmax(score)

       \* context = sum(alpha \* h)

     - FC: Linear(128, num_classes)

  2. 训练配置:

     - 优化器: AdamW(lr=1e-3, weight_decay=1e-4)

       - 调度器: CosineAnnealingLR(T_max=200)

       - 损失: CrossEntropyLoss(label_smoothing=0.1)

         - Batch Size: 64

         - Epochs: 200 (早停patience=20)

           - 数据增强: 随机时移, 高斯噪声, Mixup(alpha=0.2)

  3. INT8量化导出:

     - 使用torch.quantization.prepare_qat

       - QAT训练3-5个epoch

       - 转换为TFLite INT8

         - 使用repr_dataset校准(100-500样本)

         - 验证量化后精度下降\<2%

  4. 生成model_data.h:

     - xxd -i model_quant.tflite \> model_data.h

       - 验证文件大小\<100KB

  验收标准:

  - 训练集准确率\>98%

  - 验证集准确率\>92%

  - 量化后准确率\>90%

  - 推理延迟\<3ms (ESP32-S3)
-----------------------------------------------------------------------------------

**🎯 预期输出：**

• src/models/l1_train.py - 完整训练脚本

• L1EdgeModel 类定义（含 TemporalAttention）

• 训练循环（含早停、学习率调度、数据增强）

• QAT 量化管道

• TFLite INT8 导出脚本

• model_data.h 生成命令

**✅ 验收标准：**

☑ 训练集准确率 \>98%，验证集准确率 \>92%

☑ INT8 量化后准确率 \>90%（精度下降 \<2%）

☑ model_quant.tflite 文件 \<100KB

☑ model_data.h 正确生成并可被 ESP32 编译

☑ 在 ESP32-S3 上推理延迟 \<3ms

───────────────────────────────────────────────────────────────────────────

**P3.3 TFLite Micro 集成（C++ 端）**

**💡 上下文说明：***将 PyTorch 导出的 TFLite INT8 模型集成到 ESP32-S3 固件中。TFLite Micro 是 Google 为微控制器设计的轻量推理引擎，支持 ESP32-S3 的 AI 向量指令加速。关键挑战是内存管理------Arena 必须分配到 PSRAM 以容纳 80KB 的运行时内存。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-------------------------------------------------------------------------------------
  请实现TFLite Micro推理集成代码，文件路径: src/TFLiteInference.h

  要求：

  1. 类名: TFLiteInference

  2. 初始化:

     - 加载model_data.h中的模型数据

       - 创建AllOpsResolver

       - 创建Interpreter(arena_size=50KB, 使用PSRAM)

  3. 推理方法:

     - int8_t runInference(float \*input_window, int window_size, float \*confidence)

       - 输入: 30×21 = 630个float特征

       - 输出: gesture_id 和 confidence

  4. 性能优化:

     - 使用ESP32-S3的AI向量指令

       - 将推理Arena分配到PSRAM (CONFIG_SPIRAM)

       - 栈大小≥8192字节

  5. 集成到FreeRTOS Task_Inference:

     - 从队列获取数据

       - 填充滑动窗口

       - 调用runInference

         - 结果入队

  关键约束:

  - 使用#include \"tensorflow/lite/micro/all_ops_resolver.h\"

  - 使用#include \"tensorflow/lite/micro/micro_interpreter.h\"

  - Arena大小建议80KB(含模型38KB+运行时)
-------------------------------------------------------------------------------------

**🎯 预期输出：**

• src/TFLiteInference.h - 类声明

• src/TFLiteInference.cpp - TFLite Micro 集成实现

• Arena 分配到 PSRAM 的代码

• 与 FreeRTOS Task_Inference 的集成代码

**✅ 验收标准：**

☑ Arena 成功分配到 PSRAM（非内部 SRAM）

☑ runInference() 输入 630 个 float，输出 gesture_id 和 confidence

☑ 推理延迟 \<3ms（使用 ESP32-S3 AI 指令）

☑ 编译后固件大小增量 \<200KB

☑ 与 Task_Inference 正确集成，队列数据流无阻塞

───────────────────────────────────────────────────────────────────────────

**▎ 5 Phase 4：通信协议**

通信层负责将 ESP32 的传感器数据和推理结果传输到上位机。使用 Protobuf 序列化确保跨平台兼容性，BLE 5.0 提供低功耗移动端连接（配网 + 数据推送），WiFi/UDP 提供高带宽上位机连接（原始数据流 + L2 推理）。

**P4.1 Protobuf 定义与 Nanopb 生成**

**💡 上下文说明：***Protobuf 提供高效的二进制序列化，相比 JSON 减少约 60% 的传输数据量。ESP32 端使用 Nanopb（极简 C 实现），Python 端使用标准 protobuf 库。统一的 .proto 定义确保两端数据格式一致，避免手工解析错误。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

------------------------------------------------------------------------
  请实现Protobuf数据帧定义和Nanopb生成:

  1. 编辑 glove_data.proto:

  syntax = \"proto3\";

  message GloveData {

      uint32 timestamp = 1;

      repeated float hall_features = 2;   // 15路霍尔(mT)

      repeated float imu_features = 3;    // 6路IMU(3欧拉角°+3陀螺仪dps)

      repeated float flex_features = 4;   // 5路柔性(预留)

      uint32 l1_gesture_id = 5;

      uint32 l1_confidence_x100 = 6;     // 置信度\*100(0-10000)

  }

  2. 安装Nanopb:

     pip install protobuf nanopb

     生成C代码: nanopb/generator/protoc \--nanopb_out=. glove_data.proto

  3. 在ESP32端集成:

     #include \"glove_data.pb.h\"

     使用pb_encode序列化

     使用pb_ostream_from_buffer创建输出流

  4. 在Python端:

     使用protoc \--python_out=. glove_data.proto

     pip install protobuf

     glove_data_pb2.GloveData()解析

**🎯 预期输出：**

• glove_data.proto - Protobuf 消息定义

• glove_data.pb.h / glove_data.pb.c - Nanopb 生成的 C 代码

• glove_data_pb2.py - Python protobuf 代码

• ESP32 端序列化示例代码

• Python 端反序列化示例代码

**✅ 验收标准：**

☑ GloveData 消息包含所有 6 个字段

☑ Nanopb 生成代码编译通过（ESP32 端）

☑ Python 端可正确解析 ESP32 发送的 Protobuf 数据

☑ 单帧数据大小 \<200 字节

☑ 序列化/反序列化往返测试通过（round-trip）

───────────────────────────────────────────────────────────────────────────

**P4.2 BLE 5.0 GATT 服务实现**

**💡 上下文说明：***BLE 用于移动端连接和 WiFi 配网。GATT 服务定义了数据交换的接口------Sensor Data Characteristic 用于推送识别结果，Config Characteristic 用于接收 WiFi 凭证。BLE MTU 协商可提升吞吐量至 512 字节/帧，满足 Protobuf 数据传输需求。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现BLE 5.0 GATT服务，文件路径: lib/Comms/BLEManager.h

  要求：

  1. 类名: BLEManager

  2. GATT服务定义:

     - Custom Service UUID: 0x181A

       - Sensor Data Characteristic (Notify): UUID自定义

         - 数据格式: Protobuf序列化的GloveData

           - 最大MTU: 512字节(需MTU协商)

         - Config Characteristic (Read/Write): UUID自定义

             - 用于配网(WiFi SSID/Password下发)

  3. 配网流程:

     - 手机APP通过Config Characteristic写入WiFi凭证

       - 格式: \"SSID:PASSWORD\\n\"

       - ESP32收到后调用WiFi.begin()

         - 连接成功后通过Notify回复\"OK\"

  4. 数据传输:

     - L1识别结果通过Notify发送(每次手势变化时)

       - 传感器原始数据每5帧发送一次(20Hz BLE降级)

  5. 安全:

     - 配网数据仅BLE初始连接时有效

       - 成功配网后清除BLE中的WiFi凭证缓存
-----------------------------------------------------------------------

**🎯 预期输出：**

• lib/Comms/BLEManager.h - 类声明

• lib/Comms/BLEManager.cpp - GATT 服务实现

• BLE 服务器回调注册

• MTU 协商逻辑

• WiFi 配网流程代码

**✅ 验收标准：**

☑ BLE 服务成功广播和连接

☑ MTU 协商成功（≥247 字节，目标 512）

☑ Config Characteristic 写入 WiFi 凭证后成功连接

☑ 配网凭证不在 BLE 回调中持久化

☑ Sensor Data Notify 正常发送 Protobuf 数据

───────────────────────────────────────────────────────────────────────────

**▎ 6 Phase 5：L2 ST-GCN 推理**

L2 推理是上位机端的深度手势识别层，使用时空图卷积网络（ST-GCN）处理完整的手势动作序列。与 L1 的单帧分类不同，L2 能捕捉手指间的空间关系和动作的时序动态，理论上限更高。关键挑战是伪骨骼映射------将 21 维传感器特征映射到 21 个手部关键点的 2D 坐标。

+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| **⚠️ 严重 Bug：原始 ST-GCN 实现为假实现**                                                                                                                                                                      |
|                                                                                                                                                                                                                |
| 原论文 l2_inference.py 中的 STGCNModel 仅使用 nn.Linear(42\*30, 46)------这只是一个单层全连接层，没有任何图卷积结构！它丢失了所有空间（手部骨骼拓扑）和时间（动作序列）信息。P5.2 和 P9.2 提供了完整替换方案。 |
+================================================================================================================================================================================================================+
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

**P5.1 伪骨骼映射与数据预处理**

**💡 上下文说明：***ST-GCN 的输入是骨架序列（关键点坐标 × 时间），但数据手套只能提供 21 维传感器特征（15 霍尔 + 6 IMU）。伪骨骼映射通过学习一个线性变换矩阵，将传感器特征投影到 MediaPipe Hand Landmarks 的 21 个关键点坐标空间，使 ST-GCN 能直接处理。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

--------------------------------------------------------------------------------------------
  请实现伪骨骼映射(Pseudo-skeleton Mapping)和数据预处理管线:

  1. PseudoSkeletonMapper类:

     - 输入: 21维传感器特征(15霍尔+6IMU)

       - 输出: 21个手部关键点的2D坐标(参考MediaPipe Hand Landmarks)

       - 实现方式:

       \* 训练阶段学习映射矩阵W_map (21×21线性变换)

       \* 每个霍尔传感器对应一个MCP关节位置

       \* 欧拉角用于确定手掌朝向和整体姿态

       \* 最终输出shape: (Batch, Time=30, Landmarks=21, Coords=2)

  2. 手部骨骼图定义:

     - 21个关键点(参照MediaPipe):

       0:WRIST, 1:THUMB_CMC, 2:THUMB_MCP, 3:THUMB_IP, 4:THUMB_TIP

       5:INDEX_FINGER_MCP, 6:INDEX_FINGER_PIP, 7:INDEX_FINGER_DIP, 8:INDEX_FINGER_TIP

       9:MIDDLE_FINGER_MCP, 10:MIDDLE_FINGER_PIP, 11:MIDDLE_FINGER_DIP, 12:MIDDLE_FINGER_TIP

       13:RING_FINGER_MCP, 14:RING_FINGER_PIP, 15:RING_FINGER_DIP, 16:RING_FINGER_TIP

       17:PINKY_MCP, 18:PINKY_PIP, 19:PINKY_DIP, 20:PINKY_TIP

     - 邻接边: WRIST→各MCP→各PIP→各DIP→各TIP (骨→骨→骨→尖)

  3. 邻接矩阵A构建:

     - 基于手部解剖结构

       - 包含自环(diagonal=1)

       - 归一化: D\^(-1/2) \* A \* D\^(-1/2)

  4. ST-GCN输入数据格式:

     - (Batch, Time=30, Landmarks=21, Coords=2)

       - 对应视频动作识别中的骨架序列
--------------------------------------------------------------------------------------------

**🎯 预期输出：**

• src/models/pseudo_skeleton.py - PseudoSkeletonMapper 类

• 手部骨骼图定义（21 关键点 + 邻接矩阵）

• 邻接矩阵 A 及其归一化版本 A_norm

• 数据预处理管道：传感器特征 → 伪骨骼坐标

**✅ 验收标准：**

☑ 映射矩阵 W_map 维度正确（21×21）

☑ 输出 shape = (B, 30, 21, 2)

☑ 邻接矩阵包含 20 条边（手部解剖结构正确）

☑ A_norm = D\^(-1/2) @ A @ D\^(-1/2) 计算正确

☑ 伪骨骼坐标的分布与真实手部关键点分布一致

───────────────────────────────────────────────────────────────────────────

**P5.2 ST-GCN 模型实现（从 MS-GCN3 论文）**

**💡 上下文说明：***这是 L2 推理的核心模型。ST-GCN 通过图卷积捕捉手指间的空间关系（如拇指和食指的协同运动），通过时间卷积捕捉动作的时序动态（如握拳→展开的连续变化）。原论文的假实现（单层全连接）必须完全替换为真正的图卷积网络。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------------
  请从MS-GCN3论文出发,实现完整的ST-GCN模型,文件路径: src/models/stgcn.py

  要求：

  ⚠️ 关键: 这不是简单的nn.Linear!必须实现真正的图卷积结构。

  原论文l2_inference.py中的STGCNModel仅使用nn.Linear(42\*30, 46),

  这完全不是ST-GCN,必须在本次实现中完全重写。

  1. 图卷积层(GraphConv):

     - 空间域卷积: 基于邻接矩阵A的特征聚合

       - X\' = A_norm @ X @ W_spatial

       - W_spatial: 可学习的空间权重矩阵

  2. 时间卷积层(TemporalConv):

     - 使用1D卷积(Temporal Convolutional Network)

       - kernel_size=9, padding=4

       - 或使用膨胀卷积(空洞卷积)

  3. ST-Conv Block:

     - 结构: BN → SpatialGraphConv → BN → ReLU → TemporalConv → BN → Residual

       - 残差连接: skip connection + 1x1 conv (维度匹配时)

       - 通道数: \[64, 64, 64, 64, 128, 128, 128, 256, 256\] (9个block)

  4. 注意力池化:

     - 通道注意力: GlobalAvgPool → FC → Sigmoid → Scale

       - 时序注意力: 将时序维度加权聚合为单帧特征

  5. 分类头:

     - Global Average Pooling over time

       - FC(256 → num_classes=46)

  6. 训练配置:

     - 优化器: AdamW(lr=1e-3, weight_decay=1e-4)

       - CosineAnnealingLR(T_max=300)

       - CrossEntropyLoss(label_smoothing=0.1)

         - Batch=32, Epochs=300 (早停patience=30)

  7. 评估:

     - Top-1 Accuracy

       - Top-5 Accuracy

       - 每类F1-Score

         - 混淆矩阵可视化
-----------------------------------------------------------------------------

**🎯 预期输出：**

• src/models/stgcn.py - 完整 ST-GCN 模型

• GraphConv 类（基于邻接矩阵的空间卷积）

• TemporalConv 类（时间域卷积）

• STConvBlock 类（ST 联合卷积 + 残差连接）

• STGCNModel 主模型类

• 训练脚本和评估脚本

**✅ 验收标准：**

☑ 模型包含真正的图卷积层（非 nn.Linear 替代）

☑ 输入维度 (B, 30, 21, 2)，输出维度 (B, 46)

☑ 模型参数量在 2-5M 范围

☑ Top-1 验证准确率 \>85%

☑ 前向传播测试通过（无报错）

☑ （详细替换 Prompt 见 P9.2）

───────────────────────────────────────────────────────────────────────────

**P5.3 L2 推理管线**

**💡 上下文说明：***L2 推理管线将数据接收、预处理、ST-GCN 推理、后处理和 NLP 集成串联起来。它在上位机端持续运行，当 L1 标记为 UNKNOWN 或定期校验时触发 L2 推理。推理结果经过移动平均平滑、置信度过滤和持续时间过滤后，传递给 NLP 模块进行语法纠错和 TTS 播报。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现L2推理管线,文件路径: src/l2_pipeline.py

  要求：

  1. UDP接收线程:

     - 监听0.0.0.0:8888

       - 解析Protobuf GloveData

       - 维护滑动窗口(buffer_size=30)

  2. 推理触发:

     - 当L1标记UNKNOWN(gesture_id=0)时触发L2

       - 或定期(每5秒)进行L2校验

       - 输入: 30帧×21关键点×2D坐标

  3. 后处理:

     - 移动平均平滑(窗口=3)

       - 置信度阈值: 0.80

       - 手势持续时间过滤(\>=300ms)

  4. NLP集成:

     - 手势ID→中文词汇映射

       - 句子缓冲区(收集连续手势)

       - SOV→SVO语法纠错

         - 调用TTS播报

  5. 性能要求:

     - 推理延迟\<20ms(CPU)

       - 端到端延迟\<50ms(GPU)

       - 内存占用\<500MB
-----------------------------------------------------------------------

**🎯 预期输出：**

• src/l2_pipeline.py - 完整 L2 推理管线

• UDP 接收和 Protobuf 解析线程

• 滑动窗口管理器

• 后处理管道（平滑 + 过滤）

• NLP 和 TTS 集成接口

**✅ 验收标准：**

☑ UDP 接收不丢包（局域网环境）

☑ 滑动窗口正确维护 30 帧数据

☑ L1 UNKNOWN 时正确触发 L2

☑ 后处理后手势识别稳定（无抖动）

☑ 端到端延迟（含 NLP）\<100ms

☑ NLP 纠错结果正确输出中文句子

───────────────────────────────────────────────────────────────────────────

**▎ 7 Phase 6-7：NLP 语法纠错与渲染**

中国手语（CSL）的语法结构与标准中文不同------CSL 采用 SOV（主语-宾语-动词）语序，而中文采用 SVO（主语-动词-宾语）。NLP 语法纠错引擎负责将手语语序转换为自然中文语序。渲染层使用 Three.js 在浏览器端实时显示 3D 手部模型，提供直观的交互反馈。

**P6.1 NLP 语法纠错引擎**

**💡 上下文说明：***手语到语音的翻译不仅仅是词汇映射，还需要解决语序差异。CSL 中"我 昨天 苹果 吃"需要被纠正为"我昨天吃苹果"。基础规则库可以处理常见模式，进阶方案使用 Transformer 模型处理更复杂的语境。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

--------------------------------------------------------------------------------
  请实现中国手语(CSL)到中文的语法纠错引擎,文件路径: src/nlp/grammar_corrector.py

  要求：

  1. 基础规则库实现:

     - SOV→SVO转换模板

       - 时间词前移: \"昨天 我 苹果 吃\" → \"昨天 我吃苹果\"

       - 否定句转换: \"我 不 去 学校\" → \"我不去学校\"

         - 疑问句转换: \"你 什么 要\" → \"你要什么\"

  2. 进阶: Transformer纠错模型(可选):

     - 输入: CSL语序的词序列

       - 输出: 中文语序的词序列

       - 模型大小: \~10M参数(轻量)

         - 训练数据: CSL-CSL平行语料库

  3. 词汇映射:

     - 手势ID→中文词汇字典

       - 支持多义词消歧(基于上下文)

  4. 上下文管理:

     - 滑动窗口句子级纠错

       - 对话历史维护

       - 标点符号自动添加
--------------------------------------------------------------------------------

**🎯 预期输出：**

• src/nlp/grammar_corrector.py - 语法纠错引擎

• SOV→SVO 规则库（含 20+ 模板）

• 手势 ID→中文词汇字典

• 上下文管理器

• 标点符号添加逻辑

**✅ 验收标准：**

☑ SOV→SVO 转换正确率 \>95%（规则库覆盖范围内）

☑ 时间词前移规则正确

☑ 否定句和疑问句处理正确

☑ 上下文管理能维持对话历史

☑ 输出句子符合中文自然表达习惯

───────────────────────────────────────────────────────────────────────────

**P7.1 Three.js 浏览器端渲染**

**💡 上下文说明：***3D 手部渲染为用户提供直观的视觉反馈------通过 WebSocket 实时接收手部数据并驱动 3D 手部模型的骨骼动画。使用 Three.js ES Module 和 GLTF 格式模型，无需安装任何软件，浏览器直接打开即可。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

------------------------------------------------------------------------------------------
  请实现基于Three.js + WebSocket的浏览器端3D手部渲染,文件路径: rendering/threejs-hand.html

  要求：

  1. WebSocket连接:

     - 连接到ws://localhost:8080

       - 接收Protobuf格式的GloveData

       - 解析并更新手部模型

  2. 3D场景:

     - 使用Three.js ES Module

       - 手部模型: GLTF/GLB格式,包含骨骼绑定

       - 21个骨骼节点对应21个关键点

         - 灯光: 环境光 + 方向光

  3. 动画更新:

     - 使用requestAnimationFrame

       - 从WebSocket数据直接更新骨骼旋转

       - 四元数平滑(Slerp)

  4. UI:

     - 左侧: 3D手部视图(可旋转/缩放)

       - 右侧: 识别结果面板

       - 底部: 连接状态、延迟、FPS显示

  5. 技术约束:

     - 无需安装任何软件,浏览器直接打开

       - 支持Chrome/Edge/Safari

       - 目标帧率: 60fps
------------------------------------------------------------------------------------------

**🎯 预期输出：**

• rendering/threejs-hand.html - 单文件 HTML 应用

• Three.js 3D 场景（含手部 GLTF 模型）

• WebSocket 连接和 Protobuf 解析

• 骨骼动画更新逻辑

• UI 面板（识别结果 + 状态信息）

**✅ 验收标准：**

☑ 浏览器直接打开 HTML 文件可运行

☑ WebSocket 成功连接并接收数据

☑ 3D 手部模型正确响应传感器数据

☑ 四元数平滑无抖动

☑ FPS ≥30（目标 60）

☑ 支持鼠标旋转和滚轮缩放

───────────────────────────────────────────────────────────────────────────

**▎ 8 Phase 8：集成测试**

集成测试验证从传感器到 TTS 语音播报的完整数据流。使用预录制数据模拟传感器输入，逐层验证 L1 推理、通信传输、L2 推理和 NLP 处理的延迟和精度。端到端延迟目标 \<500ms（含 TTS），\<50ms（不含 TTS）。

**P8.1 端到端集成测试脚本**

**💡 上下文说明：***集成测试是发布前的最终验证环节。使用预录制 NPY 数据模拟真实传感器数据流，自动运行完整的处理管道并生成包含延迟分布图、混淆矩阵和 F1-Score 表格的 HTML 测试报告。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

-----------------------------------------------------------------------
  请实现端到端集成测试脚本,文件路径: tests/integration_test.py

  测试流程:

  1. 传感器数据模拟:

     - 使用预录制的NPY测试数据

       - 模拟100Hz数据流

  2. L1推理测试:

     - 验证模型加载

       - 验证推理延迟\<3ms

       - 验证Top-1准确率

  3. 通信测试:

     - BLE连接稳定性(断开/重连)

       - UDP丢包率(\<1%)

       - Protobuf序列化/反序列化一致性

  4. L2推理测试:

     - 验证ST-GCN加载

       - 验证推理延迟\<20ms

       - 验证Top-5准确率\>99%

  5. 端到端延迟测试:

     - 传感器→L1→BLE/UDP→上位机→L2→NLP→TTS

       - 目标: \<500ms (含TTS)

       - 目标: \<50ms (不含TTS)

  6. 自动化报告生成:

     - 生成HTML测试报告

       - 包含延迟分布图、混淆矩阵、F1-Score表格
-----------------------------------------------------------------------

**🎯 预期输出：**

• tests/integration_test.py - 完整的集成测试脚本

• 传感器数据模拟器

• L1/L2 推理延迟测试

• BLE/UDP 通信测试

• 端到端延迟测试

• HTML 测试报告生成器

**✅ 验收标准：**

☑ L1 推理延迟 \<3ms（P99）

☑ L2 推理延迟 \<20ms（P99）

☑ UDP 丢包率 \<1%（局域网）

☑ 端到端延迟 \<50ms（不含 TTS）

☑ 端到端延迟 \<500ms（含 TTS）

☑ HTML 报告包含所有可视化图表

───────────────────────────────────────────────────────────────────────────

**▎ 9 Bug 修复专用 Prompts**

本节收录了已知的严重 Bug 及其修复 Prompt。这些 Bug 来自原始论文代码的审查结果，每个 Bug 都附带了详细的错误分析、根因说明和修复步骤。在执行开发 Prompt 之前，建议先运行本节的修复 Prompt 以确保基础代码正确。

**P9.1 FreeRTOS xTaskCreatePinnedToCore 参数修复**

**💡 上下文说明：***xTaskCreatePinnedToCore 的参数定义为 (pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask, xCoreID)。原始代码错误地将函数指针参数和句柄变量搞混，导致第 1 个参数传入的是句柄变量的值（一个整数地址）而非函数指针。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

---------------------------------------------------------------------------------------------------------
  请修复以下FreeRTOS任务创建代码中的参数顺序Bug:

  错误代码 (main.cpp L190-192):

  xTaskCreatePinnedToCore(TaskSensorReadHandle, \"SensorRead\", 4096, NULL, 3, &TaskSensorReadHandle, 1);

  xTaskCreatePinnedToCore(TaskInferenceHandle, \"Inference\", 8192, NULL, 2, &TaskInferenceHandle, 0);

  xTaskCreatePinnedToCore(TaskCommsHandle, \"Comms\", 8192, NULL, 1, &TaskCommsHandle, 0);

  Bug分析:

  xTaskCreatePinnedToCore的正确参数顺序为:

  (pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask, xCoreID)

  第1个参数应该是函数指针(Task_SensorRead),而不是句柄变量(TaskSensorReadHandle)

  第6个参数应该是句柄的地址(&TaskSensorReadHandle)

  正确代码:

  xTaskCreatePinnedToCore(Task_SensorRead, \"SensorRead\", 4096, NULL, 3, &TaskSensorReadHandle, 1);

  xTaskCreatePinnedToCore(Task_Inference, \"Inference\", 8192, NULL, 2, &TaskInferenceHandle, 0);

  xTaskCreatePinnedToCore(Task_Comms, \"Comms\", 8192, NULL, 1, &TaskCommsHandle, 0);

  请修复所有三处,并验证编译通过。

**🎯 预期输出：**

• 三处 xTaskCreatePinnedToCore 调用全部修复

• 第 1 个参数从句柄变量改为正确的函数指针

• 编译通过，0 errors

**✅ 验收标准：**

☑ 所有三处参数顺序已修复

☑ 函数指针参数正确（Task_SensorRead, Task_Inference, Task_Comms）

☑ 句柄地址参数正确（&TaskSensorReadHandle, &TaskInferenceHandle, &TaskCommsHandle）

☑ 编译通过

☑ 任务在正确的核心上运行（验证通过日志输出）

───────────────────────────────────────────────────────────────────────────

**P9.2 ST-GCN 假实现替换**

**💡 上下文说明：***原论文 l2_inference.py 中的 STGCNModel 仅包含一个 nn.Linear(42\*30, 46)------这完全不是 ST-GCN。它将 1260 维的输入展平后直接线性映射到 46 个类别，丢失了所有空间拓扑信息和时间动态信息。必须使用 P5.2 中的真实 ST-GCN 架构完全替换。*

**📝 Prompt文本（可直接复制粘贴到 Claude Code）：**

------------------------------------------------------------------------------------
  原论文中的ST-GCN实现(l2_inference.py)存在严重问题:

  问题代码:

  class STGCNModel(nn.Module):

      def \_\_init\_\_(self, num_classes=46):

          super(STGCNModel, self).\_\_init\_\_()

          self.fc = nn.Linear(42 \* 30, num_classes)  # ← 这不是ST-GCN!

      def forward(self, x):

          x = x.view(x.size(0), -1)

          return self.fc(x)

  这只是一个单层全连接层,完全没有任何图卷积结构。它将输入展平后直接线性映射到类别数,

  丢失了所有空间(手部骨骼拓扑)和时间(动作序列)信息。

  要求:

  1. 使用P5.2中定义的真实ST-GCN架构完全替换此假实现

  2. 确保包含:

     - 图卷积层(GraphConv)

       - 时间卷积层(TemporalConv)

       - ST-Conv Block(残差连接)

         - 注意力池化

  3. 验证模型参数量在合理范围(\~2-5M)

  4. 验证输入输出维度匹配: (B,30,21,2) → (B,46)

  5. 运行一个前向传播测试,确保无报错
------------------------------------------------------------------------------------

**🎯 预期输出：**

• l2_inference.py 中的 STGCNModel 被完全替换

• 新模型包含 GraphConv, TemporalConv, STConvBlock

• 模型参数量在 2-5M 范围

• 前向传播测试通过

**✅ 验收标准：**

☑ 旧的 nn.Linear 实现被完全移除

☑ 新模型包含至少 9 个 ST-Conv Block

☑ 通道数序列为 \[64,64,64,64,128,128,128,256,256\]

☑ 输入 (B,30,21,2) → 输出 (B,46) 维度匹配

☑ 模型参数量在 2-5M 范围（打印 model.summary() 验证）

☑ torch.randn(1,30,21,2) 前向传播无报错

☑ 使用新模型重新训练并验证准确率

───────────────────────────────────────────────────────────────────────────

**▎ 附录 A：Prompt 快速索引**

以下表格按编号和开发阶段列出所有 Prompt，方便快速定位。

-------------------------------------------------------------------------------------
  **编号**          **Prompt 标题**                 **所在章节**      **类别**
----------------- ------------------------------- ----------------- -----------------
  **P0.1**          初始化 PlatformIO 项目          第 1 节           环境配置

  **P0.2**          初始化 Python 上位机项目        第 1 节           环境配置

  **P1.1**          TCA9548A I2C 多路复用器驱动     第 2 节           HAL 驱动

  **P1.2**          TMAG5273 3D 霍尔传感器驱动      第 2 节           HAL 驱动

  **P1.3**          BNO085 9 轴 IMU 驱动            第 2 节           HAL 驱动

  **P1.4**          FreeRTOS 双核任务调度           第 2 节           HAL 驱动

  **P2.1**          1D 卡尔曼滤波器                 第 3 节           信号处理

  **P2.2**          数据采集与标注工具              第 3 节           信号处理

  **P3.1**          Edge Impulse 数据采集与训练     第 4 节           L1 推理

  **P3.2**          PyTorch 1D-CNN+Attention 训练   第 4 节           L1 推理

  **P3.3**          TFLite Micro 集成               第 4 节           L1 推理

  **P4.1**          Protobuf 定义与 Nanopb 生成     第 5 节           通信

  **P4.2**          BLE 5.0 GATT 服务实现           第 5 节           通信

  **P5.1**          伪骨骼映射与数据预处理          第 6 节           L2 推理

  **P5.2**          ST-GCN 模型实现                 第 6 节           L2 推理

  **P5.3**          L2 推理管线                     第 6 节           L2 推理

  **P6.1**          NLP 语法纠错引擎                第 7 节           NLP

  **P7.1**          Three.js 浏览器端渲染           第 7 节           渲染

  **P8.1**          端到端集成测试脚本              第 8 节           测试

  **P9.1**          FreeRTOS 参数修复               第 9 节           Bug 修复

  **P9.2**          ST-GCN 假实现替换               第 9 节           Bug 修复

**▎ 附录 B：Prompt 依赖关系**

以下列出了关键 Prompt 之间的依赖关系，建议按依赖顺序执行。

P0.1 ──→ P1.1 ──→ P1.2 ──→ P1.4 ──→ P3.1 / P3.3

P0.2 ──→ P2.2 ──→ P3.2 ──→ P3.3

P4.1 ──→ P4.2

P5.1 ──→ P5.2 ──→ P5.3

P5.3 ──→ P6.1

P8.1 依赖所有前置 Prompt 完成

P9.1 应在 P1.4 之前执行（修复基础代码）

P9.2 应在 P5.2 之前执行（替换假实现）

───────────────────────────────────────────────────────────────────────────

**--- 文档结束 ---**

Claude Code Prompt 工程手册 v1.0 \| Edge AI 数据手套项目 \| 2026-04-16
