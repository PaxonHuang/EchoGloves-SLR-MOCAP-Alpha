# SPEC-08 文档标准 (Documentation Standards)

> **项目**: 实时手语翻译 + 3D手部动画渲染数据手套系统
> **版本**: v1.0.0
> **日期**: 2025-01-XX
> **负责人**: 项目经理 / 技术文档工程师
> **状态**: Draft

---

## 目录

1. [文档体系架构](#1-文档体系架构)
2. [代码文档标准](#2-代码文档标准)
3. [API文档标准](#3-api文档标准)
4. [硬件文档标准](#4-硬件文档标准)
5. [SCI论文素材准备](#5-sci论文素材准备)

---

## 1. 文档体系架构

### 1.1 文档类型列表

本项目的文档体系按照用途和受众分为六大类别：需求文档（Requirement）、设计文档（Design）、接口文档（Interface）、测试文档（Test）、用户文档（User）和管理文档（Management）。每类文档服务于不同的项目阶段和读者群体，需求文档面向产品经理和客户，设计文档面向开发工程师，接口文档面向前后端协作，测试文档面向QA工程师，用户文档面向终端用户，管理文档面向项目干系人。文档类型的选择和创建遵循"够用即可、宁简勿繁"的原则，避免过度文档化导致维护负担。

**文档类型详细分类表：**

| 文档类别 | 文档名称 | 编号前缀 | 格式 | 主要受众 | 更新频率 | 存储位置 | 必需阶段 |
|----------|---------|---------|------|---------|---------|---------|---------|
| 需求 | 软件需求规格说明书 | SPEC-01 | Markdown | 全体成员 | 每阶段更新 | docs/requirements | P1启动时 |
| 需求 | 硬件需求规格说明书 | SPEC-02 | Markdown | 硬件工程师 | 每阶段更新 | docs/requirements | P1启动时 |
| 设计 | 系统架构设计文档 | SPEC-03 | Markdown | 技术负责人 | 每阶段更新 | docs/design | P1启动时 |
| 设计 | 数据库/数据模型设计 | SPEC-04 | Markdown | 后端/AI工程师 | 需求变更时 | docs/design | P2启动时 |
| 设计 | 算法设计文档 | SPEC-05 | Markdown/LaTeX | AI/算法工程师 | 迭代更新 | docs/design | P2启动时 |
| 测试 | 测试计划 | SPEC-06 | Markdown | QA工程师 | 每阶段更新 | docs/test | P1启动时 |
| 管理 | 项目管理规范 | SPEC-07 | Markdown | 项目经理 | 每阶段更新 | docs/management | P1启动时 |
| 标准 | 文档标准规范 | SPEC-08 | Markdown | 全体成员 | 需要时更新 | docs/standards | P1启动时 |
| 接口 | REST API文档 | API-XX | OpenAPI/YAML | 前后端开发 | 每次API变更 | docs/api | P3启动时 |
| 接口 | 通信协议文档 | PROTO-XX | Markdown | 固件/上位机 | 需求变更时 | docs/protocol | P1启动时 |
| 硬件 | 原理图文档 | HW-SCH-XX | PDF+源文件 | 硬件工程师 | 硬件迭代时 | hardware/schematic | P1启动时 |
| 硬件 | PCB文档 | HW-PCB-XX | Gerber+源文件 | 硬件工程师 | 硬件迭代时 | hardware/pcb | P1启动时 |
| 硬件 | BOM物料清单 | HW-BOM-XX | Excel+PDF | 采购/硬件 | 采购变更时 | hardware/bom | P1启动时 |
| 用户 | 用户手册 | USER-XX | Markdown/PDF | 终端用户 | 版本发布时 | docs/user | P3结束前 |
| 管理 | 会议纪要 | MEET-YYYYMMDD | Markdown | 参会人员 | 每次会议 | docs/meetings | 持续 |
| 管理 | 周报/月报 | RPT-YYYYWW | Markdown | 项目干系人 | 每周/每月 | docs/reports | 持续 |

### 1.2 文档编号规范

文档编号采用统一的层级编码体系，格式为 `[类别前缀]-[序号]-[版本号]`，确保每个文档在整个项目生命周期中具有唯一、可追溯的标识。类别前缀使用2~4个大写字母表示文档的所属类别（如 SPEC 表示规范文档、HW 表示硬件文档、API 表示接口文档）。序号使用两位数字，按创建顺序递增。版本号与文档内容的语义化版本保持一致，使用 `vX.Y.Z` 格式。文档的每次修改都需要更新版本号和变更历史表。

**文档编号规范表：**

| 编号组件 | 格式 | 说明 | 示例 |
|----------|------|------|------|
| 类别前缀 | 2~4位大写字母 | 文档所属类别 | SPEC, HW, API, TEST, USER |
| 子类别 | 可选，连字符分隔 | 文档细分类别 | HW-SCH, HW-PCB, HW-BOM |
| 序号 | 两位数字 | 同类文档的顺序编号 | 01, 02, 03 |
| 版本号 | vX.Y.Z | 语义化版本 | v1.0.0, v1.1.0 |
| 修订标识 | 可选，-revN | 同版本的修订次数 | v1.0.0-rev1 |

**文档编号示例：**

```
SPEC-01-v1.2.0          # 第1号规范文档，版本1.2.0
SPEC-06-v1.0.0          # 第6号规范文档（测试计划），版本1.0.0
HW-SCH-01-v2.0.0        # 第1号原理图文档，版本2.0.0
HW-PCB-01-v1.1.0-rev2   # 第1号PCB文档，版本1.1.0的第2次修订
API-v1-v2.0.0           # API文档，版本2.0.0
MEET-20250115-v1.0.0    # 2025年1月15日会议纪要
```

### 1.3 版本管理规则

文档版本管理遵循与代码版本管理相同的原则，使用 Git 进行版本控制。文档的版本号遵循语义化版本规范：主版本号（MAJOR）在文档结构重大变更时递增，次版本号（MINOR）在新增章节或内容时递增，修订号（PATCH）在修正错误或更新少量内容时递增。文档的每次版本变更需要在文档末尾的变更历史表中记录变更日期、作者和变更说明。已发布（Release）的文档版本不可直接修改，如需修正应创建新版本。

**文档版本状态定义：**

| 状态 | 标识 | 说明 | 编辑权限 | 分支要求 |
|------|------|------|---------|---------|
| 草稿 | [DRAFT] | 正在编写中，内容不完整 | 作者 | feature分支 |
| 评审中 | [REVIEW] | 已完成编写，等待评审 | 评审人+作者 | develop分支PR |
| 已批准 | [APPROVED] | 评审通过，等待发布 | 仅修正错别字 | develop分支 |
| 已发布 | [RELEASED] | 正式发布的稳定版本 | 不可修改 | main分支标签 |

**变更历史记录模板（每个文档末尾必须包含）：**

```markdown
> **文档变更历史：**
>
> | 版本 | 日期 | 作者 | 变更说明 | 评审人 |
> |------|------|------|---------|--------|
> | v1.0.0 | 2025-01-15 | 张三 | 初始版本 | 李四 |
> | v1.1.0 | 2025-02-01 | 张三 | 新增API接口章节 | 李四 |
> | v1.1.1 | 2025-02-10 | 王五 | 修正示例代码错误 | 张三 |
```

---

## 2. 代码文档标准

### 2.1 Doxygen 注释规范 (C/C++)

固件代码（C/C++）使用 Doxygen 作为文档生成工具，遵循其标准的注释格式。Doxygen 注释覆盖所有公共头文件中的函数声明、结构体定义、枚举定义和宏定义，以及源文件中关键算法的实现逻辑。注释语言使用英文（代码层面）+ 中文（详细说明）的双语策略，确保代码可读性和国际化兼容性。注释风格采用 JavaDoc 风格的 `/** ... */` 多行注释格式，便于 Doxygen 解析和生成美观的 HTML 文档。

**Doxygen 注释模板与示例：**

```c
/**
 * @file driver_tmag5273.h
 * @brief TMAG5273 3D霍尔效应传感器驱动程序
 * @details 本驱动支持通过I2C总线与TMAG5273通信，
 *          实现磁力计数据的读取、配置和校准功能。
 *          支持TCA9548A多路复用器实现最多15路传感器的并行管理。
 * @author 张三
 * @version v1.2.0
 * @date 2025-01-15
 * @copyright BSD 3-Clause License
 */

#ifndef DRIVER_TMAG5273_H
#define DRIVER_TMAG5273_H

/**
 * @brief TMAG5273传感器配置结构体
 * @details 包含传感器初始化和运行所需的全部参数。
 *          使用前必须通过tmag5273_init_config()进行默认值初始化。
 */
typedef struct {
    uint8_t i2c_addr;           ///< I2C从设备地址 (0x22/0x23/0x24/0x25)
    uint8_t mux_channel;        ///< TCA9548A多路复用器通道号 (0-7)
    uint8_t conversion_rate;    ///< 转换速率 (0=20Hz, 1=50Hz, 2=100Hz, 3=200Hz)
    float sensitivity;          ///< 灵敏度校准值 (默认40.0 LSB/mT)
    bool enabled;               ///< 传感器使能标志
} tmag5273_config_t;

/**
 * @brief TMAG5273传感器数据结构体
 * @details 存储三轴磁场强度数据和数据有效性标志。
 */
typedef struct {
    float x;        ///< X轴磁场强度 (单位: μT)
    float y;        ///< Y轴磁场强度 (单位: μT)
    float z;        ///< Z轴磁场强度 (单位: μT)
    float temp;     ///< 芯片温度 (单位: °C)
    bool valid;     ///< 数据有效性标志 (true=有效)
    uint32_t timestamp_ms;  ///< 采样时间戳 (单位: ms)
} tmag5273_data_t;

/**
 * @brief 初始化TMAG5273传感器
 *
 * 执行以下初始化步骤：
 * 1. 切换TCA9548A多路复用器到目标通道
 * 2. 检测传感器设备ID (WHO_AM_I寄存器，期望值0x34)
 * 3. 配置转换速率和灵敏度
 * 4. 执行首次读取验证
 *
 * @param config 传感器配置结构体指针
 * @return esp_err_t
 *   - ESP_OK: 初始化成功
 *   - ESP_ERR_INVALID_ARG: 参数无效
 *   - ESP_ERR_NOT_FOUND: 设备未响应
 *   - ESP_FAIL: 配置写入失败
 *
 * @note 调用前必须确保I2C总线已初始化
 * @warning 多路复用器通道切换后，其他通道的传感器将暂时不可访问
 *
 * @b 示例:
 * @code
 * tmag5273_config_t config = {
 *     .i2c_addr = 0x22,
 *     .mux_channel = 0,
 *     .conversion_rate = 1,  // 50Hz
 *     .sensitivity = 40.0f,
 *     .enabled = true
 * };
 * esp_err_t ret = tmag5273_init(&config);
 * if (ret != ESP_OK) {
 *     ESP_LOGE(TAG, "TMAG5273 init failed: %s", esp_err_to_name(ret));
 * }
 * @endcode
 *
 * @see tmag5273_read_data(), tmag5273_calibrate(), tmag5273_deinit()
 */
esp_err_t tmag5273_init(const tmag5273_config_t* config);

/**
 * @brief 读取TMAG5273传感器数据
 *
 * @param[out] data 读取数据存储的结构体指针
 * @return esp_err_t ESP_OK成功, 其他值表示错误
 *
 * @pre 传感器已通过tmag5273_init()成功初始化
 * @post data结构体中填入最新的三轴磁场数据
 */
esp_err_t tmag5273_read_data(tmag5273_data_t* data);
```

**Doxygen 注释标签速查表：**

| 标签 | 用途 | 使用位置 | 示例 |
|------|------|---------|------|
| `@file` | 文件描述 | .h/.c文件顶部 | `@file driver_tmag5273.h` |
| `@brief` | 简要说明 | 函数/结构体/文件 | `@brief 初始化传感器` |
| `@details` | 详细说明 | 函数/结构体/文件 | `@brief` 之后补充 |
| `@param` | 参数说明 | 函数声明 | `@param[in] config 配置参数` |
| `@return` | 返回值说明 | 函数声明 | `@return ESP_OK成功` |
| `@note` | 注意事项 | 函数/文件 | `@note 需提前初始化I2C` |
| `@warning` | 警告信息 | 函数 | `@warning 非线程安全` |
| `@pre` | 前置条件 | 函数 | `@pre 总线已初始化` |
| `@post` | 后置条件 | 函数 | `@post 数据已更新` |
| `@see` | 参考链接 | 函数/结构体 | `@see tmag5273_init()` |
| `@code`/`@endcode` | 代码示例 | 函数 | 配对使用 |
| `@todo` | 待办事项 | 任何位置 | `@todo 添加CRC校验` |
| `@bug` | 已知缺陷 | 任何位置 | `@bug 长时间运行可能超时` |
| `@b` | 粗体文本 | 任何位置 | `@b 示例:` |

### 2.2 Python docstring 规范 (Sphinx/RST)

上位机 Python 代码使用 Google Style Docstring 作为文档注释标准，配合 Sphinx 文档生成工具自动生成 API 参考文档。Docstring 覆盖所有公共模块、类、方法和函数，遵循"一句话摘要 + 详细说明 + 参数 + 返回值 + 异常 + 示例"的固定结构。类型注解（Type Hint）与 Docstring 中的参数说明保持一致，确保文档的准确性。文档语言以中文为主，类名、函数名、变量名和代码示例保持英文。

**Python Docstring 模板与示例：**

```python
"""
gesture_classifier.py — 手势分类推理服务模块

本模块提供基于 Attention-BiLSTM 模型的手势分类推理服务，
支持单样本推理、批量推理和流式推理三种模式。
推理服务封装了模型加载、预处理、推理执行和后处理的完整流程。
"""

from typing import List, Dict, Optional, Tuple
import numpy as np


class GestureClassifier:
    """基于Attention-BiLSTM的手势分类器。

    封装了OpenHands手势识别模型的加载、推理和结果后处理功能。
    支持CPU和GPU推理，自动检测可用设备并选择最优推理后端。

    Attributes:
        model_path (str): 模型文件路径。
        device (str): 推理设备 ('cpu' 或 'cuda')。
        num_classes (int): 分类类别数。
        input_dim (int): 输入特征维度。
        is_loaded (bool): 模型是否已加载。

    Examples:
        >>> classifier = GestureClassifier("models/best_model.pth")
        >>> result = classifier.predict(feature_vector)
        >>> print(result.label, result.confidence)
        '你好' 0.95
    """

    def __init__(self, model_path: str, device: Optional[str] = None):
        """初始化手势分类器。

        加载预训练模型并配置推理设备。如果未指定设备，自动检测
        CUDA可用性并选择最优后端。

        Args:
            model_path: 预训练模型文件路径（.pth格式）。
                支持绝对路径和相对于模型目录的相对路径。
            device: 推理设备字符串。可选值为 'cpu', 'cuda', 'cuda:0' 等。
                如果为 None，自动检测CUDA可用性。

        Raises:
            FileNotFoundError: 模型文件不存在时抛出。
            RuntimeError: 模型加载失败时抛出（如GPU内存不足）。
            ValueError: device参数无效时抛出。

        Note:
            首次加载模型时会有约2~5秒的初始化延迟。
            GPU推理需要至少2GB显存。

        Examples:
            >>> clf = GestureClassifier("model.pth", device="cuda:0")
            >>> print(clf.is_loaded)
            True
        """
        self.model_path = model_path
        self.device = device or self._auto_detect_device()
        self.is_loaded = False
        # ... 初始化逻辑 ...

    def predict(
        self,
        features: np.ndarray,
        top_k: int = 5
    ) -> List[Dict[str, float]]:
        """执行手势分类推理。

        对输入的特征向量执行前向推理，返回Top-K分类结果及其置信度。
        输入特征应已经过标准化处理，维度必须与模型输入层匹配。

        Args:
            features: 输入特征向量。形状为 (seq_len, input_dim) 的2D数组，
                或 (batch_size, seq_len, input_dim) 的3D数组用于批量推理。
            top_k: 返回的Top-K结果数量。必须为正整数且不超过类别数。

        Returns:
            分类结果列表。每个元素为字典，包含：
                - 'label': 手势标签字符串（如 '你好', '谢谢'）
                - 'confidence': 置信度浮点数（范围 0.0~1.0）
            列表按置信度降序排列。

        Raises:
            ValueError: 输入特征维度不匹配时抛出。
            RuntimeError: 模型未加载时抛出。

        Examples:
            >>> features = np.random.randn(30, 64)  # 30帧, 64维特征
            >>> results = clf.predict(features, top_k=3)
            >>> for r in results:
            ...     print(f"{r['label']}: {r['confidence']:.2%}")
            你好: 95.30%
            谢谢: 3.20%
            再见: 1.10%
        """
        # ... 推理逻辑 ...
        pass

    def predict_stream(
        self,
        feature_buffer: np.ndarray,
        window_size: int = 30,
        stride: int = 10
    ) -> List[Dict[str, float]]:
        """流式推理：使用滑动窗口在连续特征流上执行分类。

        当新特征帧到达时，使用滑动窗口机制从缓冲区中提取推理窗口，
        避免重复计算。支持特征帧的增量添加。

        Args:
            feature_buffer: 特征缓冲区（按时间顺序排列）。
                形状为 (buffer_size, input_dim)。
            window_size: 滑动窗口大小（帧数）。默认30帧（约600ms@50Hz）。
            stride: 窗口滑动步长（帧数）。默认10帧（约200ms@50Hz）。

        Returns:
            最新一个窗口的分类结果列表。格式同 predict() 返回值。

        Note:
            缓冲区长度需 >= window_size 才能产生有效推理结果。
            如果缓冲区不足，返回空列表。

        Warning:
            此方法会修改 feature_buffer 的内部状态（追加新帧），
            请确保在正确的线程上下文中调用。
        """
        pass
```

**Python Docstring 类型与规范速查：**

| 文档元素 | 格式要求 | 语言 | 示例 |
|----------|---------|------|------|
| 模块文档 | 三引号，首行一句话摘要 | 中文 | `"""手势分类推理服务模块"""` |
| 类文档 | 类定义下方，Google Style | 中文 | `class GestureClassifier:` + docstring |
| 方法摘要 | 一句话描述功能 | 中文 | `"""执行手势分类推理。"""` |
| Args说明 | `Args:` + 缩进列表 | 中文描述 + 英文参数名 | `features: 输入特征向量。` |
| Returns说明 | `Returns:` + 描述 | 中文 | `分类结果列表，按置信度降序。` |
| Raises说明 | `Raises:` + 缩进列表 | 中文 | `ValueError: 输入维度不匹配。` |
| Note/Warning | `Note:` / `Warning:` | 中文 | `Note: 需提前加载模型。` |
| Examples | `Examples:` + 代码块 | Python | `>>> clf.predict(features)` |
| 属性文档 | `Attributes:` 列表 | 中文 | `model_path (str): 模型路径。` |

### 2.3 JavaScript JSDoc 规范

前端 JavaScript/TypeScript 代码使用 JSDoc (TSDoc) 作为文档注释标准，配合 TypeDoc 工具自动生成 API 参考文档。对于 TypeScript 项目，类型注解已提供大部分类型信息，JSDoc 主要用于补充功能描述、使用示例和注意事项。注释风格遵循 TSDoc 规范，使用 `/** */` 多行注释格式。注释语言以中文为主，保持代码元素（函数名、参数名、类型名）为英文。

**JavaScript/TypeScript JSDoc 模板与示例：**

```typescript
/**
 * 手部3D渲染控制器
 *
 * 管理Three.js场景中的手部模型渲染、动画更新和用户交互。
 * 支持单手和双手渲染模式，自动适配不同设备性能。
 *
 * @example
 * ```typescript
 * const renderer = new HandRenderer(canvas, {
 *   handModel: 'ms-mano',
 *   quality: 'high',
 *   enableShadows: true
 * });
 * renderer.start();
 * ```
 */
export class HandRenderer {
  private scene: THREE.Scene;
  private camera: THREE.PerspectiveCamera;
  private handModels: Map<string, HandModel>;

  /**
   * 创建手部渲染控制器实例
   *
   * 初始化Three.js渲染器、场景、相机和灯光，
   * 加载指定的手部3D模型并配置渲染参数。
   *
   * @param canvas - 渲染目标Canvas元素
   * @param options - 渲染配置选项
   * @param options.handModel - 手部模型类型 ('ms-mano' | 'simplified')
   * @param options.quality - 渲染质量 ('low' | 'medium' | 'high')
   * @param options.enableShadows - 是否启用阴影效果，默认false
   * @param options.maxFPS - 最大帧率限制，默认60
   *
   * @throws {Error} 当WebGL不可用时抛出
   * @throws {Error} 当模型加载失败时抛出
   *
   * @see {@link HandModel} 手部模型类
   * @see {@link RenderOptions} 完整配置选项接口
   */
  constructor(
    canvas: HTMLCanvasElement,
    options: RenderOptions
  ) {
    // ...
  }

  /**
   * 更新手部姿态数据并触发渲染
   *
   * 接收来自上位机的手部姿态数据（关节角度/四元数），
   * 更新3D手部模型的姿态，并请求浏览器进行下一帧渲染。
   *
   * @param handId - 手部标识符 ('left' | 'right' | 'glove-1' 等)
   * @param poseData - 手部姿态数据，包含21个关节的位置/旋转信息
   * @param poseData.joints - 关节数组，按THREE.js手部骨架顺序排列
   * @param poseData.timestamp - 数据时间戳（毫秒）
   *
   * @returns 本次更新的渲染延迟（毫秒）
   *
   * @example
   * ```typescript
   * const delay = renderer.updateHandPose('left', {
   *   joints: [
   *     { position: [0, 0, 0], rotation: [0, 0, 0, 1] },
   *     // ... 其余20个关节
   *   ],
   *   timestamp: Date.now()
   * });
   * ```
   */
  updateHandPose(
    handId: string,
    poseData: PoseData
  ): number {
    // ...
  }
}

/**
 * 渲染配置选项接口
 *
 * @interface RenderOptions
 * @property {string} handModel - 手部3D模型类型
 * @property {('low'|'medium'|'high')} [quality='medium'] - 渲染质量等级
 * @property {boolean} [enableShadows=false] - 是否启用阴影
 * @property {number} [maxFPS=60] - 最大帧率限制
 * @property {THREE.Color} [backgroundColor] - 场景背景颜色
 */
export interface RenderOptions {
  handModel: string;
  quality?: 'low' | 'medium' | 'high';
  enableShadows?: boolean;
  maxFPS?: number;
  backgroundColor?: THREE.Color;
}
```

**JSDoc/TSDoc 标签速查表：**

| 标签 | 用途 | TypeScript中是否必需 | 示例 |
|------|------|-------------------|------|
| `@param` | 参数说明 | 否(已有类型注解) | `@param name - 名称` |
| `@returns` | 返回值说明 | 否 | `@returns 渲染延迟(ms)` |
| `@throws` | 抛出的异常 | 否 | `@throws {Error} WebGL不可用` |
| `@example` | 使用示例 | 否 | 配合代码块 |
| `@see` | 参考链接 | 否 | `@see {@link HandModel}` |
| `@interface` | 接口文档 | 否 | `@interface RenderOptions` |
| `@property` | 接口属性说明 | 否 | `@property name - 描述` |
| `@deprecated` | 废弃标记 | 否 | `@deprecated 使用新方法替代` |
| `@internal` | 内部API标记 | 推荐 | `@internal 仅供内部使用` |
| `@since` | 引入版本 | 推荐 | `@since v0.2.0` |

### 2.4 README.md 模板

每个代码仓库和子模块必须包含 README.md 文件，作为项目的入口文档和快速上手指南。README.md 采用统一的模板结构，确保所有模块的 README 具有一致的信息架构。README 分为基本信息、快速开始、使用说明、开发指南和附加信息五个部分。README 中尽量使用英文撰写（面向开源社区），复杂概念可辅以中文说明。

**README.md 标准模板：**

```markdown
# 模块名称 (Module Name)

[一句话描述模块的功能和用途]

[项目状态徽章: build status, coverage, version, license]

## 📋 目录 (Table of Contents)

- [概述](#概述)
- [快速开始](#快速开始)
- [使用说明](#使用说明)
- [开发指南](#开发指南)
- [API参考](#api参考)
- [配置](#配置)
- [故障排除](#故障排除)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

## 概述 (Overview)

[模块功能的详细描述，2~3段文字]

**核心特性:**
- ✅ 特性1的简述
- ✅ 特性2的简述
- ✅ 特性3的简述

**技术栈:**
| 组件 | 技术 | 版本 |
|------|------|------|
| 语言 | C/C++ / Python / TypeScript | - |
| 框架 | PlatformIO / FastAPI / Three.js | - |
| 依赖 | xxx, yyy, zzz | - |

## 快速开始 (Quick Start)

### 环境要求

| 要求 | 最低版本 | 推荐版本 |
|------|---------|---------|
| OS | Ubuntu 20.04 | Ubuntu 22.04 |
| 工具链 | xxx v1.0 | xxx v2.0 |
| 依赖 | - | - |

### 安装

\```bash
# 步骤1: 克隆仓库
git clone https://github.com/xxx/yyy.git

# 步骤2: 安装依赖
pip install -r requirements.txt

# 步骤3: 配置环境
cp .env.example .env
\```

### 运行

\```bash
# 启动服务
python main.py
\```

## 使用说明 (Usage)

[详细的使用说明和代码示例]

\```python
from module import Class

obj = Class(config)
result = obj.method(param)
print(result)
\```

## 开发指南 (Development)

### 项目结构

\```
module/
├── src/           # 源代码
├── tests/         # 测试代码
├── docs/          # 文档
├── examples/      # 示例
├── scripts/       # 工具脚本
└── README.md      # 本文件
\```

### 运行测试

\```bash
pytest tests/ -v --cov=src
\```

## 配置 (Configuration)

[配置项说明表格]

## 故障排除 (Troubleshooting)

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 问题1 | 原因描述 | 解决步骤 |
| 问题2 | 原因描述 | 解决步骤 |

## 许可证 (License)

[许可证类型和链接]
```

---

## 3. API文档标准

### 3.1 REST API 文档格式 (OpenAPI/Swagger)

本项目上位机提供的 REST API 文档使用 OpenAPI 3.0 规范（原Swagger规范）编写，配合 Swagger UI 或 Redoc 生成可交互的在线 API 文档。API 文档与代码同步维护，使用 `openapi.yaml` 文件作为单一事实来源（Single Source of Truth），通过自动化工具在 CI/CD 流水线中验证 API 文档与实际实现的兼容性。API 遵循 RESTful 设计原则，使用标准 HTTP 方法（GET/POST/PUT/DELETE）、标准状态码和统一的响应格式。

**OpenAPI 3.0 文档结构模板：**

```yaml
openapi: "3.0.3"
info:
  title: "手语翻译系统 API"
  description: |
    实时手语翻译 + 3D手部动画渲染数据手套系统的REST API接口文档。
    提供手势识别结果查询、实时数据流推送、模型管理和系统配置等功能。
  version: "1.0.0"
  contact:
    name: "API Support"
    email: "api@example.com"
  license:
    name: "MIT"
    url: "https://opensource.org/licenses/MIT"

servers:
  - url: "http://localhost:8000/api/v1"
    description: "本地开发环境"
  - url: "http://192.168.1.100:8000/api/v1"
    description: "局域网测试环境"

tags:
  - name: "System"
    description: "系统状态和管理接口"
  - name: "Gesture"
    description: "手势识别和翻译接口"
  - name: "Stream"
    description: "实时数据流接口 (SSE)"
  - name: "Model"
    description: "AI模型管理接口"
  - name: "Calibration"
    description: "传感器校准接口"
  - name: "Config"
    description: "系统配置接口"

paths:
  /status:
    get:
      operationId: "getSystemStatus"
      tags: ["System"]
      summary: "获取系统状态"
      description: |
        返回系统的整体运行状态，包括手套连接状态、
        AI模型状态、渲染引擎状态和系统资源使用情况。
      responses:
        "200":
          description: "系统状态正常"
          content:
            application/json:
              schema:
                $ref: "#/components/schemas/SystemStatus"
              example:
                status: "running"
                gloves:
                  - id: "glove-1"
                    hand: "left"
                    connected: true
                    battery_level: 85
                    sensor_count: 16
                model:
                  name: "attention-bilstm-v2"
                  loaded: true
                  device: "cuda:0"
                uptime_seconds: 3600
        "503":
          description: "服务不可用"
          content:
            application/json:
              schema:
                $ref: "#/components/schemas/ErrorResponse"

  /gesture:
    get:
      operationId: "getCurrentGesture"
      tags: ["Gesture"]
      summary: "获取当前识别结果"
      description: |
        返回最新的手势识别结果，包括Top-K预测和置信度。
        如果当前没有有效的手势识别结果，返回空结果。
      parameters:
        - name: top_k
          in: query
          description: "返回的Top-K结果数量"
          required: false
          schema:
            type: integer
            minimum: 1
            maximum: 20
            default: 5
      responses:
        "200":
          description: "识别结果"
          content:
            application/json:
              schema:
                $ref: "#/components/schemas/GestureResult"
              example:
                gesture: "你好"
                confidence: 0.953
                top_k:
                  - label: "你好"
                    confidence: 0.953
                  - label: "再见"
                    confidence: 0.025
                  - label: "谢谢"
                    confidence: 0.012
                timestamp: "2025-01-15T10:30:00.123Z"
                processing_time_ms: 12.5

  /gesture/stream:
    get:
      operationId: "streamGestures"
      tags: ["Stream"]
      summary: "手势识别实时数据流 (SSE)"
      description: |
        建立Server-Sent Events连接，实时推送手势识别结果。
        每次识别结果产生时推送一个事件，事件类型为 `gesture`。
        连接保持活跃，通过心跳事件（类型 `ping`）维持。
      responses:
        "200":
          description: "SSE事件流"
          content:
            text/event-stream:
              schema:
                type: string
                example: |
                  event: gesture
                  data: {"gesture":"你好","confidence":0.953}

                  event: ping
                  data: {"timestamp":"2025-01-15T10:30:01Z"}

components:
  schemas:
    SystemStatus:
      type: object
      required: [status, gloves, model]
      properties:
        status:
          type: string
          enum: [running, initializing, error, maintenance]
          description: "系统运行状态"
        gloves:
          type: array
          items:
            $ref: "#/components/schemas/GloveStatus"
        model:
          $ref: "#/components/schemas/ModelStatus"
        uptime_seconds:
          type: integer
          description: "系统运行时长（秒）"

    GestureResult:
      type: object
      required: [gesture, confidence, timestamp]
      properties:
        gesture:
          type: string
          description: "识别的手势标签"
        confidence:
          type: number
          format: float
          minimum: 0
          maximum: 1
          description: "识别置信度"
        top_k:
          type: array
          items:
            $ref: "#/components/schemas/Prediction"
          description: "Top-K预测结果"
        timestamp:
          type: string
          format: date-time
          description: "识别时间戳 (ISO 8601)"
        processing_time_ms:
          type: number
          format: float
          description: "推理处理时间（毫秒）"

    ErrorResponse:
      type: object
      required: [error, code, message]
      properties:
        error:
          type: string
          description: "错误类型"
        code:
          type: integer
          description: "HTTP状态码"
        message:
          type: string
          description: "错误详细描述"
        details:
          type: object
          description: "附加错误信息（可选）"

  securitySchemes:
    BearerAuth:
      type: http
      scheme: bearer
      bearerFormat: JWT
```

**API 响应格式统一规范：**

| HTTP状态码 | 含义 | 使用场景 | 响应体格式 |
|-----------|------|---------|-----------|
| 200 | OK | 成功请求 | `{ data: {...} }` |
| 201 | Created | 资源创建成功 | `{ data: {...}, id: "xxx" }` |
| 202 | Accepted | 异步任务已接受 | `{ task_id: "xxx", status: "pending" }` |
| 400 | Bad Request | 请求参数错误 | `{ error, code: 400, message }` |
| 401 | Unauthorized | 未认证 | `{ error, code: 401, message }` |
| 404 | Not Found | 资源不存在 | `{ error, code: 404, message }` |
| 422 | Validation Error | 参数验证失败 | `{ error, code: 422, message, details }` |
| 500 | Internal Server Error | 服务端错误 | `{ error, code: 500, message }` |
| 503 | Service Unavailable | 服务不可用 | `{ error, code: 503, message }` |

### 3.2 WebSocket/SSE 协议文档

除 REST API 外，系统还使用 Server-Sent Events (SSE) 和 WebSocket 两种实时通信协议。SSE 用于单向数据推送（服务器→客户端），如实时手势识别结果流、传感器数据流和系统状态更新流。WebSocket 用于双向实时通信，如固件与上位机之间的控制命令和数据交换。协议文档需要详细定义消息格式、事件类型、连接管理和错误处理规范。

**SSE 事件类型定义表：**

| 事件类型 | 方向 | 数据格式 | 推送频率 | 说明 |
|----------|------|---------|---------|------|
| `gesture` | Server→Client | JSON | 事件驱动 | 手势识别结果 |
| `sensor_data` | Server→Client | JSON/Binary | 50Hz | 原始传感器数据 |
| `pose_update` | Server→Client | JSON | 50Hz | 手部姿态数据 |
| `system_status` | Server→Client | JSON | 10s | 系统状态心跳 |
| `ping` | Server→Client | JSON | 5s | 连接保活心跳 |
| `error` | Server→Client | JSON | 事件驱动 | 错误通知 |
| `model_changed` | Server→Client | JSON | 事件驱动 | 模型切换通知 |

**SSE 事件消息格式示例：**

```
event: gesture
data: {"gesture":"你好","confidence":0.953,"top_k":[{"label":"你好","confidence":0.953}],"timestamp":"2025-01-15T10:30:00.123Z","processing_time_ms":12.5}
id: msg-00012345
retry: 3000

event: sensor_data
data: {"glove_id":"glove-1","sensors":[{"id":0,"x":1.23,"y":-0.45,"z":2.67}],"timestamp":1705312200000}
id: msg-00012346

event: ping
data: {"timestamp":"2025-01-15T10:30:05.000Z","uptime":3650}
id: msg-00012350
```

### 3.3 固件 API 文档

固件层的 API 文档描述固件内部模块间的接口契约，包括传感器驱动接口、通信接口、信号处理接口和AI推理接口。固件 API 文档以头文件（.h）的 Doxygen 注释为主要载体，辅以接口规格表（Markdown表格）进行补充说明。接口规格表明确列出每个函数的原型、功能描述、参数规格、返回值规格、线程安全性和调用约束等关键信息。

**固件核心接口规格表（示例）：**

| 模块 | 函数原型 | 功能 | 线程安全 | 调用约束 | 最大耗时 |
|------|---------|------|---------|---------|---------|
| tmag5273 | `esp_err_t tmag5273_init(const config_t*)` | 初始化传感器 | 否 | I2C已初始化 | < 100ms |
| tmag5273 | `esp_err_t tmag5273_read(data_t*)` | 读取数据 | 否 | 已初始化 | < 10ms |
| bno085 | `esp_err_t bno085_init(const config_t*)` | 初始化IMU | 否 | I2C/SPI已初始化 | < 200ms |
| bno085 | `esp_err_t bno085_get_quat(quat_t*)` | 获取四元数 | 否 | 已初始化 | < 5ms |
| espnow | `esp_err_t espnow_send(const uint8_t*, size_t)` | 发送数据 | 是 | 已初始化 | < 20ms |
| espnow | `void espnow_set_rx_callback(callback_t)` | 注册回调 | 否 | 任意时刻 | < 1ms |
| signal | `esp_err_t signal_filter(float*, size_t, filter_t)` | 滤波处理 | 是 | 缓冲区有效 | < 5ms |
| signal | `esp_err_t signal_extract_features(const data_t*, feat_t*)` | 特征提取 | 是 | 数据有效 | < 10ms |
| tflite | `esp_err_t tflm_load_model(const uint8_t*)` | 加载模型 | 否 | PSRAM可用 | < 100ms |
| tflite | `esp_err_t tflm_predict(const float*, result_t*)` | 执行推理 | 否 | 模型已加载 | < 20ms |

---

## 4. 硬件文档标准

### 4.1 原理图文档要求

原理图文档是硬件设计的核心交付物，必须包含完整的设计信息，使其他工程师能够基于文档独立复现或修改硬件设计。原理图文档分为电子版（EDA源文件 + PDF）和纸质版（A3打印），电子版使用 Git 进行版本管理。原理图设计必须遵循以下规范：使用标准化的元件符号和封装库、完整的网络标签命名、清晰的电源域划分和详细的注释说明。每个原理图页面需要包含标题栏（项目名称、版本、日期、设计者、审核者）。

**原理图文档交付清单：**

| 交付物 | 格式 | 要求 | 存储位置 | 命名规范 |
|--------|------|------|---------|---------|
| EDA源文件 | KiCad/Eagle/Altium项目文件 | 可编辑源文件 | hardware/schematic/ | HW-SCH-{rev}.{ext} |
| 原理图PDF | PDF | A3幅面，含标题栏 | hardware/schematic/ | HW-SCH-{rev}-v{ver}.pdf |
| 网络表 | Netlist | 用于PCB导入 | hardware/schematic/ | HW-SCH-{rev}.net |
| BOM参考 | CSV | 原理图自动生成 | hardware/schematic/ | HW-SCH-{rev}-bom.csv |
| 设计说明 | Markdown | 设计决策和特殊说明 | docs/hardware/ | HW-SCH-{rev}-notes.md |

**原理图设计规范要求：**

| 规范项 | 要求 | 检查方法 |
|--------|------|---------|
| 元件符号 | 使用项目统一符号库，禁止自定义临时符号 | 设计规则检查(DRC) |
| 元件值标注 | 每个无源器件标注具体值和封装 | 人工审查 |
| 网络标签 | 使用统一命名规范（如 VCC_3V3, I2C_SDA, GPIO_XX） | DRC + 人工审查 |
| 电源域划分 | 不同电压域使用不同颜色标注 | 人工审查 |
| 去耦电容 | 每个IC的电源引脚就近放置去耦电容 | DRC |
| 测试点 | 关键信号预留测试点 | 人工审查 |
| 标题栏 | 包含项目名、版本、日期、设计者、页码 | 人工审查 |
| 注释说明 | 特殊设计决策（如为什么选这个阻值）用注释标注 | 人工审查 |

### 4.2 PCB文档要求

PCB文档是在原理图基础上进行的物理布局布线设计，需要满足电气性能、EMC、可制造性（DFM）和可测试性（DFT）的要求。PCB文档交付包括 EDA 源文件、Gerber 文件（含钻孔文件）、BOM 文件、贴片坐标文件和 PCB 制造规格说明。PCB 设计必须通过设计规则检查（DRC）和可制造性检查（DFM Check）后才能提交制板。

**PCB文档交付清单：**

| 交付物 | 格式 | 要求 | 命名规范 |
|--------|------|------|---------|
| EDA源文件 | KiCad/Eagle/Altium项目 | 可编辑源文件 | HW-PCB-{rev}.{ext} |
| Gerber文件 | RS-274X | 含所有层（信号/电源/阻焊/丝印） | HW-PCB-{rev}-gerber.zip |
| 钻孔文件 | Excellon | 含所有过孔和安装孔 | 含于Gerber包 |
| BOM文件 | CSV/XLSX | 含MPN和供应商信息 | HW-PCB-{rev}-bom.xlsx |
| 贴片坐标 | CSV | SMT贴片机用 | HW-PCB-{rev}-pickplace.csv |
| PCB规格 | PDF | 层叠结构、阻抗控制、表面处理 | HW-PCB-{rev}-spec.pdf |
| 3D渲染图 | STEP/PDF | 用于机械装配验证 | HW-PCB-{rev}-3d.step |

**PCB设计规范要求：**

| 规范项 | 本项目要求 | 说明 |
|--------|-----------|------|
| 板层数 | 4层 (信号-地-电源-信号) | 满足阻抗控制和EMC要求 |
| 板厚 | 1.6mm | 标准厚度 |
| 铜厚 | 1oz (35μm) | 内外层均为1oz |
| 最小线宽/间距 | 6mil/6mil | 嘉立创标准工艺能力 |
| 最小过孔 | 0.3mm/0.6mm (孔/焊盘) | 标准机械过孔 |
| 阻抗控制 | 50Ω±10% (单端), 100Ω±10% (差分) | I2C/SPI信号 |
| 表面处理 | HASL无铅 | 成本优先 |
| 丝印颜色 | 白色 (绿油白字) | 标准配色 |
| 尺寸 | ≤ 50mm × 80mm | 适配手套尺寸 |
| 安装孔 | M2 × 4个 (四角) | 用于固定到手套骨架 |

### 4.3 BOM 文档要求

BOM（Bill of Materials）文档是硬件采购和成本管理的核心文件，必须完整、准确地列出所有电子元器件和结构件的信息。BOM 文档使用 Excel 格式（.xlsx）维护，同时导出 PDF 版本用于归档。每个物料需要包含完整的选型信息（物料编号、名称、规格、封装）、采购信息（制造商、MPN、供应商、单价、MOQ）和设计信息（参考设计值、原理图标号、PCB封装）。BOM 的每次修改需要更新版本号并记录变更原因。

**BOM 文档列定义表：**

| 列名 | 数据类型 | 必填 | 说明 | 示例 |
|------|---------|------|------|------|
| BOM_ID | 字符串 | 是 | 物料编号 | BOM-E001 |
| Category | 枚举 | 是 | 物料类别 (IC/被动件/连接器/PCB/结构件/其他) | IC |
| Name | 字符串 | 是 | 物料名称 | TMAG5273 |
| Description | 字符串 | 是 | 详细规格描述 | 3D Hall Effect Sensor, I2C, QFN-8 |
| Manufacturer | 字符串 | 是 | 制造商 | Texas Instruments |
| MPN | 字符串 | 是 | 制造商物料编号 | TMAG5273LCCR |
| Package | 字符串 | 是 | 封装类型 | QFN-8 (2mm×2mm) |
| Quantity | 整数 | 是 | 单板用量 | 15 |
| Reference | 字符串 | 是 | 原理图位号 | U1-U15 |
| Supplier1 | 字符串 | 是 | 首选供应商 | TI官方/立创商城 |
| Supplier1_PN | 字符串 | 是 | 供应商物料号 | TMAG5273LCCR |
| Supplier1_Price | 浮点数 | 是 | 单价(¥) | 6.50 |
| Supplier2 | 字符串 | 否 | 备选供应商 | - |
| MOQ | 整数 | 是 | 最小订购量 | 1 |
| Lead_Time | 字符串 | 是 | 交期 | 2-4周 |
| Notes | 字符串 | 否 | 备注说明 | 需要提前备货 |

### 4.4 机械设计文档要求

机械设计文档涵盖手套的物理结构设计，包括3D打印文件、柔性传感器安装方案和人体工学参数。机械设计使用 CAD 软件（如 Fusion 360 / SolidWorks）完成，交付物包括 3D 模型文件（STEP/STL）、工程图纸（PDF）和设计说明文档。机械设计需要与硬件设计紧密配合，确保传感器安装位置准确、线缆走线合理且穿戴舒适。

**机械设计文档交付清单：**

| 交付物 | 格式 | 要求 | 说明 |
|--------|------|------|------|
| 手套骨架3D模型 | STEP + STL | 可打印格式 | TPU材料，SLS工艺 |
| 手指关节模型 | STEP + STL | 每个手指独立模型 | 适配S/M/L三种尺寸 |
| 传感器安装夹具 | STEP + STL | 精确定位传感器 | 考虑公差补偿 |
| 整体装配模型 | STEP | 含所有零部件 | 验证装配干涉 |
| 工程图纸 | PDF (A3) | 含尺寸标注和公差 | 关键配合面标注 |
| 材料清单 | Markdown | 3D打印参数 | 填充率、支撑策略 |
| 设计说明 | Markdown | 人体工学考量 | 适配性、舒适度分析 |

---

## 5. SCI论文素材准备

### 5.1 实验数据记录模板

SCI论文的实验数据需要有完整的、可追溯的记录，确保实验结果的可重复性。实验数据记录模板涵盖每次实验的基本信息（日期、实验者、环境条件）、实验配置（模型参数、硬件配置、数据集版本）、实验过程（训练日志、中间结果）和实验结论（最终指标、对比分析）。所有实验数据使用标准化的目录结构存储，并使用 JSON/YAML 格式的元数据文件进行索引和描述。

**实验数据目录结构：**

```
experiments/
├── README.md                       # 实验数据总说明
├── config/
│   ├── model_configs/              # 模型配置文件
│   ├── dataset_configs/            # 数据集配置文件
│   └── experiment_configs/         # 实验配置文件
├── ablation/
│   ├── sensor_count/               # 传感器数量消融
│   │   ├── exp_5sensors/
│   │   │   ├── config.yaml         # 实验配置
│   │   │   ├── train_log.csv       # 训练日志
│   │   │   ├── eval_results.json   # 评估结果
│   │   │   ├── confusion_matrix.png
│   │   │   └── report.md           # 实验报告
│   │   ├── exp_10sensors/
│   │   └── exp_15sensors/
│   ├── model_architecture/         # 模型架构消融
│   ├── feature_type/               # 特征类型消融
│   ├── communication/              # 通信方案消融
│   └── filtering/                  # 滤波方法消融
├── baseline/                       # 基线实验
├── user_study/                     # 用户研究数据
│   ├── raw_data/                   # 原始采集数据
│   ├── questionnaires/             # 问卷数据
│   └── analysis/                   # 统计分析结果
└── figures/                        # 论文图表源文件
```

**实验记录元数据模板（config.yaml）：**

```yaml
experiment:
  id: "ABL-S-03"                    # 实验编号
  name: "传感器数量消融 - 15个TMAG5273"
  date: "2025-03-15"
  researcher: "张三"
  category: "ablation/sensor_count"

dataset:
  name: "CSL-50-v2.0"
  num_classes: 50
  num_samples_per_class: 200
  train_ratio: 0.8
  val_ratio: 0.1
  test_ratio: 0.1
  num_subjects: 8
  preprocessor_version: "v1.3.0"

model:
  architecture: "Attention-BiLSTM"
  framework: "PyTorch 2.1.0"
  input_dim: 45                     # 15传感器 × 3轴
  hidden_dim: 256
  num_layers: 2
  attention_heads: 8
  dropout: 0.3
  optimizer: "Adam"
  learning_rate: 0.001
  batch_size: 32
  epochs: 100

hardware:
  gpu: "NVIDIA RTX 3060 12GB"
  cpu: "Intel i7-12700"
  ram: "32GB DDR4"
  os: "Ubuntu 22.04 LTS"
  cuda_version: "12.2"

results:
  best_epoch: 78
  train_accuracy: 0.987
  val_accuracy: 0.892
  test_accuracy: 0.876
  test_top5_accuracy: 0.965
  avg_inference_time_ms: 9.8
  model_size_mb: 5.6
  confusion_matrix: "confusion_matrix.png"

notes: |
  15传感器配置下准确率显著高于10传感器(p<0.01)，
  但与15+BNO085基准配置差距不大(仅1.2%)。
  说明手腕IMU的贡献主要在动态手势场景。
```

### 5.2 图表生成规范

论文图表的质量直接影响审稿人的第一印象和论文的可接受度。本项目使用 Python matplotlib/seaborn 作为主要图表生成工具，所有图表使用统一的样式配置，确保视觉一致性。图表遵循 IEEE 期刊的格式要求：单栏宽度 3.5 英寸（88mm），双栏宽度 7.16 英寸（182mm），字体大小 8~10pt，线宽 0.5~1.5pt。所有图表保存为矢量格式（PDF/SVG）用于论文投稿，同时保存高分辨率位图（PNG, 300DPI）用于预览和演示。

**matplotlib 全局样式配置（plot_config.py）：**

```python
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np

# ==================== IEEE论文图表样式 ====================

# 字体设置 (中文论文使用SimHei, 英文论文使用Times New Roman)
FONT_FAMILY = 'serif'
FONT_SERIF = ['Times New Roman', 'DejaVu Serif']
FONT_SIZE_TITLE = 10
FONT_SIZE_LABEL = 9
FONT_SIZE_TICK = 8
FONT_SIZE_LEGEND = 8

# 颜色方案 (色盲友好)
COLORS = {
    'primary': '#0072B2',    # 蓝色
    'secondary': '#E69F00',  # 橙色
    'tertiary': '#009E73',   # 绿色
    'quaternary': '#CC79A7', # 粉色
    'quinary': '#56B4E9',    # 浅蓝
    'senary': '#D55E00',     # 红橙
    'accent': '#F0E442',     # 黄色
}

COLOR_SEQUENCE = ['#0072B2', '#E69F00', '#009E73', '#CC79A7',
                  '#56B4E9', '#D55E00', '#F0E442', '#000000']

# 图表尺寸 (IEEE标准)
FIG_SIZE_SINGLE_COL = (3.5, 2.5)    # 单栏: 3.5 × 2.5 英寸
FIG_SIZE_DOUBLE_COL = (7.16, 2.8)   # 双栏: 7.16 × 2.8 英寸
FIG_SIZE_SQUARE = (3.5, 3.5)        # 方形图

# 线型和标记
LINE_WIDTH = 1.2
MARKER_SIZE = 5
GRID_ALPHA = 0.3
DPI = 300  # 位图导出分辨率

def apply_ieee_style():
    """应用IEEE论文图表全局样式"""
    plt.rcParams.update({
        'font.family': FONT_FAMILY,
        'font.serif': FONT_SERIF,
        'font.size': FONT_SIZE_TICK,
        'axes.titlesize': FONT_SIZE_TITLE,
        'axes.labelsize': FONT_SIZE_LABEL,
        'xtick.labelsize': FONT_SIZE_TICK,
        'ytick.labelsize': FONT_SIZE_TICK,
        'legend.fontsize': FONT_SIZE_LEGEND,
        'figure.dpi': DPI,
        'savefig.dpi': DPI,
        'savefig.bbox': 'tight',
        'savefig.pad_inches': 0.05,
        'axes.linewidth': 0.5,
        'grid.linewidth': 0.3,
        'grid.alpha': GRID_ALPHA,
        'lines.linewidth': LINE_WIDTH,
        'lines.markersize': MARKER_SIZE,
        'pdf.fonttype': 42,           # TrueType字体嵌入
        'ps.fonttype': 42,
        'text.usetex': False,          # 不使用LaTeX渲染(加速)
    })

def save_figure(fig, name, formats=['pdf', 'png']):
    """保存图表为多种格式"""
    for fmt in formats:
        filepath = f"figures/{name}.{fmt}"
        fig.savefig(filepath, format=fmt, dpi=DPI,
                    bbox_inches='tight', pad_inches=0.05)
        print(f"Saved: {filepath}")
```

**标准图表类型与生成规范表：**

| 图表类型 | 用途 | 推荐尺寸 | 必需元素 | matplotlib函数 |
|----------|------|---------|---------|---------------|
| 准确率对比柱状图 | 模型/方法性能对比 | 双栏 | y轴标签、误差棒、图例、p值标注 | `plt.bar()` |
| 混淆矩阵热力图 | 分类错误分析 | 方形 | 色标、数值标注、类别标签 | `seaborn.heatmap()` |
| 训练曲线图 | 训练过程可视化 | 单栏 | 训练/验证曲线、最优点标注 | `plt.plot()` |
| 延迟分布图 | 系统性能分析 | 单栏 | P50/P95/P99线、箱线图 | `plt.boxplot()` |
| 传感器数量-准确率曲线 | 消融实验结果 | 单栏 | 误差带、显著性标记 | `plt.plot()` + `fill_between()` |
| 系统架构图 | 系统整体设计 | 双栏 | 模块标注、数据流箭头 | draw.io + export |
| 手套渲染截图 | 3D渲染效果展示 | 单栏 | 多角度视图、标注 | 截图 + 标注 |

### 5.3 对比实验设计

SCI论文需要与现有工作进行严格的对比实验，证明本系统/方法的优越性。对比实验分为定量对比（数值指标比较）和定性对比（视觉效果/用户体验比较）。定量对比需要选择具有代表性的基线方法，并在相同的数据集和评估条件下进行公平比较。定性对比需要通过用户研究或专家评审进行。对比实验设计需要确保实验条件的一致性（相同数据、相同评估协议、相同硬件平台），避免因实验条件差异导致的比较偏差。

**对比实验基线方法选择：**

| 对比维度 | 基线方法 | 来源 | 对比指标 | 预期优势 |
|----------|---------|------|---------|---------|
| 传感器方案 | 数据手套(弯曲传感器) | SignBridge [1] | 准确率、精度 | 磁力计精度更高 |
| 传感器方案 | 深度相机(MediaPipe) | Google | 准确率、延迟 | 无遮挡依赖 |
| AI模型 | Random Forest | scikit-learn | 准确率、延迟 | 深度学习优势 |
| AI模型 | 1D-CNN | [2] | 准确率、参数量 | 序列建模优势 |
| AI模型 | BiLSTM | [3] | 准确率 | 注意力增强 |
| 特征表示 | 纯时域特征 | - | 准确率 | 混合特征优势 |
| 特征表示 | 纯频域特征 | - | 准确率 | 多域互补 |
| 端到端延迟 | 传统ASR流程 | - | 延迟(ms) | 边缘+流式优势 |
| 3D渲染 | 无3D反馈 | 仅文本输出 | 用户满意度 | 可视化增强 |
| 整体系统 | 现有CSL翻译系统 | [4][5] | 综合指标 | 集成创新 |

**对比实验公平性保障措施：**

| 措施 | 说明 | 验证方法 |
|------|------|---------|
| 相同数据集 | 所有方法使用相同的训练/测试划分 | 数据版本控制 |
| 相同评估协议 | K-fold CV, 相同指标, 相同统计检验 | 脚本统一 |
| 相同硬件平台 | GPU型号、内存配置一致 | 配置记录 |
| 超参数搜索 | 每个方法进行独立的超参数优化 | 网格/随机搜索 |
| 多次运行 | 报告均值±标准差(≥5次) | 种子控制 |
| 统计显著性 | 使用t-test或Wilcoxon检验 | p<0.05 |

### 5.4 论文结构大纲 (IMRAD)

论文遵循 IMRAD（Introduction, Methods, Results, And Discussion）结构，针对 IEEE 期刊/会议的格式要求进行组织。以下为论文各章节的详细大纲，每个章节标注了预计页数、核心内容和关键图表安排。

**论文结构大纲表：**

| 章节 | 标题(拟定) | 预计页数 | 核心内容 | 关键图表 |
|------|-----------|---------|---------|---------|
| I. | Introduction | 1.5页 | 研究背景(手语交流障碍)、现有方法局限、本系统贡献(3~4点) | 图1: 系统概念图 |
| II. | Related Work | 1页 | 数据手套传感器技术、手语识别算法、3D手部建模、实时翻译系统 | 表1: 相关工作对比 |
| III. | System Architecture | 1.5页 | 整体架构设计、硬件选型与设计、通信协议、软件架构 | 图2: 系统架构图 |
| IV-A. | Hardware Design | 0.5页 | TMAG5273多路复用电路、ESP32-S3集成、手套机械结构 | 图3: 硬件架构图 |
| IV-B. | Signal Processing | 0.5页 | 滤波管道、特征提取(时域+频域)、Pose映射算法 | 图4: 信号处理流程 |
| IV-C. | AI Model | 1页 | Attention-BiLSTM架构设计、训练策略、边缘部署优化 | 图5: 模型架构图 |
| IV-D. | 3D Rendering | 0.5页 | Three.js + MS-MANO集成、实时动画更新 | 图6: 渲染流程 |
| V-A. | Experimental Setup | 0.5页 | 数据集描述(50类CSL)、评估指标、实验环境 | 表2: 数据集统计 |
| V-B. | Ablation Study | 1页 | 5组消融实验结果与分析 | 图7-11: 消融实验图 |
| V-C. | Comparison | 0.5页 | 与基线方法的定量对比 | 表3: 方法对比结果 |
| V-D. | Performance | 0.5页 | 延迟、功耗、渲染性能测试 | 表4: 性能指标 |
| V-E. | User Study | 0.5页 | UAT结果、满意度分析 | 表5: SUS评分 |
| VI. | Discussion | 0.5页 | 结果解读、局限性分析、未来工作方向 | - |
| VII. | Conclusion | 0.25页 | 主要贡献总结、性能总结 | - |
| Ref. | References | 0.75页 | 25~35篇参考文献 | - |
| App. | Appendix | 0.5页 | 补充实验数据、手势类别列表 | 表A1: 手势列表 |
| **合计** | | **~12页** | | |

**论文贡献点声明模板（Introduction末尾）：**

> The main contributions of this work are summarized as follows:
>
> 1. **硬件设计**: We propose a novel data glove design utilizing 15 TMAG5273 3D Hall-effect sensors and 1 BNO085 IMU, achieving high-precision finger pose estimation with a compact, low-cost form factor.
>
> 2. **AI模型**: We design an Attention-BiLSTM architecture for real-time Chinese Sign Language recognition, achieving 85~91% accuracy on a 50-class CSL dataset with <20ms inference latency on edge devices.
>
> 3. **系统集成**: We present a complete end-to-end system integrating edge computing (ESP32-S3 + TFLite Micro), real-time communication (ESP-NOW), cloud-side AI inference, and 3D hand animation rendering (Three.js + MS-MANO).
>
> 4. **全面评估**: We conduct extensive ablation studies and user studies (N=15) demonstrating the effectiveness and usability of the proposed system, with SUS score of XX and end-to-end latency of <200ms.

---

> **文档变更历史：**
>
> | 版本 | 日期 | 作者 | 变更说明 |
> |------|------|------|---------|
> | v1.0.0 | 2025-01-XX | 项目经理 | 初始版本 |
