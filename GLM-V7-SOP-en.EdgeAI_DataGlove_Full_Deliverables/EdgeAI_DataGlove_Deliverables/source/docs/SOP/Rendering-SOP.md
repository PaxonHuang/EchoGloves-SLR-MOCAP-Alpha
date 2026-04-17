# Rendering-SOP: 3D 实时渲染 (MVP 与后期高精度)

## 1. MVP 阶段: Tauri + React Three Fiber (Web/跨端)
- **目标**: 快速验证传感器数据与 3D 动画的映射关系，构建轻量级跨端仪表盘。
- **技术栈**: Tauri 2.0 (Rust UDP Listener) + React Three Fiber (R3F) + Zustand。
- **实现逻辑**:
  - Rust 后端接收 100Hz UDP 数据包，解析 Protobuf 并通过 Tauri Event 发送给前端。
  - 前端 Zustand Store 接收数据（使用 Transient Updates 避免 React 重渲染）。
  - R3F 的 `useFrame` 钩子读取 Zustand 数据，直接修改简易手部模型 (GLTF) 的骨骼旋转。

## 2. 后期高精度阶段: Unity + ms-MANO
- **目标**: 实现高逼真度、符合人体解剖学的 ms-MANO 参数化手部模型渲染。
- **技术栈**: Unity 2022.3 LTS + XR Hands Package。
- **实现逻辑**:
  - Unity C# 脚本直接监听 UDP 端口。
  - 将 15 路霍尔角度映射至 MANO 的 `pose_parameters` (48维)。
  - 腕部直接应用 BNO085 四元数。
  - 使用 `Quaternion.Slerp` 和 `Vector3.Lerp` 减少数据抖动。

## 3. Claude Code 任务点 (Prompt)
- **MVP 阶段**: "请使用 React Three Fiber 编写一个简易的手部骨骼渲染组件，使用 `useFrame` 钩子从 Zustand store 中读取最新的 15 个关节角度和腕部四元数，并应用到对应的 `THREE.Bone` 上。"
- **高精度阶段**: "请编写一个 Unity C# 脚本，通过 UDP 接收 Protobuf 格式的手套数据，并将其映射至 XR Hands 的骨骼旋转参数中。"
