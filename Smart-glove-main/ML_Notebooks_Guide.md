# Smart Glove ASL Recognition - Jupyter Notebooks Guide

## Project Overview

This directory contains complete machine learning solutions for ASL gesture recognition, ranging from traditional ML to deep learning, from feature engineering to hyperparameter optimization.

---

## Notebook List

### 1. RF.ipynb
- **Description**: Original Random Forest training code
- **Content**: Data loading, Random Forest classifier training, evaluation
- **Algorithm**: Random Forest (2000 trees)
- **Use Case**: Quick baseline establishment

### 2. Conv1D_BiLSTM_Training.ipynb
- **Description**: Deep learning time series modeling
- **Content**: Conv1D + Bi-LSTM complete training pipeline
- **Architecture**:
  - Conv1D (3 layers: 32→64→128 filters)
  - Bi-LSTM (128→64 units)
  - Dense classification layer
- **Features**:
  - Kalman filter preprocessing
  - TensorFlow Lite conversion
  - ESP32 deployment code generation
- **Use Case**: Production-grade time series recognition

### 3. Traditional_ML_Comparison.ipynb
- **Description**: Traditional ML algorithm comparison
- **Content**: 5 classic algorithms comparison
- **Algorithms**:
  - **SVM**: C=10, RBF kernel
  - **KNN**: K=5, distance-weighted
  - **XGBoost**: 200 trees, max_depth=6
  - **LightGBM**: Histogram algorithm, Leaf-wise growth
  - **AdaBoost**: Decision tree base learners
- **Use Case**: Fast algorithm selection, resource-constrained environments

### 4. Ensemble_Learning.ipynb
- **Description**: Ensemble learning methods
- **Methods**:
  - **Voting**: Hard/Soft/Weighted voting
  - **Stacking**: Manual + sklearn implementation
  - **Blending**: Holdout validation
  - **Weight Optimization**: scipy.optimize
- **Use Case**: Maximize model performance

### 5. Hyperparameter_Optimization.ipynb
- **Description**: Hyperparameter optimization methods
- **Methods**:
  - **Grid Search**: Exhaustive search
  - **Random Search**: Random sampling
  - **Bayesian Optimization**: Gaussian process surrogate
  - **Optuna**: TPE sampler with pruning
  - **PSO**: Particle Swarm Optimization
- **Use Case**: Model tuning, performance maximization

### 6. model_deployment.ipynb
- **Description**: Model deployment code
- **Content**: Raspberry Pi/PC receives ESP32 data and runs inference
- **Communication**: TCP Socket (port 4444)
- **Output**: Text + TTS voice

---

## Recommended Workflow

### Phase 1: Quick Prototype (1-2 days)
1. Run `RF.ipynb` to establish baseline
2. Run `Traditional_ML_Comparison.ipynb` to select best traditional algorithm

### Phase 2: Performance Improvement (3-5 days)
3. Run `Ensemble_Learning.ipynb` to try ensemble methods
4. Run `Hyperparameter_Optimization.ipynb` to optimize hyperparameters

### Phase 3: Deep Learning (5-7 days)
5. Run `Conv1D_BiLSTM_Training.ipynb` for time series model

### Phase 4: Deployment (2-3 days)
6. Use `model_deployment.ipynb` to deploy to target device

---

## Algorithm Comparison

| Method | Accuracy | Speed | Complexity | Use Case |
|--------|----------|-------|------------|----------|
| Random Forest | ~81% | Fast | Low | Baseline |
| SVM | ~85% | Medium | Medium | High-dimensional data |
| XGBoost | ~87% | Fast | Medium | Production |
| LightGBM | ~87% | Very Fast | Medium | Large-scale data |
| Stacking | ~89% | Slow | High | Competition/Research |
| Conv1D+BiLSTM | ~92% | Medium | High | Time Series |

---

## Deep Learning Details (Conv1D_BiLSTM)

### Model Architecture
```
Input: (100, 11) - 100 frames time series, 11 features

Conv1D Feature Extraction:
├── Conv1D(32, kernel=3) + BN + MaxPool + Dropout(0.3)
├── Conv1D(64, kernel=3) + BN + MaxPool + Dropout(0.3)
└── Conv1D(128, kernel=3) + BN

Bi-LSTM Temporal Modeling:
├── Bi-LSTM(128, return_sequences=True) + Dropout(0.3)
└── Bi-LSTM(64, return_sequences=False) + Dropout(0.3)

Classification:
├── Dense(128) + ReLU + Dropout(0.3)
└── Dense(num_classes) + Softmax
```

### TensorFlow Lite Quantization

| Type | Size | Accuracy | Use Case |
|------|------|----------|----------|
| Float32 | ~500KB | Highest | Performance testing |
| Dynamic Range | ~150KB | High | General deployment |
| INT8 (Recommended) | ~80KB | Good | ESP32 edge deployment |

### ESP32-S3 Deployment
- **Memory**: 50KB Tensor Arena
- **Latency**: <50ms
- **Model Size**: ~80KB (INT8 quantized)

---

## Environment Setup

### Python Version
- **Recommended**: Python 3.10
- **Supported**: 3.9, 3.10, 3.11

### Install Dependencies

```bash
# All dependencies
pip install -r requirements.txt

# By usage scenario:

# Basic ML (RF, SVM, KNN):
pip install numpy pandas scikit-learn joblib matplotlib seaborn

# + Deep Learning (TensorFlow):
pip install tensorflow scipy

# + Advanced ML (XGBoost, LightGBM):
pip install xgboost lightgbm

# + Hyperparameter Optimization:
pip install optuna scikit-optimize deap
```

### Important Notes
- **This project uses TensorFlow, NOT PyTorch**
- No PyTorch dependencies required
- TensorFlow 2.13.0+ recommended for Python 3.9+

---

## Learning Path

### Beginner
1. RF.ipynb → Understand basic ML workflow
2. Traditional_ML_Comparison.ipynb → Learn different algorithms

### Intermediate
3. Ensemble_Learning.ipynb → Learn ensemble methods
4. Hyperparameter_Optimization.ipynb → Master tuning techniques

### Advanced
5. Conv1D_BiLSTM_Training.ipynb → Deep learning application

---

## File Structure

```
Smart-glove-main/
├── RF.py                          # Original Random Forest training
├── model_deployment.ipynb         # Deployment code
├── requirements.txt               # All dependencies
├── Conv1D_BiLSTM_Training.ipynb  # Deep Learning
├── Traditional_ML_Comparison.ipynb# Traditional ML comparison
├── Ensemble_Learning.ipynb        # Ensemble methods
├── Hyperparameter_Optimization.ipynb # Hyperparameter tuning
├── ML_Notebooks_Guide.md         # This file
└── modified dataset/             # Training data
```

---

## Tips

1. **Memory Limit**: Use subsampling if memory insufficient
2. **Time Limit**: Reduce iterations during hyperparameter search
3. **GPU Acceleration**: Deep learning notebook auto-detects GPU
4. **Chinese Support**: All notebooks configured for Chinese display

---

## Support

For issues or suggestions, please submit an Issue.

**Last Updated**: 2026-03-01
**Version**: v2.0
