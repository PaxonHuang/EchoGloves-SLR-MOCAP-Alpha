# Conv1D + Bi-LSTM ASL手势识别训练指南

## 项目概述

本项目提供完整的深度学习方案，使用 **Conv1D + Bi-LSTM** 架构进行时间序列手势识别，支持从数据预处理到ESP32-S3边缘部署的全流程。

## 生成的文件

### 1. 主要Jupyter Notebook
- **文件**: `Conv1D_BiLSTM_Training.ipynb`
- **功能**: 完整的模型训练、评估、转换流程
- **包含**: 
  - 数据加载与预处理
  - Conv1D + Bi-LSTM模型设计
  - 训练与评估
  - TensorFlow Lite转换
  - ESP32部署代码生成

## 快速开始

### 步骤1: 安装依赖
```bash
pip install tensorflow numpy pandas matplotlib seaborn scikit-learn scipy
```

### 步骤2: 准备数据
确保数据文件位于 `modified dataset/alphabet/` 目录下，格式为 `{gesture}_merged.csv_exported.csv`

### 步骤3: 运行Notebook
```bash
jupyter notebook Conv1D_BiLSTM_Training.ipynb
```

按顺序执行所有cell即可完成：
1. 数据加载与探索
2. 数据预处理（时间序列生成）
3. 模型设计与训练
4. 模型评估与可视化
5. TensorFlow Lite转换
6. ESP32部署代码生成

## 模型架构

### Conv1D + Bi-LSTM 网络结构

```
输入: (100, 11) - 100帧时间序列，11维特征

Conv1D特征提取层:
├── Conv1D(32, kernel=3) + BN + MaxPool + Dropout(0.3)
├── Conv1D(64, kernel=3) + BN + MaxPool + Dropout(0.3)
└── Conv1D(128, kernel=3) + BN

Bi-LSTM时序建模层:
├── Bi-LSTM(128, return_sequences=True) + Dropout(0.3)
└── Bi-LSTM(64, return_sequences=False) + Dropout(0.3)

分类层:
├── Dense(128) + ReLU + Dropout(0.3)
└── Dense(num_classes) + Softmax
```

### 数学公式

#### 1. Conv1D 卷积操作
```
y_t = Σ(w_k · x_{t+k}) + b
```
- 提取局部时序特征
- 具有平移不变性

#### 2. LSTM 单元
```
f_t = σ(W_f · [h_{t-1}, x_t] + b_f)  # 遗忘门
i_t = σ(W_i · [h_{t-1}, x_t] + b_i)  # 输入门
C̃_t = tanh(W_C · [h_{t-1}, x_t] + b_C)  # 候选状态
C_t = f_t ⊙ C_{t-1} + i_t ⊙ C̃_t  # 细胞状态
o_t = σ(W_o · [h_{t-1}, x_t] + b_o)  # 输出门
h_t = o_t ⊙ tanh(C_t)  # 隐藏状态
```

#### 3. Bi-LSTM 拼接
```
h_t = [→h_t ; ←h_t]
```
- 前向LSTM: 学习过去上下文
- 后向LSTM: 学习未来上下文

#### 4. Softmax 分类
```
P(y=i|x) = exp(z_i) / Σ(exp(z_j))
```

## 数据预处理流程

### 1. 特征选择 (11维)
- **弯曲传感器** (5维): flex_1 ~ flex_5
- **加速度计** (3维): ACCx, ACCy, ACCz
- **陀螺仪** (3维): GYRx, GYRy, GYRz

### 2. 归一化
```python
# 弯曲传感器
flex_normalized = flex / max_value  # → [0, 1]

# 加速度计
accel_normalized = accel / 9.81  # → g单位

# 陀螺仪
gyro_normalized = gyro / 2000.0  # → 归一化到[-1, 1]
```

### 3. 时间序列生成
使用滑动窗口创建训练样本：
```python
sequence_length = 100  # 2秒 @ 50Hz
stride = 10  # 步长
```

### 4. 卡尔曼滤波 (可选)
```python
# 状态方程: x_k = x_{k-1} + w_k
# 观测方程: z_k = x_k + v_k
# 卡尔曼增益: K = P_pred / (P_pred + R)
```

## 训练配置

### 超参数
```python
batch_size = 32
epochs = 100
learning_rate = 0.001
optimizer = Adam
loss = sparse_categorical_crossentropy
```

### 回调函数
- **EarlyStopping**: patience=10, 监控val_accuracy
- **ReduceLROnPlateau**: patience=5, factor=0.5
- **ModelCheckpoint**: 保存最佳模型

## TensorFlow Lite转换

### 三种量化方案

#### 1. Float32 (基准)
- 大小: ~500KB
- 精度: 最高
- 适用: 性能测试

#### 2. 动态范围量化
- 大小: ~150KB
- 精度: 较高
- 适用: 一般部署

#### 3. **INT8全量化 (推荐)**
- 大小: ~80KB
- 精度: 良好
- 适用: ESP32边缘部署
- 速度: 最快（使用整数运算）

### 转换代码
```python
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8
tflite_model = converter.convert()
```

## ESP32-S3部署

### 硬件要求
- **开发板**: ESP32-S3-DevKitC-1
- **内存**: 8MB PSRAM (推荐)
- **Flash**: 16MB
- **传感器**: MPU6050 + 5x弯曲传感器

### 软件配置

#### platformio.ini
```ini
[env:esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.arduino.memory_type = qio_qspi
board_build.partitions = default_16MB.csv
build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
lib_deps = 
    adafruit/Adafruit MPU6050
    https://github.com/tanakamasayuki/TensorFlowLite_ESP32.git
```

### 内存分配
```cpp
// Tensor Arena大小
constexpr int kTensorArenaSize = 50 * 1024;  // 50KB
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

// 滑动窗口缓冲区
float sensor_buffer[100][11];  // 100帧 x 11特征
```

### 推理流程
```cpp
void loop() {
    // 50Hz采样
    if (millis() - last_time >= 20) {
        // 读取传感器
        readSensors(features);
        
        // 添加到滑动窗口
        addToBuffer(features);
        
        // 缓冲区满后推理
        if (buffer_full) {
            int gesture = predictGesture();
            float confidence = getConfidence();
            
            if (confidence > 0.7) {
                Serial.printf("Gesture: %d (%.1f%%)\n", 
                            gesture, confidence * 100);
            }
        }
    }
}
```

## 性能优化建议

### 1. 模型优化
- 使用深度可分离卷积减少参数量
- LSTM单元控制在64-128之间
- 避免复杂激活函数(swish等)

### 2. 内存优化
- Tensor Arena: 50KB (可根据模型调整)
- 模型大小: <200KB (INT8量化后)
- 避免动态内存分配

### 3. 推理优化
- 使用DMA传输传感器数据
- 双缓冲避免数据丢失
- 批处理推理(如果可能)

## 预期性能

### 准确率
- **训练集**: >95%
- **验证集**: >90%
- **测试集**: >88%
- **Top-3准确率**: >95%

### 推理性能 (ESP32-S3)
- **延迟**: <50ms
- **功耗**: <100mW (推理时)
- **帧率**: 50Hz采样，~20Hz推理

### 模型大小
- **Keras**: ~2MB
- **TFLite Float32**: ~500KB
- **TFLite INT8**: ~80KB
- **C++ Header**: ~200KB (hex编码)

## 故障排除

### 训练问题

**Q: 模型过拟合**
A: 
- 增加Dropout比率(0.3→0.5)
- 增加训练数据
- 使用数据增强
- 减少模型容量

**Q: 训练不收敛**
A:
- 降低学习率(0.001→0.0001)
- 检查数据归一化
- 增加批次大小
- 检查标签编码

**Q: 验证准确率波动大**
A:
- 使用更大的验证集
- 增加EarlyStopping patience
- 使用学习率衰减

### 部署问题

**Q: ESP32内存不足**
A:
- 减小Tensor Arena
- 使用更小的模型
- 启用PSRAM
- 优化缓冲区大小

**Q: 推理结果不准确**
A:
- 检查数据预处理是否与训练一致
- 验证传感器校准
- 增加置信度阈值
- 检查量化误差

**Q: 推理速度慢**
A:
- 使用INT8量化
- 减少模型复杂度
- 优化Tensor Arena大小
- 检查是否有不必要的运算

## 扩展建议

### 1. 数据增强
- 时间扭曲(Time Warp)
- 幅度缩放
- 添加高斯噪声
- 随机裁剪

### 2. 模型改进
- 尝试TCN (Temporal Convolutional Network)
- 添加注意力机制
- 使用残差连接
- 尝试Transformer架构

### 3. 功能扩展
- 添加置信度阈值自适应
- 实现连续手势识别
- 添加手势过渡检测
- 支持自定义手势学习

## 参考资料

### 论文
- "Deep Learning for Time Series Classification" by Fawaz et al.
- "Long Short-Term Memory" by Hochreiter & Schmidhuber
- "Convolutional Neural Networks for Human Activity Recognition"

### 文档
- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
- [ESP32 Technical Reference](https://docs.espressif.com/)
- [Edge Impulse Documentation](https://docs.edgeimpulse.com/)

## 许可证

本项目基于MIT许可证开源。

## 联系方式

如有问题或建议，请提交Issue或Pull Request。

---

**最后更新**: 2026-02-15  
**版本**: v1.0  
**作者**: Smart Glove Team
