#ifndef BRAIDS_WRAPPER_H
#define BRAIDS_WRAPPER_H

#include <cstdint>
#include <cstring>
#include <vector>

// Forward declare to avoid polluting namespace
namespace braids {
class MacroOscillator;
}

class BraidsWrapper {
public:
  void Init(float sample_rate);
  float Process();

  // Parameters
  void SetFreq(float freq);
  void SetTimbre(float value); // 0.0 - 1.0
  void SetColor(float value);  // 0.0 - 1.0
  void SetModel(int model_index);

  // Extra knobs for UI
  void SetFine(float value);       // -1.0 to 1.0 (semitone)
  void SetCoarse(float value);     // -24 to +24 (semitones)
  void SetFM(float value);         // 0.0 - 1.0 (LFO -> Pitch amount)
  void SetModulation(float value); // 0.0 - 1.0 (LFO -> Timbre amount)

  // Helper
  const char *GetModelName(int index);

private:
  braids::MacroOscillator *osc_;
  float sample_rate_;

  // Parameter State
  int16_t timbre_;
  int16_t color_;

  // UI Parameter State
  float fine_;
  float coarse_;
  float fm_amount_;
  float mod_amount_;
  float base_freq_;

  // Simple internal LFO for modulation
  float lfo_phase_;

  // Internal buffer for block processing (Braids works in blocks)
  static const size_t kBlockSize = 24;
  int16_t buffer_[kBlockSize];
  size_t buffer_index_;

  uint8_t sync_buffer_[kBlockSize];
};

#endif // BRAIDS_WRAPPER_H
