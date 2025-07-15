# Fox Render Engine

This is a Windows-specific graphics project built using **Vulkan** and the **Win32 API**, designed to test and experiment with:

- Physically-Based Rendering (PBR)
- Custom shaders (GLSL/HLSL to SPIR-V)
- Vulkan raytracing extensions

---

## Requirements

- Windows 10/11
- Vulkan SDK (1.3+)
- CMake 3.20+
- Visual Studio 2019/2022 or MSVC toolchain
- GPU with raytracing support (RTX 20xx+/RX 6000+ recommended)

---

## Build Instructions

```bash
git clone https://github.com/Niffoxic/FoxRenderEngine.git
cd FoxRenderEngine
mkdir build && cd build
cmake ..
cmake --build .
```
