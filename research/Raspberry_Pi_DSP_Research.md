# Raspberry Pi DSP Project Research

## Architectural Framework for a Distributed Headless Digital Signal Processing System on ARMv8 Embedded Platforms

The transition from monolithic hardware synthesizers to modular, software-defined digital signal processing (DSP) systems has been accelerated by the availability of high-performance, low-power embedded computing platforms. The development of a headless DSP system—wherein the synthesis engine is decoupled from the user interface and physical control surfaces—represents a significant advancement in digital lutherie. By utilizing the **Raspberry Pi Zero 2 W** as the computational core, developers can leverage a quad-core 64-bit ARM architecture to execute complex DSP algorithms in a compact form factor. This report explores the architectural requirements, software frameworks, and communication protocols necessary to implement such a system using the **DaisySP** library as a synthesis backbone, integrated with a web-based graphical user interface (GUI) and a modular hardware ecosystem.

### Computational Foundation: The Raspberry Pi Zero 2 W Architecture
The selection of the Raspberry Pi Zero 2 W is predicated on its balance between computational throughput and physical footprint. At the heart of the device is the **RP3A0 System-in-Package (SiP)**, which incorporates a quad-core ARM Cortex-A53 CPU clocked at 1.0 GHz. This represents a significant shift from the original single-core Raspberry Pi Zero, providing approximately five times the multi-threaded performance. For a real-time DSP application, this multi-core capability is essential for segregating the high-priority audio callback from lower-priority tasks such as web server management and wireless communication.

The SiP also integrates 512MB of LPDDR2 SDRAM. While this memory capacity is modest compared to desktop systems, it is sufficient for many synthesis methods provided by DaisySP, such as subtractive and physical modeling. However, memory management becomes a critical design factor when implementing granular synthesis or large-scale sampling, as these methods require significant buffers for audio data storage and real-time manipulation. The LPDDR2 interface must be managed carefully to avoid bus contention between the CPU and the GPU, particularly if any video streaming or complex graphical processing is attempted, although the headless nature of this project mitigates such risks.

| Hardware Feature | Raspberry Pi Zero 2 W Specification | Implications for DSP and Connectivity |
| :--- | :--- | :--- |
| **Processor** | Quad-core 64-bit ARM Cortex-A53 @ 1GHz | Supports parallel synthesis voices and concurrent networking threads. |
| **Memory** | 512MB LPDDR2 SDRAM | Adequate for synthesis; requires optimization for granular/sampling. |
| **WiFi** | 2.4GHz 802.11 b/g/n | Facilitates the headless web-based GUI and remote management. |
| **Bluetooth** | Bluetooth 4.2 & BLE (Bluetooth Low Energy) | Enables interaction with wireless MIDI controllers and bespoke BLE modules. |
| **I/O Pins** | 40-pin GPIO (I2C, SPI, UART, PWM) | Allows for modular hardware expansion via standard serial protocols. |
| **Form Factor** | 65mm x 30mm | Ideal for embedding into custom enclosures and "exotic" instruments. |

### Audio Hardware Abstraction and DAC Integration
The Raspberry Pi Zero 2 W lacks a high-quality integrated audio output, necessitating the use of an external Digital-to-Analog Converter (DAC). For professional-grade DSP, the interface must support low-latency **I2S (Inter-IC Sound)** communication. Common choices include the **Raspberry Pi Codec Zero**, which features a high-performance 24-bit audio codec and supports sample rates up to 96 kHz.

Alternatively, the **Hifiberry DAC+** or **DAC+ADC** stages are frequently utilized in embedded musical projects like Zynthian due to their use of Burr-Brown (Texas Instruments) DACs, which offer superior signal-to-noise ratios and dynamic range. The integration of these hats requires careful pin management, as the I2S interface occupies specific GPIO pins (typically Pins 12, 35, 38, and 40 for PCM audio).

| Audio HAT / Codec | Resolution | Input/Output Config | Targeted Application |
| :--- | :--- | :--- | :--- |
| **Codec Zero** | 24-bit / 96kHz | Stereo Out / Mono Mic In | Compact, low-power general audio. |
| **Hifiberry DAC+ADC** | 24-bit / 192kHz | Stereo Out / Stereo Line In | High-fidelity synthesis and sampling. |
| **IQaudio DAC+** | 24-bit / 192kHz | Stereo Out | Standard high-quality output. |
| **Pimoroni pHAT DAC** | 24-bit / 192kHz | Stereo Out | Minimalist, cost-effective synthesis. |

### DSP Framework: Architectural Integration of DaisySP
**DaisySP** is an open-source, modular DSP library written in C++, originally designed for the Electrosmith Daisy platform. Its architecture is highly portable, making it an excellent choice for a Raspberry Pi-based system. The library provides a wide array of synthesis components, including oscillators (sine, sawtooth, square, pulse, triangle), noise generators, filters (state-variable, Moog ladder), and more complex synthesis engines like Karplus-Strong physical modeling and granular players.

#### Porting DaisySP to a Linux Environment
While DaisySP is often used in bare-metal environments, its core components are platform-agnostic. To utilize it on a Raspberry Pi running Linux, the library must be compiled within a wrapper that manages the audio callback. A typical implementation involves using the **Advanced Linux Sound Architecture (ALSA)** or the **JACK Audio Connection Kit (JACK)**. ALSA provides the low-level interface to the hardware, while JACK offers a more flexible routing system for interconnecting multiple audio applications.

The **audio callback** function is the heart of the real-time synthesis engine. In C++, this is typically a function that is called by the audio driver whenever the output buffer requires new samples. Within this callback, the DaisySP objects are processed iteratively to fill the buffer.

Deterministic execution within this callback is paramount. Any non-deterministic operations, such as heap memory allocation (`new` or `malloc`) or blocking I/O, must be strictly avoided to prevent audio dropouts, commonly known as glitches.

#### Synthesis Modules and Computational Optimization
For a multi-module system, the DaisySP library must be organized to allow dynamic switching or concurrent execution of different synthesis engines. A modular C++ architecture can utilize polymorphism or template-based dispatching to handle different engine types (percussion, subtractive, granular).

Research into porting DaisySP to other ARM platforms, such as the Teensy 4.x (Cortex-M7), indicates that high-voice polyphony is achievable. For instance, a 16-voice polyphonic synth with 64 oscillators and 16 Moog filters was shown to utilize approximately 69% of a Teensy 4.x CPU. On the Raspberry Pi Zero 2 W, which features more powerful A53 cores but a slower clock speed and higher OS overhead, similar polyphony can be expected if the code is optimized for the ARMv8 instruction set, potentially utilizing **NEON SIMD** (Single Instruction, Multiple Data) instructions for parallel sample processing.

### Operating System Selection for Low-Latency Audio
Standard Linux distributions, like Raspbian (Debian-based), are not optimized for real-time audio out of the box. The scheduler in a general-purpose OS can de-schedule the audio thread in favor of background tasks, leading to buffer underruns. Two primary solutions exist for the Raspberry Pi platform: **Patchbox OS** and **Elk Audio OS**.

#### Patchbox OS and the PREEMPT_RT Kernel
**Patchbox OS** is a community-driven distribution pre-configured for low-latency audio. It utilizes a **real-time (RT) patched kernel**, which allows for higher-priority task preemption, significantly reducing the maximum latency of the audio callback. Patchbox includes several integrated tools, such as `patchbox-config`, which facilitates the setup of I2S soundcards and WiFi-MIDI bridges.

Key features of Patchbox OS include:
*   **JACK Backend:** Provides robust audio management and interconnection between apps like Pure Data or SuperCollider.
*   **MODEP:** A virtual pedalboard for hosting LV2 plugins, which can be used alongside bespoke C++ synthesis code.
*   **Amidiauto:** Automatically manages MIDI connections, easing the integration of USB and BLE MIDI controllers.

#### Elk Audio OS and Xenomai
For applications requiring even lower latency, **Elk Audio OS** provides a more radical architecture. Instead of the standard Linux scheduler, Elk uses the **Xenomai** real-time kernel extension. This allows the audio engine, known as **Sushi**, to run in a dedicated real-time domain with ultra-low jitter and round-trip latencies as low as 1ms.

The core audio engine, Sushi, is a multi-track live plugin host that can be configured via JSON files. It supports VST2, VST3, and LV2 formats, allowing a developer to package their DaisySP code as a standard plugin and host it within a professional-grade environment. Elk also provides a daemon called **Sensei**, which simplifies the handling of GPIO-based sensors and physical widgets through a declarative JSON configuration.

| Feature | Patchbox OS | Elk Audio OS |
| :--- | :--- | :--- |
| **Kernel Type** | PREEMPT_RT Linux | Dual-Kernel (Linux + Xenomai) |
| **Primary Host** | JACK / MODEP | Sushi (Plugin Host) |
| **Target Latency** | 5ms - 10ms | < 1ms |
| **Configuration** | Command line / CLI | JSON / gRPC / OSC |
| **Best For** | Prototyping, Pd, SuperCollider | Pro-audio products, minimal jitter |

### Headless Connectivity: Web Servers and WebSockets
A headless system requires a remote GUI that is both responsive and lightweight. Hosting a web server on the Raspberry Pi Zero 2 W allows any device with a browser to act as a control surface. For real-time parameter manipulation, the standard HTTP request/response model is insufficient due to high overhead. **WebSockets** provide the necessary full-duplex, low-latency communication channel.

#### C++ WebSocket Libraries
Several open-source C++ libraries are suitable for integrating a web server directly into the DSP application.
*   **uWebSockets:** One of the most efficient WebSocket implementations available, meticulously optimized for speed and memory footprint. It supports millions of concurrent connections on high-end hardware and is highly performant on embedded devices.
*   **WebSocket++:** A header-only library that is highly portable and flexible. It can utilize either the Boost.Asio library or the C++11 standard library for networking. It is frequently used for adding real-time web GUIs to industrial and telecommunications systems.
*   **IXWebSocket:** A simpler, minimal-dependency library (no Boost required) that is very easy to integrate into existing C++ projects.

#### Parameter Synchronization Strategy
The web server thread must communicate with the audio thread without introducing blocking or locks. A common pattern involves a JSON-based messaging system over WebSockets to send parameter changes from the browser to the Pi.

```json
{
 "module": "subtractive_1",
 "parameter": "cutoff",
 "value": 0.75
}
```

Once received by the web server thread, these values are placed into a **lock-free queue**, which the audio thread polls at the start of each block. This ensures that the DSP state is updated exactly once per audio cycle, maintaining phase coherence and preventing parameter "zipper" noise if appropriate smoothing (interpolating) filters are applied.

### Wireless Control: Bluetooth Low Energy MIDI
Bluetooth Low Energy (BLE) offers a compelling alternative to Wi-Fi for low-latency control, particularly for battery-powered peripheral modules. The **BLE MIDI** specification allows MIDI messages to be sent over a standard GATT (Generic Attribute Profile) connection.

On the Raspberry Pi, the **BlueZ** stack handles Bluetooth communication. Tools like `bluetoothctl` are used to pair and trust devices. For C++ development, libraries such as `gattlib` provide an API for interacting with BLE devices without the complexity of direct D-Bus programming.

A notable challenge with BLE on Linux is the "MIC (Message Integrity Check) failure," which can cause intermittent connection drops, especially when communicating with certain Windows or iOS clients. Successful implementation requires careful handling of connection parameters (intervals, latency, timeout) to ensure a stable stream of MIDI data.

### Modular Hardware Ecosystem: Bespoke Controllers and Modules
The vision of "bespoke hardware modules" for different functions (percussion, granular, etc.) requires a robust and scalable communication bus. Given the constraints of the Raspberry Pi Zero 2 W, I2C and CAN bus are the primary candidates for an inter-module communication backbone.

#### I2C for Modular Interconnectivity
**I2C (Inter-Integrated Circuit)** is the most widespread protocol for connecting sensors and microcontrollers over short distances. Its simple two-wire design (SDA, SCL) and built-in 7-bit addressing allow for up to 127 devices to reside on a single bus.

In a modular synthesis context:
*   **Address Management:** Each hardware module (e.g., a "subtractive synth" module with 8 encoders) would have a unique I2C address.
*   **I/O Expansion:** The **MCP23017** is a standard 16-bit I/O expander used in projects like Zynthian to read multiple rotary encoders and switches through a single I2C connection.
*   **Interrupt Handling:** To avoid constant polling from the Pi, a dedicated "Interrupt" line can be used. When a module detects a knob turn, it pulls the interrupt line low, signaling the Pi to perform an I2C read only when data is actually available.

#### SPI for High-Speed Visuals
For modules that include graphical feedback (e.g., small OLED or TFT displays), **SPI (Serial Peripheral Interface)** is preferred due to its significantly higher throughput—up to 50MHz compared to the 400kHz or 1MHz typical of I2C. The Raspberry Pi supports up to two SPI devices natively, but more can be added using GPIO-based chip select lines.

#### CAN Bus for Robust Modular Systems
For more ambitious modular setups, the **CAN (Controller Area Network)** bus offers superior reliability and error checking. Research into modular instrument communication suggest that 1Mbps CAN can effectively transport Universal MIDI Packets (UMP) across up to 16 modules with minimal latency and jitter. This is particularly useful for "exotic" instruments that may be physically distant or subject to electromagnetic interference.

### Exotic Instruments and Gestural Sensing
The project's goal to include "exotic" instruments that depart from traditional encoders and buttons necessitates a deep dive into advanced gestural sensors.

#### Time-of-Flight (ToF) Distance Sensing
Sensors like the **VL53L0X** use laser Time-of-Flight technology to measure absolute distance to a target up to 2 meters away with millimeter precision. In a musical context, this allows for:
*   **Spatial Modulation:** Using hand proximity to control filter resonance or granular density without physical contact.
*   **3D Gesturing:** Combining multiple ToF sensors to track hand position in a defined volume.

#### Inertial Measurement Units (IMUs) and Sensor Fusion
IMUs like the **BNO055** or **LSM6DS33** provide 9-DOF (Degrees of Freedom) data, including acceleration, gyroscopic rotation, and magnetic orientation. Sensor fusion algorithms, which combine these raw inputs into stable pitch, roll, and yaw angles, are essential for "tilt-based" or "shake-based" synthesis control.

The **Sygaldry** framework is a notable resource here. It is a C++20 library designed for building replicable DMIs (Digital Musical Instruments) like the T-Stick. Sygaldry uses declarative firmwares, where an instrument is defined simply by listing its components (IMU, capacitive touch strips, pressure sensors). The framework automatically handles protocol bindings, such as generating the necessary code to send sensor data over OSC (Open Sound Control) or libmapper.

#### Capacitive Touch and Continuous Control
Capacitive sensors (e.g., the **Trill** series from Bela or the MPR121) provide high-resolution continuous touch data. Unlike standard buttons, these sensors can detect position and pressure, enabling "MPE-like" (MIDI Polyphonic Expression) capabilities where each finger can independently modulate the pitch, timbre, and amplitude of a note.

| Sensor Type | Specific Model | Communication | Use Case in Modular Synth |
| :--- | :--- | :--- | :--- |
| **ToF Distance** | VL53L0X | I2C | Non-contact proximity control (Theremin-style). |
| **IMU (9-DOF)** | BNO055 | I2C / UART | Tilt and motion tracking for "shakers" or handhelds. |
| **Capacitive Touch** | Trill / MPR121 | I2C | Continuous touch strips, pressure-sensitive pads. |
| **Flex Sensor** | FlexiForce | ADC | Bend-sensing for wearable or soft instruments. |
| **3D Magnetometer** | MLX90393 | I2C | Detection of magnet position for "magnetic" faders. |

### Software Architecture: Thread Safety and Lock-Free Patterns
A fundamental challenge in this multi-threaded system is ensuring that the GUI, the network, and the audio threads do not interfere with one another. Standard synchronization primitives like **mutexes are generally prohibited in the audio thread** because they can cause priority inversion. If the audio thread attempts to lock a mutex that is currently held by a low-priority web server thread, the audio callback will be delayed, resulting in a glitch.

#### Lock-Free Queues
The primary mechanism for inter-thread communication is the lock-free Single-Producer Single-Consumer (SPSC) queue.
*   **ReaderWriterQueue:** A widely used C++ implementation by Cameron Desrochers (moodycamel) that provides a wait-free guarantee for a two-thread use case. It allocates memory in contiguous blocks and can grow dynamically if needed (though dynamic growth should happen on the non-real-time thread).
*   **Ring Buffer:** A circular buffer using atomic read and write indices. The producer increments the write index, and the consumer increments the read index.

#### Wait-Free Parameter Updates
For exotic instruments that send a high volume of sensor data (e.g., 100Hz IMU updates), the audio thread should not process every single update if they arrive faster than the audio block rate. A "latest-value" buffer—where the GUI thread overwrites the previous value and the audio thread simply reads the most recent one—is often more appropriate than a queue for continuous parameters.

### Comparative Analysis of Similar Embedded Projects
Analyzing established projects provides a roadmap for successful implementation.
*   **Monome Norns:** Uses OSC for all communication between the control layer (Lua) and audio engines (SuperCollider/C++).
*   **Zynthian:** Demonstrates effective use of a web-based configuration tool and a C-based encoder library ("zyncoder").
*   **Bela:** A gold standard for headless instrument development, providing real-time code editing and oscilloscope visualization within the browser.

### Synthesis: Designing the Unified Architecture
The proposed system can be synthesized into a hierarchical architecture that ensures real-time stability while providing flexible connectivity.

**Layer 1: The Real-Time Audio Core**
This layer contains the C++ application linked with DaisySP. It should run on a dedicated CPU core to minimize context switching.
*   Backend: ALSA (Direct MMAP) or Sushi (Elk).
*   Processing: DaisySP objects processed in a block-based loop.
*   Input: Lock-free queue for parameter updates.

**Layer 2: The Control Orchestrator**
This layer manages the logic of the instrument—mapping sensor inputs to synthesis parameters.
*   Mapping: "Semantic" layer converting raw sensor data to musical parameters.
*   Framework: Integration with Sygaldry for exotic hardware modules.

**Layer 3: The Communication Gateway**
This layer handles the non-real-time external interfaces.
*   Web Server: uWebSockets serving a GUI.
*   Wireless: BlueZ-based BLE MIDI handler.
*   I2C Bus: Dedicated thread for modular hardware polling.

### Implementation Roadmap and Optimization
*   **Power Management:** Use a stable 5V/2.5A power supply and consider active cooling for the Pi Zero 2 W.
*   **Memory Constraints:** Pre-allocate audio buffers and consider fixed-point optimizations if needed.
*   **Dynamic Module Discovery:** Implement an I2C scanning system to identify connected modules at startup.

### Future Outlook
The development of a headless DSP system on the Raspberry Pi Zero 2 W contributes to the **Internet of Musical Things (IoMusT)**. As network technology evolves, distributed DSP becomes feasible, allowing for collaborative musical experiences and scalable digital orchestras.
