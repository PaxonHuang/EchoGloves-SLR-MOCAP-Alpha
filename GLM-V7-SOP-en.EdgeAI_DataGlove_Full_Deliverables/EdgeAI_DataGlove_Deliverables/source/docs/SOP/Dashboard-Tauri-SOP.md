# Dashboard-Tauri-SOP: 跨端仪表盘开发指南

## 1. 架构选择与优势
- **框架**: Tauri 2.0 + Next.js (React) + TailwindCSS + Zustand + React Three Fiber (R3F)。
- **为什么不用纯 Web**: 纯 Web 浏览器无法监听 UDP 端口，无法接收 ESP32 发出的 100Hz 3D 渲染数据包。
- **为什么不用 Electron**: Tauri 打包体积小（几MB），且 Rust 后端处理 UDP Socket 性能极高，内存占用极低。

## 2. Rust 后端 (src-tauri)
- **UDP 监听**: 在 `main.rs` 中开启 `std::net::UdpSocket` 监听 `0.0.0.0:8888`。
- **数据反序列化**: 使用 `prost` 库解析 Protobuf 数据帧。
- **事件分发**: 使用 `app.emit_all("glove-data", payload)` 将解析后的数据以 100Hz 频率发送给前端。

## 3. React 前端 (src)
- **状态管理 (Zustand)**:
  - **关键优化**: 绝对不能将 100Hz 的数据存入触发 React 渲染的 State 中。必须使用 Zustand 的 `useStore.getState()` 进行瞬态读取（Transient Updates）。
- **3D 渲染 (React Three Fiber)**:
  - 导入简易的手部模型（GLTF/GLB）。
  - 在 `useFrame` 钩子中，直接读取 Zustand 中的最新传感器角度，并应用到骨骼节点的 `rotation` 属性上。
- **UI 组件**: 使用 `shadcn/ui` 构建仪表盘卡片，展示延迟、连接状态、当前识别的手势及 NLP 翻译结果。

## 4. Claude Code 任务点 (Prompt)
> "请初始化一个 Tauri 2.0 + React + TypeScript 项目。在 Rust 端实现一个 UDP 监听器，接收 Protobuf 格式的数据并 emit 给前端。在前端使用 Zustand 创建一个不触发重渲染的 store，并使用 React Three Fiber 的 `useFrame` 钩子读取该 store，实时更新 3D 手部模型的骨骼旋转。"
