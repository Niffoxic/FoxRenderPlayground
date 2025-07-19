# Fox Render Engine

> Current Output its nothing special right now but will fun to see later
![Fox Render Engine Demo](doc/output/output-1.gif)

This is a Windows-specific graphics project built using **Vulkan** and the **Win32 API**, designed to test and experiment with:

- Physically-Based Rendering (PBR)
- Custom shaders (GLSL/HLSL to SPIR-V)
- Vulkan raytracing extensions

---

## Requirements

- Windows 10/11
- Vulkan SDK (1.3+) `note: can be downloaded with gui installer`
- CMake 3.90+ `note: can be downloaded with gui installer`
- Visual Studio 2019/2022 or MSVC toolchain
- GPU with raytracing support (RTX 20xx+/RX 6000+ recommended)

---

## Build Instructions

> Recommended: Use the GUI installer for first-time setup. It handles everything — installing CMake (if missing), Vulkan SDK, compiling shaders, and building the project.

### 🔄 Quick Start with GUI Installer

![GUI Installer Preview](doc/installer-image.png)

```bash
cd FoxRenderEngine/setup
pip install -r requirements.txt
python build_installer.py
```

`Note: I will update how to build it without gui installer just in case if anyone don't trust me with .exe file XD (I will do it later tho I gotta go back to the main vulkan stuff`

This project is licensed under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).
