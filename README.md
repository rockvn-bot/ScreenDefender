# ScreenDefender
A Windows security feature that prevents screen sharing, screenshots, and unauthorized screen capture. Includes dynamic watermarking to deter handcam leaks and protect confidential content.


# 🛡️ ScreenDefender

**ScreenDefender** is a Windows feature designed to protect your screen from unauthorized capture and sharing.  
It prevents **screen sharing**, **screenshots**, and **screen recording**, while overlaying a **dynamic watermark** to discourage handcam leaks.

---

## ✨ Features

- 🔒 **Screenshot & Screen Share Blocking**  
  Stops common capture tools (Snipping Tool, Print Screen, OBS, etc.) from grabbing protected content.
  
- 🖥️ **Secure Display Mode**  
  Ensures protected windows or the entire desktop cannot be duplicated or mirrored.

- 💧 **Dynamic Watermarking**  
  Displays a customizable watermark (like username, timestamp, or IP) to discourage photo leaks.

- ⚙️ **Native Windows Implementation**  
  Built entirely with Win32 APIs for maximum efficiency and system integration.

---

## 🧩 Build Instructions

### Requirements
- **CMake ≥ 3.20**
- **Visual Studio 2019 or newer**
- **Windows SDK**

### Build Steps

```bash
# Configure build system
cmake -S . -B build

# Build in Release mode
cmake --build build --config Release
