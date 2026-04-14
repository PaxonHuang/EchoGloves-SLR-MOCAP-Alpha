# Rendering-SOP: Unity 3D 实时渲染

## 1. Unity 环境配置
- **版本**: Unity 2022.3 LTS。
- **Package**: 安装 `XR Hands` 和 `Input System`。

## 2. ms-MANO 接入
- **模型**: 导入 ms-MANO 参数化手部模型。
- **解算**: 
  - 腕部: 直接应用 BNO085 四元数。
  - 手指: 将 15 路霍尔角度映射至 MANO 的 `pose_parameters` (48维)。

## 3. 数据同步
- **协议**: UDP 接收端。
- **平滑**: 使用 `Quaternion.Slerp` 和 `Vector3.Lerp` 减少数据抖动。

## 4. Claude Code 任务点
- "请编写一个 Unity C# 脚本，通过 UDP 接收 Protobuf 格式的手套数据，并将其映射至 XR Hands 的骨骼旋转参数中。"
