# Zynthora Project Context & Vision

## 🌌 The Vision: "Ghost Modular"
Zynthora is a hybrid hardware/software synthesizer ecosystem designed to leverage mobile device power to augment hardware synthesis. The core concept is a **"Ghost Modular"** system—where modules are physically disconnected but logically patched via wireless protocols.

## 🏗️ Architecture Summary
- **The Brain:** Raspberry Pi Zero 2 W running a headless C++ DSP engine.
- **DSP Core:** 
    - `DaisySP` (Ported from the Daisy Seed hardware) for high-quality oscillators, filters, and effects.
    - `miniaudio` for low-latency Linux audio output.
- **Control Interface:** 
    - `mongoose` web server running inside the C++ app.
    - Phone/Tablet connects via WiFi/USB -> Browser -> WebSocket.
    - Real-time parameter control via JSON-over-WebSocket.
- **Wireless Control:** ESP32 modules acting as physical controllers (knobs/sliders) connecting via BLE to the Pi.

## 📍 Current Project State (as of 2026-02-09)

### Core Synth Engine (`Zynthora-daisySP`)
- **Status:** Functional prototype with a web-based UI.
- **Current Modules:** PolyBLEP Oscillator, MoogLadder Filter, ReverbSc, Overdrive, Delay.
- **Recent Update:** Implemented a **Custom "Chorus 2"** effect. Unlike the standard DaisySP chorus, this uses dual delay lines with a stereo phase-inverted LFO to create a wider, more lush spatial effect.

### AI & Visualization (`Zynthora-AI`)
- **Status:** Exploring AI-driven patch generation and visual feedback.
- **Recent Update:** Added `visualizer.html` for real-time spectral feedback.

## 🛠️ Tech Stack
- **Language:** C++17
- **Libraries:** DaisySP, miniaudio, mongoose, nlohmann/json.
- **Platforms:** Linux (Target: Pi Zero 2 W), Web (Control UI).

## 🚀 Roadmap / Next Steps
1.  Finalize the "Event Backbone" for stable lock-free parameter updates.
2.  Optimize the Pi Zero 2 W performance (CPU usage monitoring).
3.  Expand the "Ghost Modular" patching system to allow dynamic routing between internal DSP modules.
