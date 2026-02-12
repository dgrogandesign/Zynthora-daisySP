#ifndef PLAITS_WRAPPER_H_
#define PLAITS_WRAPPER_H_

#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"

class PlaitsWrapper {
public:
  PlaitsWrapper();
  ~PlaitsWrapper();

  void Init(float sample_rate);
  float Process();

  // Parameters
  void SetFrequency(float freq); // Set base frequency
  void SetModel(int model);
  void SetHarmonics(float value); // 0.0 to 1.0
  void SetTimbre(float value);    // 0.0 to 1.0
  void SetMorph(float value);     // 0.0 to 1.0

  // Tuning
  void SetCoarse(float semitones); // +/- 24
  void SetFine(float semitones);   // +/- 1

  // Modulation
  void SetFMAmount(float amount);  // 0.0 to 1.0
  void SetModAmount(float amount); // 0.0 to 1.0 (Timbre/Morph Mod)

  // Trigger / Gate (for internal envelope)
  void SetGate(bool state);
  void SetSustain(bool sustain);
  void SetDecay(float value); // 0.0 to 1.0

private:
  plaits::Voice voice_;
  plaits::Patch patch_;
  plaits::Modulations modulations_;

  // Audio Buffers
  static const size_t kBlockSize = 24;
  plaits::Voice::Frame output_frames_[kBlockSize];
  size_t buffer_index_;

  float sample_rate_;

  // Parameter Cache
  float base_freq_;
  float harmonics_;
  float timbre_;
  float morph_;
  float coarse_;
  float fine_;
  float fm_amount_;
  float mod_amount_;
  float decay_;

  int model_;

  // Internal LFO state
  float lfo_phase_;
  bool gate_;
  int trigger_reset_counter_;

  // Memory for Plaits
  char *shared_buffer_;
};

#endif // PLAITS_WRAPPER_H_
