# Zynthora - Research & Technical Feasibility

## 🧠 Core Philosophy
**Goal:** Create a "Hybrid" Synth Ecosystem.
*   **DSP Engine:** Headless Raspberry Pi Zero 2 W (Linux).
*   **User Interface:** Mobile Device (Phone/Tablet) via Web/BLE.
*   **Physical Controls:** Custom PCB with ESP32-C3 via BLE.

## 📊 Hardware Capability Analysis (Pi Zero 2 W)

| Feature | Specs | Implications for Zynthora |
| :--- | :--- | :--- |
| **CPU** | Quad-core Cortex-A53 @ 1GHz | Massive headroom for synthesis. Can run 100+ oscillators or complex physical modeling. Far superior to STM32H7 (Daisy). |
| **RAM** | 512MB LPDDR2 | Huge for wavetables/samples. Can hold entire drum kits or multi-sampled pianos in RAM. (Daisy has 64MB SDRAM). |
| **Audio I/O** | No analog input/output | **Requires I2S DAC.** The PCM5102 is the best budget choice (Line Level). |
| **WiFi** | 802.11 b/g/n (2.4GHz) | Sufficient for Web UI control. (Note: Zero 2 W only does 2.4GHz, unlike Pi 4). |
| **Bluetooth** | BT 4.2 / BLE | Native BLE support means **No extra dongles needed** for your ESP32 controllers. |
| **Boot Time** | ~20-30s (Linux) | Slower than MCU (Instant). Mitigation: Optimize `systemd` or use "Bare Metal" OS (Circle) if critical. |

## 🔊 Audio Quality & DAC Options

| DAC Option | Chip | Quality | Price | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Generic I2S Board** | **PCM5102A** | **High** (112dB SNR) | ~£3 | **Winner.** Line-level output (2.1V RMS). No hardware volume control (must do in software). Great bass response. |
| **HiFiBerry DAC+ Zero** | PCM5122 | High (112dB SNR) | ~£15 | Adds hardware volume mixer. Overkill if doing software mixing. |
| **PWM (Native)** | None | Terrible | £0 | Don't bother. Noisy, 11-bit equivalent. |
| **USB DAC** | Various | Variable | £5-£50 | Adds USB latency overhead. Avoid for real-time synth. |

**Recommendation:** Stick with the **PCM5102A**. It is the industry standard for "Good Cheap I2S". It drives Line Level perfectly.

## 🛠️ Software Ecosystem & Libraries

### 1. The "DaisySP on Linux" Approach (Current)
*   **Pros:** High-quality DSP code (Moog filters, Reverbs), portable to hardware later.
*   **Cons:** Not "native" Linux optimized (single threaded by default).
*   **Optimization:** Use `taskset` to pin the audio thread to a specific CPU core on the Pi Zero 2 W to avoid jitter.

### 2. Alternative Libraries
*   **Tracktion Engine:** Full DAW engine (open source). Overkill but powerful.
*   **FluidSynth:** If you want SoundFonts (Real pianos/strings).
*   **SuperCollider / PureData:** Great for prototyping, but heavier than compiled C++.

## ⚠️ Potential Bottlenecks & Solutions

### 1. Latency (The Big One)
*   **Problem:** Linux is not a Real-Time OS. Background tasks (WiFi scanning) can cause audio glitches (Xruns).
*   **Solution:** Use **Patchbox OS** or configure a `PREEMPT_RT` kernel.
*   **Target:** With optimized ALSA/Jack, you can get **< 10ms** latency on Pi Zero 2 W, which is "feeling instant."

### 2. Network Jitter (UI)
*   **Problem:** WiFi interference makes sliders laggy.
*   **Solution:** Your architecture (BLE for knobs, WiFi for Screen) is **perfect**. BLE is low-bandwidth but low-latency for controls. WiFi handles the visual "heavy lifting" (Waveforms/UI) where latency matters less.

## 🔮 Future Ideas
*   **Granular Synthesis:** The Pi has enough RAM to record 5 minutes of audio and granulate it live. Daisy Seed struggles with >1min buffers.
*   **Sampler:** You can stream disk-based samples (Gb of piano samples) which is impossible on MCUs.
*   **Spectral Processing:** FFT-based effects (vocoders, spectral freezing) are CPU heavy but viable on Quad-Core A53.

## 🔗 Related Projects
*   **Zynthian:** Open synth platform on Pi (uses encoders/screen).
*   **LMN-3:** Open source DAW-in-a-box using Pi 4.
*   **MicroDexed:** DX7 emulator running on bare-metal Pi.
