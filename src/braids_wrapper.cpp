#include "braids_wrapper.h"
#include "braids/macro_oscillator.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// --- Helper Functions for converting Float Params to Braids Internal Format
// ---

// Braids uses 0-32767 for most params
static inline int16_t FloatToU15(float f) {
  if (f < 0.0f)
    f = 0.0f;
  if (f > 1.0f)
    f = 1.0f;
  return (int16_t)(f * 32767.0f);
}

// Braids Pitch is (60 << 7) for C4.
// 128 units per semitone.
static inline int32_t FreqToPitch(float freq) {
  if (freq < 1.0f)
    freq = 1.0f;
  float midi = 69.0f + 12.0f * std::log2(freq / 440.0f);
  return (int32_t)(midi * 128.0f);
}

void BraidsWrapper::Init(float sample_rate) {
  sample_rate_ = sample_rate;
  osc_ = new braids::MacroOscillator();
  osc_->Init();

  // Default config
  osc_->set_shape(braids::MACRO_OSC_SHAPE_CSAW);
  timbre_ = 0;
  color_ = 0;

  // UI Defaults
  fine_ = 0.0f;
  coarse_ = 0.0f;
  fm_amount_ = 0.0f;
  mod_amount_ = 0.0f;
  base_freq_ = 440.0f;
  lfo_phase_ = 0.0f;

  osc_->set_parameters(timbre_, color_); // Timbre, Color
  osc_->set_pitch(60 << 7);

  // Clear Buffers
  std::fill(std::begin(buffer_), std::end(buffer_), 0);
  std::fill(std::begin(sync_buffer_), std::end(sync_buffer_), 0);

  buffer_index_ = kBlockSize; // Force render on first call
}

float BraidsWrapper::Process() {
  if (buffer_index_ >= kBlockSize) {
    // Update LFO (approx 6Hz)
    float lfo_inc = 6.0f / sample_rate_ * kBlockSize;
    lfo_phase_ += lfo_inc;
    if (lfo_phase_ > 1.0f)
      lfo_phase_ -= 1.0f;

    float lfo_val = std::sin(lfo_phase_ * 2.0f * M_PI); // -1 to 1

    // Apply Modulation to Timbre
    if (mod_amount_ > 0.0f) {
      int16_t mod_timbre = timbre_ + (int16_t)(lfo_val * mod_amount_ *
                                               16384.0f); // +/- half range
      if (mod_timbre < 0)
        mod_timbre = 0;
      if (mod_timbre > 32767)
        mod_timbre = 32767;
      osc_->set_parameters(mod_timbre, color_);
    } else {
      osc_->set_parameters(timbre_, color_);
    }

    // Apply FM and Tuning to Pitch
    // Base Pitch from Freq
    int32_t pitch = FreqToPitch(base_freq_);

    // Coarse (semitones) + Fine (semitones)
    pitch += (int32_t)((coarse_ + fine_) * 128.0f); // 128 units per semitone

    // FM (Pitch Mod)
    if (fm_amount_ > 0.0f) {
      // FM range: +/- 1 octave? (12 * 128 = 1536)
      pitch += (int32_t)(lfo_val * fm_amount_ * 1536.0f);
    }

    osc_->set_pitch(pitch);

    // Render block
    osc_->Render(sync_buffer_, buffer_, kBlockSize);
    buffer_index_ = 0;
  }

  // Output is 16-bit signed.
  // Convert to float range [-1.0, 1.0]
  return (float)buffer_[buffer_index_++] / 32768.0f;
}

void BraidsWrapper::SetFreq(float freq) { base_freq_ = freq; }

void BraidsWrapper::SetTimbre(float value) { timbre_ = FloatToU15(value); }

void BraidsWrapper::SetColor(float value) { color_ = FloatToU15(value); }

void BraidsWrapper::SetFine(float value) { fine_ = value; }

void BraidsWrapper::SetCoarse(float value) { coarse_ = value; }

void BraidsWrapper::SetFM(float value) { fm_amount_ = value; }

void BraidsWrapper::SetModulation(float value) { mod_amount_ = value; }

void BraidsWrapper::SetModel(int model_index) {
  osc_->set_shape((braids::MacroOscillatorShape)model_index);
}

const char *BraidsWrapper::GetModelName(int index) {
  switch (index) {
  case 0:
    return "CSAW";
  case 1:
    return "MORPH";
  // ... (can expand later, just placeholders for now)
  default:
    return "UNKNOWN";
  }
}
