#ifndef ZYNTHORA_EVENTS_H
#define ZYNTHORA_EVENTS_H

#include <cstdint>

// ============================================================================
// 1. EVENT PROTOCOL (The Common Language)
// ============================================================================

// Who sent the event? (Source ID)
enum SourceID : uint8_t {
    SRC_SYSTEM        = 0x00, // Internal System (Clock, Init)
    SRC_GUI_MAIN      = 0x10, // The Main Phone/Tablet Interface
    SRC_GUI_CLIENT_B  = 0x11, // Second Phone (Multi-user)
    SRC_MOD_SEQ       = 0x20, // Hardware Sequencer Module
    SRC_MOD_BASS      = 0x30, // Hardware Bassline Module
    SRC_MOD_PADS      = 0x40, // Hardware Pad/Sampler Module
    SRC_MOD_CONDUCTOR = 0x50  // Hardware Mixer/Conductor Module
};

// What type of event is it?
enum EventType : uint8_t {
    TYPE_NOTE_ON      = 0x01, // Note Pressed
    TYPE_NOTE_OFF     = 0x02, // Note Released
    TYPE_PARAM_CHANGE = 0x03, // Knob/Slider Moved
    TYPE_PAGE_SWITCH  = 0x04, // "Focus" this module on the GUI
    TYPE_TRANSPORT    = 0x05, // Play/Stop/Rewind
    TYPE_LOCK_PARAM   = 0x06  // Multi-user lock (fight mechanism)
};

// Global Parameter List (Param ID)
// This maps every knob in the ecosystem to a unique number.
enum ParamID : uint16_t {
    // --- SYNTH 1 (BASS) ---
    // Oscillators
    P_SYN1_OSC_WAVE   = 100, // 0=Saw, 1=Sqr, 2=Tri, 3=Sine
    P_SYN1_OSC_FREQ   = 101, // Frequency (Hz)
    P_SYN1_OSC_DETUNE = 102, // Detune amount
    
    // Filter
    P_SYN1_FLT_CUTOFF = 110, // 0.0 - 1.0 (Mapped to Hz)
    P_SYN1_FLT_RES    = 111, // Resonance 0.0 - 1.0
    P_SYN1_FLT_ENV    = 112, // Envelope Depth
    
    // Envelope
    P_SYN1_AMP_ATTACK = 120,
    P_SYN1_AMP_DECAY  = 121,
    
    // Effects
    P_SYN1_DRIVE      = 130, // Overdrive amount

    // --- GLOBAL MIXER (CONDUCTOR) ---
    P_MIX_VOL_SYN1    = 900,
    P_MIX_VOL_SYN2    = 901,
    P_MIX_MUTE_SYN1   = 910,
    P_MIX_REV_SEND    = 920, // Global Reverb Send
    P_MIX_DLY_SEND    = 921  // Global Delay Send
};

// ============================================================================
// 2. THE EVENT STRUCTURE
// ============================================================================
// This struct is packed to be sent over BLE / Queues easily.
// Size: 8 + 1 + 1 + 2 + 4 = 16 bytes (very efficient)

struct SynthEvent {
    uint64_t timestamp;  // When did it happen? (nanoseconds)
    uint8_t  source_id;  // Who sent it?
    uint8_t  type;       // What kind of event? (Note vs Param)
    uint16_t id;         // Which Param or Note Number?
    float    value;      // The value (Velocity, 0.0-1.0, etc)
};

#endif // ZYNTHORA_EVENTS_H
