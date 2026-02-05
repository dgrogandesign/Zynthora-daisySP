# Zynthora - To-Do List

## Phase 1: The Event Backbone (High Priority)
- [ ] **Event Struct:** Define `SynthEvent` in C++ header.
- [ ] **Lock-Free Queue:** Implement `ReaderWriterQueue` or `RingBuffer` to safely pass events from Main Thread -> Audio Thread.
- [ ] **Event Dispatcher:** Create the logic in the Audio Callback to parse events (`switch(param_id)`) and update DSP variables.
- [ ] **Recorder:** Create a simple file writer to log events to disk.

## Phase 2: Core DSP Engine
- [ ] **FM Synthesis:** Implement `daisysp::Fm2` (2-Op FM) as a selectable engine.
- [ ] **Wavetable Synthesis:** Explore `daisysp::BlOsc` (Band-limited Wavetable).
- [ ] **Effects:** Refine Reverb and Delay parameters for better "performance" feel.

## Phase 3: Connectivity (Pi Zero 2 W)
- [ ] **BLE Stack:** Implement `BlueZ` / `gattlib` C++ wrapper to receive MIDI/Events over Bluetooth.
- [ ] **Network Stability:** Solve `usb0` gadget mode issues for reliable laptop tethering.
- [ ] **Performance Tuning:** Profile CPU usage with the new Event System.

## Phase 4: UI / Control
- [ ] **Multi-Client Web:** Ensure multiple phone browsers can connect simultaneously and see the same state.
- [ ] **Session Replay:** Build a simple player to read back `.zynth` files.
