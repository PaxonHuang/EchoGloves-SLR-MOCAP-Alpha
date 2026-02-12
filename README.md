# AGENTS.md - Smart Glove Project Guidelines

This file provides guidelines for AI agents working on the Smart Glove ASL translation project.

## Project Overview

Smart glove that translates American Sign Language (ASL) to text/speech using:

- **Hardware**: ESP32, 5 flex sensors, MPU6050 (gyroscope + accelerometer)
- **ML Model**: Random Forest classifier
- **Deployment**: Raspberry Pi 4 or ESP32 directly (via Micromlgen)
- **PCB Design**: KiCad files in `smartglovepcb/`

## Repository Structure

```
Smart-glove-main/
├── RF.py                      # Python ML training script
├── success.ino                # Main ESP32 firmware (WiFi + sensor data)
├── Random Forest on esp32/
│   ├── rf.ino                 # Random Forest inference on ESP32
│   ├── Datasetcollection.ino  # Data collection firmware
│   └── classifier.h           # Generated Random Forest model
├── smartglovepcb/             # KiCad PCB design files
└── modified dataset/          # Training data (CSV files)
```

## Build/Test Commands

### Python ML Code

```bash
# No formal test suite - run training script directly
python RF.py

# Verify data files exist before training
python -c "import os; print([f for f in os.listdir('Smart-glove-main/modified dataset/alphabet/') if f.endswith('.csv')])"
```

### Arduino/ESP32 Code

```bash
# No automated tests - compile with Arduino IDE or PlatformIO
# Required libraries:
# - FlexLibrary
# - Adafruit_MPU6050
# - Adafruit_Sensor
# - WiFi (built-in for ESP32)
```

### PCB Design

```bash
# Open in KiCad 7.0+
# Files: smartglove.kicad_pro, smartglove.kicad_sch, smartglove.kicad_pcb
```

## Code Style Guidelines

### Python (ML Scripts)

**Imports**: Standard library first, then third-party

```python
import os
import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
```

**Naming Conventions**:

- Variables: `snake_case` (e.g., `combined_df`, `y_encoded`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `columns_to_filter`)
- Classes: `PascalCase` (follow sklearn conventions)

**Code Format**:

- UTF-8 encoding with `# -*- coding: utf-8 -*-` header
- 4 spaces indentation
- Comments in English, print statements may use Chinese for user feedback
- Maximum line length: ~100 characters

**ML-Specific Patterns**:

- Use `train_test_split` with `random_state` for reproducibility
- Always scale features with `StandardScaler`
- Use `LabelEncoder` for categorical targets
- Print accuracy and OOB score for model evaluation

### Arduino/C++ (Embedded Code)

**Includes**: Order by dependency (custom first, then libraries)

```cpp
#include "FlexLibrary.h"
#include <Adafruit_MPU6050.h>
#include <Wire.h>
```

**Naming Conventions**:

- Constants: `UPPER_SNAKE_CASE` (e.g., `VCC`, `R_DIV`, `MPU6050_RANGE_8_G`)
- Variables: `camelCase` (e.g., `angles`, `sensorValue`)
- Functions: `camelCase` (e.g., `updateVal()`, `getSensorValue()`)
- Classes: `PascalCase` (e.g., `RandomForest`, `Flex`)
- Namespaces: nested under `Eloquent::ML::Port`

**Formatting**:

- 4 spaces indentation
- Opening braces on same line
- Comments with `// ` or `/* */` for blocks
- Sensor pin definitions as array: `Flex flex[5] = {Flex(36), ...}`

**Hardware Constants** (defined at top of file):

```cpp
#define VCC 5.0
#define R_DIV 10000.0
#define flatResistance 32500.0
#define bendResistance 76000.0
```

### Header Files (.h)

- Use `#pragma once` for include guards
- Namespaced under project-specific hierarchy
- Public methods documented with `/** */` comments
- Predictable array sizes (e.g., `float x[11]` for 11 features)

## Error Handling

### Python

- Check file existence with `os.path.exists()` before loading
- Print descriptive error messages in Chinese for local users
- Validate dataframes are not empty before processing

### Arduino

- Use `while(1)` loops with `delay(10)` for critical hardware init failures
- Print status messages via `Serial.println()`
- Reconnect logic for WiFi/server connections in `loop()`

## Data Handling

**CSV Data Format**:

- Columns: `flex_1` through `flex_5`, `Qw`, `Qx`, `Qy`, `Qz`, `ACCx_body`, etc.
- Target column: `Alphabet`
- Filter negative flex values to 0
- Drop rows with NaN in flex columns

**Feature Engineering**:

- Remove columns: `Alphabet`, `timestamp`, `user_id`, quaternions, ACC columns
- Keep only flex sensor values for training
- Normalize flex values: `(value - min) / (max - min)`

## Common Tasks

### Adding a New Sign/Gesture

1. Collect data via `Datasetcollection.ino`
2. Add new alphabet to `alphabets` list in `RF.py`
3. Retrain model and export to `classifier.h` via Micromlgen
4. Update `idxToLabel()` switch statement

### Modifying Sensor Configuration

- Flex sensor pins: 36, 39, 34, 35, 32 (analog)
- Update `Flex flex[5]` array initialization
- Adjust calibration loops (default: 1000 iterations)

### PCB Modifications

- Open `.kicad_pro` in KiCad 7.0+
- Backup files auto-saved to `smartglove-backups/`
- Update schematic first, then PCB layout

## Testing Checklist

- [ ] Python script loads all CSV files successfully
- [ ] Model accuracy > 85% on test set
- [ ] Arduino code compiles without warnings
- [ ] Sensor values print correctly to Serial Monitor
- [ ] WiFi connection establishes within 10 seconds
- [ ] Predictions match expected labels in `classifier.h`

## Dependencies

**Python**: pandas, numpy, scikit-learn, matplotlib, seaborn, joblib
**Arduino**: Adafruit MPU6050 library, FlexLibrary, Wire
**Hardware**: ESP32 dev board, MPU6050, 5x flex sensors
