# 智能手套ASL识别 - Jupyter Notebook 集合

## 项目概述

本目录包含多个Jupyter Notebook，涵盖从传统机器学习到深度学习、从特征工程到超参数优化的完整ASL手势识别解决方案。

---

## 📓 Notebook 列表

### 1. **RF.ipynb** (原始文件)
- **描述**: 原始随机森林训练代码
- **内容**: 基础数据加载、Random Forest分类器训练、评估
- **算法**: Random Forest (2000棵树)
- **适用场景**: 快速baseline建立

### 2. **Conv1D_BiLSTM_Training.ipynb** (已创建)
- **描述**: 深度学习时间序列建模
- **内容**: Conv1D + Bi-LSTM完整训练流程
- **算法**: 
  - Conv1D (3层: 32→64→128 filters)
  - Bi-LSTM (128→64 units)
  - 全连接分类层
- **数学公式**: 
  - 卷积操作: y_t = Σ(w_k · x_{t+k}) + b
  - LSTM: h_t = o_t ⊙ tanh(C_t)
  - Softmax: P(y=i|x) = exp(z_i) / Σexp(z_j)
- **特色**: 
  - 卡尔曼滤波数据预处理
  - TensorFlow Lite转换
  - ESP32部署代码生成
- **适用场景**: 生产级时间序列识别

### 3. **Traditional_ML_Comparison.ipynb** (已创建)
- **描述**: 传统机器学习算法对比
- **内容**: 5种经典算法完整对比
- **算法**:
  - **SVM (支持向量机)**: C=10, RBF核, 最大化分类间隔
  - **KNN (K近邻)**: K=5, 距离加权, 欧氏距离
  - **XGBoost**: 200棵树, max_depth=6, 梯度提升
  - **LightGBM**: 直方图算法, Leaf-wise生长
  - **AdaBoost**: 决策树基学习器, 自适应增强
- **数学公式**:
  - SVM: min 1/2||w||² + CΣξ_i
  - XGBoost: Obj = ΣL(y_i, ŷ_i) + ΣΩ(f_k)
  - KNN: d(x,y) = √Σ(x_i - y_i)²
- **特色**:
  - 完整数学公式推导
  - 交叉验证对比
  - 特征重要性分析
  - 超参数敏感性实验
- **适用场景**: 快速算法选择，资源受限环境

### 4. **Ensemble_Learning.ipynb** (已创建)
- **描述**: 集成学习方法探索
- **内容**: 多种集成策略实现
- **方法**:
  - **Voting (投票法)**:
    - 硬投票: ŷ = mode{h_1(x), ..., h_n(x)}
    - 软投票: ŷ = argmax Σw_i·P_i(y=c|x)
    - 加权投票: 自适应权重学习
  - **Stacking (堆叠法)**:
    - 手动实现
    - sklearn实现
    - 元学习器: Logistic Regression
  - **Blending (混合法)**:
    - 保留集验证
    - 快速原型
  - **权重优化**:
    - scipy.optimize最小化
    - 约束优化
- **数学公式**:
  - 加权平均: H(x) = Σα_t·h_t(x)
  - Stacking: H(x) = g(h_1(x), ..., h_n(x))
- **特色**:
  - 手动+sklearn双实现
  - 权重优化算法
  - 性能对比可视化
- **适用场景**: 最大化模型性能

### 5. **Hyperparameter_Optimization.ipynb** (已创建)
- **描述**: 超参数优化方法大全
- **内容**: 5种优化策略实现
- **方法**:
  - **网格搜索 (Grid Search)**:
    - 穷举搜索
    - 参数网格: n_estimators×max_depth×min_samples
    - 时间复杂度: O(n^k)
  - **随机搜索 (Random Search)**:
    - 随机采样50组
    - 速度提升: ~10倍
  - **贝叶斯优化 (Bayesian)**:
    - 高斯过程代理模型
    - 采集函数: EI, UCB
    - 样本效率最高
  - **Optuna**:
    - TPE采样器
    - 剪枝策略
    - 可视化
  - **粒子群优化 (PSO)**:
    - 粒子: 15个
    - 迭代: 20次
    - 速度更新: v = w·v + c1·r1·(pbest-x) + c2·r2·(gbest-x)
- **数学公式**:
  - PSO: x_i(t+1) = x_i(t) + v_i(t+1)
  - EI: E[max(f(θ) - f(θ⁺), 0)]
- **特色**:
  - 收敛曲线可视化
  - 时间-准确率权衡分析
  - 实际部署建议
- **适用场景**: 模型调优，性能最大化

### 6. **model_deployment.ipynb** (原始文件)
- **描述**: 模型部署代码
- **内容**: 树莓派/PC接收ESP32数据并推理
- **通信**: TCP Socket (端口4444)
- **输出**: 文字 + TTS语音

---

## 🎯 推荐使用流程

### 阶段1: 快速原型 (1-2天)
1. 运行 `RF.ipynb` 建立baseline
2. 运行 `Traditional_ML_Comparison.ipynb` 选择最佳传统算法

### 阶段2: 性能提升 (3-5天)
3. 运行 `Ensemble_Learning.ipynb` 尝试集成方法
4. 运行 `Hyperparameter_Optimization.ipynb` 优化超参数

### 阶段3: 深度学习 (5-7天)
5. 运行 `Conv1D_BiLSTM_Training.ipynb` 训练时间序列模型

### 阶段4: 部署 (2-3天)
6. 使用 `model_deployment.ipynb` 部署到目标设备

---

## 📊 算法对比总结

| 方法 | 准确率 | 速度 | 复杂度 | 适用场景 |
|------|--------|------|--------|----------|
| Random Forest | ~81% | 快 | 低 | Baseline |
| SVM | ~85% | 中 | 中 | 高维数据 |
| XGBoost | ~87% | 快 | 中 | 生产环境 |
| LightGBM | ~87% | 很快 | 中 | 大规模数据 |
| Stacking | ~89% | 慢 | 高 | 竞赛/研究 |
| Conv1D+BiLSTM | ~92% | 中 | 高 | 时间序列 |

---

## 🔧 环境要求

### 基础依赖
```bash
pip install numpy pandas matplotlib seaborn scikit-learn
```

### 梯度提升库
```bash
pip install xgboost lightgbm
```

### 深度学习
```bash
pip install tensorflow keras
```

### 超参数优化
```bash
pip install scikit-optimize optuna
```

### 遗传算法
```bash
pip install deap
```

---

## 📖 学习路径建议

### 初学者
1. RF.ipynb → 理解基础流程
2. Traditional_ML_Comparison.ipynb → 了解不同算法

### 进阶
3. Ensemble_Learning.ipynb → 学习集成方法
4. Hyperparameter_Optimization.ipynb → 掌握调参技巧

### 高级
5. Conv1D_BiLSTM_Training.ipynb → 深度学习应用

---

## 📁 文件结构

```
Smart-glove-main/
├── RF.ipynb                          # 原始随机森林
├── model_deployment.ipynb            # 部署代码
├── Conv1D_BiLSTM_Training.ipynb      # 深度学习 (已创建)
├── Traditional_ML_Comparison.ipynb   # 传统ML对比 (已创建)
├── Ensemble_Learning.ipynb           # 集成学习 (已创建)
├── Hyperparameter_Optimization.ipynb # 超参数优化 (已创建)
└── ML_Notebooks_Guide.md             # 本指南
```

---

## 💡 提示

1. **内存限制**: 如果内存不足，在数据加载时使用子采样
2. **时间限制**: 超参数优化时可以减少迭代次数
3. **GPU加速**: 深度学习notebook自动检测GPU
4. **中文支持**: 所有notebook已配置matplotlib中文显示

---

## 📞 支持

如有问题或建议，请提交Issue。

**最后更新**: 2026-02-15  
**版本**: v1.0
