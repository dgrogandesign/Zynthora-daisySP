#ifndef AIRWINDOWS_WRAPPER_H
#define AIRWINDOWS_WRAPPER_H

#include "Console7Channel.h"
#include "Galactic.h"
#include "Mackity.h"
#include "Pressure5.h"
#include "Tope.h"
#include "audioeffectx.h"

class AirwindowsWrapper {
public:
  AirwindowsWrapper();
  ~AirwindowsWrapper();

  void Init(float sample_rate);
  void Process(float *left, float *right, size_t size);

  // Parameters
  void SetGalacticMix(float value); // Mix (0.0 - 1.0)
  void SetGalacticSize(float value);
  void SetGalacticDetune(float value);
  void SetGalacticBrightness(float value);

  void SetConsoleDrive(float value);
  void SetMackityDrive(float value);
  void SetPressureThreshold(float value);
  void SetTopeDrive(float value);
  void SetTopeFuzz(float value);
  void SetEnabled(bool enabled);
  bool IsEnabled() const { return enabled_; }

private:
  airwindows::Galactic *galactic_;
  airwindows::Console7Channel *console_;
  airwindows::Mackity *mackity_;
  airwindows::Pressure5 *pressure_;
  airwindows::Tope *tope_;

  float *temp_in_l_;
  float *temp_in_r_;
  float *temp_out_l_;
  float *temp_out_r_;

  // Pointers for processReplacing (expects float**)
  float *inputs_[2];
  float *outputs_[2];

  size_t block_size_;
  bool enabled_ = true;
};

#endif
