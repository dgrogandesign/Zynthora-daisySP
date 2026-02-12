#include "plaits_wrapper.h"
#include <algorithm> // for std::fill
#include <cmath>     // for std::sin, M_PI
#include <cstring>

#define PLAITS_BUFFER_SIZE 32768

PlaitsWrapper::PlaitsWrapper()
    : buffer_index_(kBlockSize), sample_rate_(48000.0f), base_freq_(110.0f),
      harmonics_(0.5f), timbre_(0.5f), morph_(0.5f), coarse_(0.0f), fine_(0.0f),
      fm_amount_(0.0f), mod_amount_(0.0f), decay_(0.5f),
      model_(0), // First model
      lfo_phase_(0.0f), gate_(false), trigger_reset_counter_(0) {
  shared_buffer_ = new char[PLAITS_BUFFER_SIZE];

  // Default Patch
  patch_.note = 60.0f; // Middle C
  patch_.harmonics = 0.5f;
  patch_.timbre = 0.5f;
  patch_.morph = 0.5f;
  patch_.frequency_modulation_amount = 0.0f;
  patch_.timbre_modulation_amount = 0.0f;
  patch_.morph_modulation_amount = 0.0f;
  patch_.engine = 0;
  patch_.decay = 0.5f;
  patch_.lpg_colour = 0.5f;

  // Default Modulations
  modulations_.engine = 0.0f;
  modulations_.note = 0.0f;
  modulations_.frequency = 0.0f;
  modulations_.harmonics = 0.0f;
  modulations_.timbre = 0.0f;
  modulations_.morph = 0.0f;
  modulations_.trigger = 0.0f;
  modulations_.level = 1.0f; // Boost level for FM/VA models
  modulations_.frequency_patched = false;
  modulations_.trigger_patched = true; // Use internal trigger/envelope logic
  modulations_.level_patched = true;   // Use explicit level control

  // Set default attenuverters to 1.0 so external modulation works
  patch_.frequency_modulation_amount = 0.05f; // Small amount for Vibrato
  patch_.timbre_modulation_amount = 0.5f;     // Half depth for Mod slider
  patch_.morph_modulation_amount = 0.5f;
}

PlaitsWrapper::~PlaitsWrapper() { delete[] shared_buffer_; }

void PlaitsWrapper::Init(float sample_rate) {
  sample_rate_ = sample_rate;

  stmlib::BufferAllocator allocator(shared_buffer_, PLAITS_BUFFER_SIZE);
  voice_.Init(&allocator);

  std::fill(output_frames_, output_frames_ + kBlockSize,
            plaits::Voice::Frame{0, 0});
}

void PlaitsWrapper::SetFrequency(float freq) {
  if (freq > 0.0f) {
    base_freq_ = freq;
  }
}

void PlaitsWrapper::SetModel(int model) {
  if (model != model_) {
    model_ = model;
    // engine is int in Patch struct? Yes.
    // plaits/dsp/engine/engine.h defines many, no enum for ID in patch struct,
    // just int index 0-15? We will assume 0-15 mapping fits standard banks.
    patch_.engine = model;
  }
}

void PlaitsWrapper::SetHarmonics(float value) { harmonics_ = value; }
void PlaitsWrapper::SetTimbre(float value) { timbre_ = value; }
void PlaitsWrapper::SetMorph(float value) { morph_ = value; }
void PlaitsWrapper::SetCoarse(float semitones) { coarse_ = semitones; }
void PlaitsWrapper::SetFine(float semitones) { fine_ = semitones; }
void PlaitsWrapper::SetFMAmount(float amount) { fm_amount_ = amount; }
void PlaitsWrapper::SetModAmount(float amount) { mod_amount_ = amount; }
void PlaitsWrapper::SetDecay(float value) {
  decay_ = value;
  patch_.decay = value;
}

void PlaitsWrapper::SetGate(bool state) {
  if (state && gate_) {
    trigger_reset_counter_ = 8; // Force low for ~8 blocks (>> kTriggerDelay=5)
  }
  gate_ = state;
}

float PlaitsWrapper::Process() {
  if (buffer_index_ >= kBlockSize) {
    // Handle Trigger/Gate
    if (trigger_reset_counter_ > 0) {
      modulations_.trigger = 0.0f;
      trigger_reset_counter_--;
    } else {
      modulations_.trigger = gate_ ? 1.0f : 0.0f;
    }

    // Update LFO (6Hz)
    float lfo_inc = 6.0f / sample_rate_ * kBlockSize;
    lfo_phase_ += lfo_inc;
    if (lfo_phase_ > 1.0f)
      lfo_phase_ -= 1.0f;

    float lfo_val = std::sin(lfo_phase_ * 2.0f * M_PI); // -1 to 1

    // Calc Note from Frequency
    float note = 69.0f + 12.0f * std::log2(base_freq_ / 440.0f);

    // Apply Coarse and Fine
    patch_.note = note + coarse_ + fine_;

    // Update Patch parameters
    patch_.harmonics = harmonics_;
    patch_.timbre = timbre_;
    patch_.morph = morph_;

    // Apply Modulation
    // FM (Pitch LFO)
    // FM (Pitch LFO)
    if (fm_amount_ > 0.0f) {
      // Apply Vibrato via Note modulation
      modulations_.note = lfo_val * fm_amount_ * 1.0f;
      modulations_.frequency = 0.0f;
      modulations_.frequency_patched = false;
    } else {
      modulations_.note = 0.0f;
      modulations_.frequency = 0.0f;
      modulations_.frequency_patched = false;
    }

    // Timbre LFO
    if (mod_amount_ > 0.0f) {
      // We send full LFO range, scaled by Slider (mod_amount_)
      modulations_.timbre = lfo_val * mod_amount_;
      modulations_.timbre_patched = true;

      // Also modulate Morph for "richer" mod feel?
      modulations_.morph = lfo_val * mod_amount_;
      modulations_.morph_patched = true;
    } else {
      modulations_.timbre = 0.0f;
      modulations_.timbre_patched = false;
      modulations_.morph = 0.0f;
      modulations_.morph_patched = false;
    }

    // Render
    voice_.Render(patch_, modulations_, output_frames_, kBlockSize);
    buffer_index_ = 0;
  }

  // Output is int16_t, convert to float
  // output_frames[i].out (Main) and .aux (Aux)
  // We'll mix them or just take main.
  // Main provides the primary sound.
  float out = static_cast<float>(output_frames_[buffer_index_].out) / 32768.0f;
  buffer_index_++;
  return out;
}
