# Smart Glove - ASL Translation Project

A smart glove that translates American Sign Language (ASL) to text and speech using machine learning and deep learning models. The system uses ESP32 microcontrollers with flex sensors and IMU (MPU6050) for gesture recognition.

## Hardware

- **Microcontroller**: ESP32
- **Sensors**: 5x Flex Sensors + MPU6050 (Gyroscope + Accelerometer)
- **Deployment**: Raspberry Pi 4 or ESP32 directly

## Features

- Real-time ASL gesture recognition
- Text and speech output (TTS)
- Multiple ML algorithms support
- ESP32 edge deployment capability
- TensorFlow Lite conversion for embedded systems

## Project Structure

```
Smart-glove-main/
├── RF.py                          # Random Forest training script
├── success.ino                   # Main ESP32 firmware (WiFi)
├── 5flexsensors.ino              # Flex sensor test
├── model deployment.py           # Raspberry Pi deployment
├── requirements.txt              # Python dependencies
│
├── Random Forest on esp32/        # ESP32 firmware
│   ├── rf.ino                    # On-device ML inference
│   ├── Datasetcollection.ino     # Data collection
│   └── classifier.h             # RF model header
│
├── smartglovepcb/                # KiCad PCB design
├── modified dataset/             # Training data (CSV)
│
├── ML_Notebooks_Guide.md         # ML notebooks guide
├── TRAINING_GUIDE.md            # Deep learning training guide
└── INSTALLATION_GUIDE.md        # Installation instructions
```

## ML Notebooks

| Notebook | Description | Best For |
|----------|-------------|----------|
| RF.ipynb | Random Forest baseline | Quick start |
| Traditional_ML_Comparison.ipynb | SVM, KNN, XGBoost, LightGBM | Algorithm selection |
| Ensemble_Learning.ipynb | Voting, Stacking, Blending | Performance boost |
| Hyperparameter_Optimization.ipynb | Grid, Random, Bayesian, Optuna | Model tuning |
| Conv1D_BiLSTM_Training.ipynb | Deep learning time series | Best accuracy |
| model_deployment.ipynb | Raspberry Pi deployment | Production |

## Installation

### Python Dependencies

```bash
# Install all dependencies
pip install -r requirements.txt

# Or install by need:
# Basic ML
pip install numpy pandas scikit-learn joblib matplotlib seaborn

# Deep Learning (TensorFlow - NOT PyTorch)
pip install tensorflow scipy

# Advanced ML
pip install xgboost lightgbm optuna scikit-optimize deap
```

**Note**: This project uses TensorFlow, NOT PyTorch.

### Hardware Setup

1. Connect 5 flex sensors to ESP32 analog pins (36, 39, 34, 35, 32)
2. Connect MPU6050 via I2C (SDA=21, SCL=22)
3. Flash firmware using Arduino IDE or PlatformIO

## Usage

### Train Model

```bash
# Run Random Forest training
python RF.py

# Or use Jupyter notebooks
jupyter notebook
# Open and run any notebook
```

### Deploy to ESP32

1. Train model → Export via Micromlgen → Flash to ESP32
2. Or use TensorFlow Lite → ESP32-S3 deployment

### Real-time Inference

```bash
# Raspberry Pi (receives ESP32 data via WiFi)
python "model deployment.py"
```

## Documentation

- [ML Notebooks Guide](ML_Notebooks_Guide.md) - Complete notebook overview
- [Training Guide](TRAINING_GUIDE.md) - Deep learning training details
- [Installation Guide](INSTALLATION_GUIDE.md) - Setup instructions

## Dataset

Original dataset available at: [Figshare](https://figshare.com/articles/dataset/ASL-Sensor-Dataglove-Dataset_zip/20031017?file=35776586)

## License

MIT License

## Support

For issues or suggestions, please submit an Issue.

---

If you like this project, consider supporting us: [Donate](https://ko-fi.com/heyitsmeyo)
