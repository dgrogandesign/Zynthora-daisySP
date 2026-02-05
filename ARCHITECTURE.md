# Zynthora Technical Architecture

## The Event Backbone
The core differentiation of Zynthora is that it treats **Audio Generation** and **Control Events** as completely separate domains connected by a **Lock-Free Event Queue**.

### 1. Data Structure: `SynthEvent`
Every interaction in the system is normalized into a single struct.

```cpp
struct SynthEvent {
    uint64_t timestamp;  // High-resolution system time (nanoseconds)
    uint8_t source_id;   // Who sent this? (0x10=UserA_Module, 0x11=UserA_Phone, 0x20=UserB...)
    uint16_t param_id;   // Global Parameter ID (e.g., FILTER_CUTOFF)
    float value;         // Normalized value (0.0 - 1.0)
};
```

### 2. The Engine Loop (Audio Thread)
The Audio Callback consumes events from the **Lock-Free Queue**.

```mermaid
graph TD
    A[ESP32 Modules (BLE)] -->|Event| B(Central Event Queue)
    C[Phone GUI (WS)] -->|Event| B
    D[Internal Sequencer] -->|Event| B
    E[Audio Thread] -->|Poll Queue| B
    E -->|Update DSP| F[DaisySP Engine]
```

### 3. Hardware Ecosystem
*   **Brain:** Raspberry Pi Zero 2 W acts as **BLE Central**. It scans for advertising ESP32 modules.
*   **Modules:** ESP32-C3/S3 chips acting as **BLE Peripherals**.
    *   **Discovery:** Modules advertise their type (e.g., "Zyn_Seq", "Zyn_Bass").
    *   **Pairing:** The Brain automatically connects and assigns them a `source_id`.

### 4. Battery Power Strategy
*   **Brain:** Powered via USB Power Bank (5V).
*   **Modules:** Powered via LiPo Cells (3.7V). Deep sleep logic required for idle states.

### 5. Multi-User Logic
*   **Conflict:** "Last Message Wins" (Chaotic/Jam style).
*   **Locking:** Optional "Param Lock" feature to prevent other users from tweaking a specific knob.
