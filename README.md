# Zynthora (DaisySP Edition)

A headless synthesizer engine running on **Linux** (Raspberry Pi / Desktop), using **DaisySP** for sound generation and a **Web Interface** for control.

## 🚀 Concept
This project decouples the **Sound Engine** from the **User Interface**.
*   **The Brain (C++):** Runs on the hardware (Raspberry Pi Zero 2 W). Generates audio using the high-quality `DaisySP` library.
*   **The Screen (Phone/Browser):** A lightweight Web App (HTML/JS) served by the Pi. Connects via WebSockets to control the synth in real-time.

## 🎛 Features
*   **Oscillator:** PolyBLEP (Band-limited) Saw, Square, Triangle, Sine.
*   **Filter:** Moog Ladder Filter (4-pole Low Pass with Resonance).
*   **Envelope:** ADSR (Attack, Decay, Sustain, Release).
*   **Effects Chain:**
    1.  **Overdrive:** Analog-style saturation.
    2.  **Chorus:** Stereo width and modulation.
    3.  **Delay:** Stereo echo.
    4.  **Reverb:** Sean Costello `ReverbSc` (Lush, diffused tail).
*   **Control:** Virtual Keyboard and MIDI-mapped keys (A, W, S, E...).

## 🛠 Architecture
*   **Language:** C++17
*   **Audio Output:** `miniaudio` (ALSA/Jack/PulseAudio).
*   **DSP Library:** `DaisySP` (Electro-Smith).
*   **Web Server:** `mongoose` (Embedded Web Server + WebSockets).

## 📦 How to Build
### Prerequisites
*   Linux (tested on Pop!_OS / Debian / Raspberry Pi OS).
*   `g++`, `make`.

### Build & Run
```bash
# 1. Clone the repo
git clone https://github.com/dgrogandesign/Zynthora-daisySP.git
cd Zynthora-daisySP

# 2. Build
make

# 3. Run
./zynthora
```

### Usage
1.  Open your browser to `http://localhost:8000`.
2.  **Turn up the Volume.**
3.  **Press Keys (A, S, D, F...)** on your keyboard to play.
4.  Tweak the sliders to change the sound.

## 🎹 Signal Path
`[Oscillator] -> [Envelope] -> [Overdrive] -> [Filter] -> [Chorus] -> [Delay] -> [Reverb] -> [Master Vol] -> [Output]`
