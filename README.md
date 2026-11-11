# Gamma-Boost-Open-Sourced
This is Free option of Gamma Boost + this has more features, and its open sourced

# 🌸 GammaBoost+ (

**GammaBoost+** is a high-performance, zero-telemetry, 100% open-source C++ display enhancer, low-latency FPS channel optimizer, and gameplay accessibility tool for Windows.

Built natively with Win32, GDI+, Hardware Gamma LUT (`SetDeviceGammaRamp`), and the Windows DWM Color Matrix (`Magnification.dll`), it delivers instant dark-scene shadow lifting, vivid color saturation, game `.exe` auto-switching, dedicated OS latency tuning, and external overlay assistance.

---

## ✨ Features

### 🎨 Display Presets & Calibration
- **Instant Presets**: One-click calibrated visual profiles (`High Contrast`, `Warm Boost`, `Cool Boost`, `Forest Ops`, `Vivid`, `Color Boost`, `Everyday`, `Cinema`, `Tarkov++`, `ColorBoost+`, `BabyPink FPS`, `Night Ops`).
- **Interactive Split Preview**: Drag the `< >` divider live across test scenes (`Custom`, `Night`, `Cinematic`, `Nature`, `Interior`) to preview adjustments.
- **Full Light & Color Inspector**:
  - **Light**: Gamma (`0.50 - 2.80`), Contrast (`0.70 - 1.80`), Black Point (`0.00 - 0.15`), Shadows (`-20 to +50`).
  - **Color**: Saturation (`50% - 240%`), Vibrance (`50% - 220%`), Temperature (`4000K - 9000K`).
  - **Live RGB Curve**: Real-time rendering of active hardware LUT curves.

### ⚡ Dedicated FPS Channels
System and frame pacing optimizations organized into dedicated cards:
1. **Sub-Tick Timer** — Locks Windows kernel scheduler timer resolution to `0.50 ms` (`NtSetTimerResolution`) for ultra-low input latency.
2. **Thread Focus** — Grants game processes `HIGH_PRIORITY_CLASS` and disables Intel/AMD core power throttling.
3. **Standby Flow** — Automated standby memory cache cleaner and working set auto-trimming to eliminate micro-stutters.
4. **Direct Flip** — Hardware-accelerated GPU scheduling and flip-model optimizations.
5. **Core Unpark** — Unparks CPU cores for consistent frame-time pacing.
6. **Background Mute** — Removes multimedia network throttling and background GameDVR overhead.
7. **Raw Aim 1:1** — Disables Windows pointer acceleration curves for true raw linear mouse movement.
8. **Packet Stream** — Disables Nagle's TCP delay algorithm for faster server packet exchange.

### 🎯 Game Enhancements (External Overlays)
Dedicated accessibility and precision overlay tools running via lightweight desktop overlays:
- **Vision Target Box Overlay (`Alt+F6`)**: 50 Hz screen center scanning with momentum tracking (`vx, vy`), silhouette detection, and vehicle driver identification.
- **Steady Aim Pull (`Alt+F8`)**: Sub-pixel recoil compensation engine with selectable pull strength (`1px - 5px`) and ADS-only mode.
- **Eagle-Eye Center Zoom (`Alt+F7`)**: Instant hardware-scaled scope zoom (`1.0x - 3.0x`) with mouse wheel controls.

### 🎮 Game Profiles (`.exe` Auto-Detection)
- Bind any `.exe` on your PC to its own custom visual preset and performance profile.
- Automatically detects running games in the foreground and switches profiles seamlessly.

### 🎞️ Animated Retro Backdrop
- Fluid 64-frame rotoscoped animated background rendered in a custom Baby-Pink duotone palette.
- High-performance pre-cached RAM bitmap buffering with 0% disk I/O and zero frame-drops during window dragging.

### 💬 Discord & Open Source Ready
- **1-Click Share**: Export your active preset code directly from the **Discord & OSS** tab to share with teammates.
- **Single Portable Executable**: Statically linked binary with zero external runtime dependencies.

---

## 🛠️ Building from Source (MinGW-w64)

```bat
build.bat
```

Or compile manually:
```bat
windres resource.rc -O coff -o resource.res
g++ -O2 -mwindows -municode main.cpp resource.res -o GammaBoost.exe -lgdi32 -lgdiplus -lole32 -ldwmapi -luser32 -lcomctl32 -lcomdlg32 -lshell32 -lwinmm -lpsapi -ladvapi32 -static -static-libgcc -static-libstdc++
```

---

## 📄 License
Licensed under the [MIT License](LICENSE).

