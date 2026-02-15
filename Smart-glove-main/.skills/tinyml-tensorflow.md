# TinyML & TensorFlow Lite Skill
# 适用于ESP32边缘AI模型部署

## 概述
本Skill专注于为智能手套项目提供TinyML和TensorFlow Lite Micro的完整解决方案，支持从模型设计到ESP32部署的全流程。

## 核心能力

### 1. 模型架构设计
- **Conv1D + Bi-LSTM**: 适合时间序列手势识别
- **TCN (Temporal Convolutional Network)**: 备选方案
- **轻量化CNN**: 适用于静态手势

### 2. TensorFlow Lite转换
```python
# 标准转换流程
def convert_to_tflite(model, representative_dataset=None):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # 动态范围量化
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    # 全整数量化(推荐用于ESP32)
    if representative_dataset:
        converter.representative_dataset = representative_dataset
        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8
    
    return converter.convert()
```

### 3. ESP32-S3优化策略
- **内存管理**: 使用Tensor Arena (30-50KB)
- **算子选择**: 仅注册必要算子
- **量化**: INT8量化减少内存占用
- **模型大小**: 目标<300KB

## 智能手套专用模板

### Conv1D + Bi-LSTM 架构
```python
import tensorflow as tf

def create_gesture_model(num_classes=22, sequence_length=100, num_features=11):
    """
    为ASL手势识别优化的Conv1D + Bi-LSTM模型
    
    Args:
        num_classes: 手势类别数(当前22种)
        sequence_length: 时间序列长度(2秒@50Hz)
        num_features: 特征维度(5弯曲+3陀螺仪+3加速度计)
    """
    model = tf.keras.Sequential([
        # Conv1D层 - 提取局部时序特征
        tf.keras.layers.Conv1D(32, kernel_size=3, activation='relu',
                              input_shape=(sequence_length, num_features)),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Dropout(0.2),
        
        tf.keras.layers.Conv1D(64, kernel_size=3, activation='relu'),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Dropout(0.2),
        
        tf.keras.layers.Conv1D(128, kernel_size=3, activation='relu'),
        tf.keras.layers.BatchNormalization(),
        
        # Bi-LSTM层 - 捕捉长期依赖
        tf.keras.layers.Bidirectional(
            tf.keras.layers.LSTM(128, return_sequences=True)
        ),
        tf.keras.layers.Dropout(0.3),
        
        tf.keras.layers.Bidirectional(
            tf.keras.layers.LSTM(64, return_sequences=False)
        ),
        tf.keras.layers.Dropout(0.3),
        
        # 全连接层
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])
    
    return model
```

### 数据预处理管道
```python
import numpy as np
from scipy import signal

class GestureDataPreprocessor:
    """手势数据预处理器"""
    
    def __init__(self, sequence_length=100, sampling_rate=50):
        self.sequence_length = sequence_length
        self.sampling_rate = sampling_rate
        
    def normalize_flex(self, flex_data, min_val=0, max_val=4095):
        """弯曲传感器归一化到[0,1]"""
        return (flex_data - min_val) / (max_val - min_val)
    
    def normalize_imu(self, accel, gyro):
        """IMU数据归一化"""
        # 加速度归一化到g
        accel_norm = accel / 9.81
        # 陀螺仪归一化(假设最大2000deg/s)
        gyro_norm = gyro / 2000.0
        return accel_norm, gyro_norm
    
    def apply_kalman_filter(self, data, process_noise=0.01, measurement_noise=0.1):
        """应用卡尔曼滤波"""
        # 简化的卡尔曼滤波实现
        n_iter = len(data)
        xhat = np.zeros(n_iter)
        P = np.zeros(n_iter)
        xhatminus = np.zeros(n_iter)
        Pminus = np.zeros(n_iter)
        K = np.zeros(n_iter)
        
        Q = process_noise  # 过程噪声
        R = measurement_noise  # 测量噪声
        
        xhat[0] = data[0]
        P[0] = 1.0
        
        for k in range(1, n_iter):
            # 预测
            xhatminus[k] = xhat[k-1]
            Pminus[k] = P[k-1] + Q
            
            # 更新
            K[k] = Pminus[k] / (Pminus[k] + R)
            xhat[k] = xhatminus[k] + K[k] * (data[k] - xhatminus[k])
            P[k] = (1 - K[k]) * Pminus[k]
        
        return xhat
    
    def create_sequences(self, data, labels, stride=10):
        """创建时间序列样本"""
        sequences = []
        sequence_labels = []
        
        for i in range(0, len(data) - self.sequence_length, stride):
            seq = data[i:i + self.sequence_length]
            label = labels[i + self.sequence_length - 1]
            sequences.append(seq)
            sequence_labels.append(label)
        
        return np.array(sequences), np.array(sequence_labels)
```

### TFLite Micro ESP32代码模板
```cpp
// gesture_model.h - 生成的模型头文件
#pragma once
#include <cstdint>

const unsigned char gesture_model_tflite[] = {
    // 模型数据将通过转换脚本生成
};
const int gesture_model_tflite_len = 0; // 实际长度

// main.cpp - ESP32推理代码
#include <TensorFlowLite_ESP32.h>
#include "gesture_model.h"

namespace {
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    
    // Tensor Arena - 根据模型调整大小
    constexpr int kTensorArenaSize = 50 * 1024;  // 50KB
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];
}

void setup() {
    Serial.begin(115200);
    
    // 加载模型
    model = tflite::GetModel(gesture_model_tflite);
    
    // 配置算子解析器
    static tflite::MicroMutableOpResolver<12> resolver;
    resolver.AddConv2D();
    resolver.AddMaxPool2D();
    resolver.AddLstm();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddReshape();
    
    // 创建解释器
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    
    // 分配张量
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        Serial.println("AllocateTensors() failed");
        return;
    }
    
    input = interpreter->input(0);
    output = interpreter->output(0);
}

int predict_gesture(float* sensor_data, int sequence_length, int num_features) {
    // 复制数据到输入张量
    memcpy(input->data.f, sensor_data, 
           sequence_length * num_features * sizeof(float));
    
    // 运行推理
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        Serial.println("Invoke failed");
        return -1;
    }
    
    // 获取预测结果
    float* predictions = output->data.f;
    int num_classes = 22;  // 根据实际类别数调整
    int predicted_class = 0;
    float max_prob = predictions[0];
    
    for (int i = 1; i < num_classes; i++) {
        if (predictions[i] > max_prob) {
            max_prob = predictions[i];
            predicted_class = i;
        }
    }
    
    return predicted_class;
}
```

## 模型转换脚本
```python
# convert_to_esp32.py
import tensorflow as tf
import numpy as np
import os

def representative_dataset_gen():
    """生成代表性数据集用于量化"""
    for _ in range(100):
        # 生成随机数据模拟实际传感器输入
        data = np.random.randn(1, 100, 11).astype(np.float32)
        yield [data]

def convert_and_export(model_path, output_dir='./esp32_model'):
    """转换并导出模型"""
    os.makedirs(output_dir, exist_ok=True)
    
    # 加载模型
    model = tf.keras.models.load_model(model_path)
    
    # 1. 转换为标准TFLite
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()
    
    with open(f'{output_dir}/model.tflite', 'wb') as f:
        f.write(tflite_model)
    print(f"标准TFLite模型: {len(tflite_model)/1024:.1f}KB")
    
    # 2. 转换为INT8量化TFLite
    converter_int8 = tf.lite.TFLiteConverter.from_keras_model(model)
    converter_int8.optimizations = [tf.lite.Optimize.DEFAULT]
    converter_int8.representative_dataset = representative_dataset_gen
    converter_int8.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter_int8.inference_input_type = tf.int8
    converter_int8.inference_output_type = tf.int8
    
    tflite_model_int8 = converter_int8.convert()
    
    with open(f'{output_dir}/model_int8.tflite', 'wb') as f:
        f.write(tflite_model_int8)
    print(f"INT8量化模型: {len(tflite_model_int8)/1024:.1f}KB")
    
    # 3. 生成C++头文件
    with open(f'{output_dir}/gesture_model.h', 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <cstdint>\n\n')
        f.write('const unsigned char gesture_model_tflite[] = {\n')
        
        hex_array = [f'0x{b:02x}' for b in tflite_model_int8]
        for i in range(0, len(hex_array), 12):
            line = ', '.join(hex_array[i:i+12])
            f.write(f'    {line},\n')
        
        f.write('};\n')
        f.write(f'const int gesture_model_tflite_len = {len(tflite_model_int8)};\n')
    
    print(f"C++头文件已生成: {output_dir}/gesture_model.h")

if __name__ == '__main__':
    convert_and_export('path/to/your/model.h5')
```

## 性能优化建议

### 1. 模型优化
- 使用深度可分离卷积减少参数量
- LSTM单元数控制在64-128之间
- 避免使用复杂激活函数(如swish)

### 2. 内存优化
- Tensor Arena大小: 50KB (ESP32-S3可支持更大)
- 模型大小目标: <200KB (INT8量化后)
- 避免动态内存分配

### 3. 推理优化
- 批处理推理(Batch Inference)
- 使用DMA传输传感器数据
- 双缓冲机制避免数据丢失

## 常见问题

### Q: 模型在ESP32上运行过慢?
A: 检查是否启用了XTAensa DSP加速，使用INT8量化，减少LSTM单元数

### Q: 推理结果不准确?
A: 确保输入数据归一化与训练时一致，检查数据预处理管道

### Q: 内存不足?
A: 减小Tensor Arena，使用更小的模型，或升级到ESP32-S3 (8MB PSRAM)
