#pragma once

#include <algorithm>
#include <math.h>
#include <stdint.h>

// --- Korg Logue SDK Constants ---
#define k_samplerate (48000)
#define k_samplerate_recipf (1.0f / 48000.0f)
#define k_note_mod_fscale (1.f / 127.f)

// --- Korg Logue SDK Macros ---
#define clip1m1f(x) std::clamp(x, -1.0f, 1.0f)
#define si_floorf(x) floorf(x)

#define faster_pow2f(x) powf(2.0f, x)

// --- Korg Logue SDK Functions ---

// Fast Sawtooth Approximation (Naive or PolyBLEP)
static inline float osc_sawf(float x) {
  // Naive Sawtooth: 2.0 * x - 1.0 (assuming x is 0..1)
  // Map 0..1 to -1..1
  return 2.0f * x - 1.0f;
}

// Convert MIDI Note to Frequency (w0 = radians per sample)
static inline float osc_w0f_for_note(uint8_t note, uint8_t mod) {
  float f = 440.0f * powf(2.0f, (float)(note - 69 + mod * 0.01f) / 12.0f);
  return f * k_samplerate_recipf;
}

// Parameter Helpers
static inline float param_10bit_to_f32(int32_t val) {
  return (float)val / 1023.0f;
}

static inline int32_t param_f32_to_10bit(float val) {
  return (int32_t)(val * 1023.0f);
}

// Memory Attributes (Empty for PC)
#define __fast_inline inline
#define fast_inline inline
#define __unit_callback
