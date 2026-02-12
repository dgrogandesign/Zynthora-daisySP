#include "AirwindowsWrapper.h"
#include <cmath>
#include <iostream>
#include <ostream>

AirwindowsWrapper::AirwindowsWrapper() {
  galactic_ = new airwindows::Galactic(nullptr);
  console_ = new airwindows::Console7Channel(nullptr);
  mackity_ = new airwindows::Mackity(nullptr);
  pressure_ = new airwindows::Pressure5(nullptr);
  tope_ = new airwindows::Tope(nullptr);

  block_size_ = 4096; // Default max
  temp_in_l_ = new float[block_size_];
  temp_in_r_ = new float[block_size_];
  temp_out_l_ = new float[block_size_];
  temp_out_r_ = new float[block_size_];

  inputs_[0] = temp_in_l_;
  inputs_[1] = temp_in_r_;
  outputs_[0] = temp_out_l_;
  outputs_[1] = temp_out_r_;

  // Default Settings
  SetGalacticMix(0.4f); // 40% Wet
  SetGalacticSize(0.5f);
  SetGalacticDetune(0.5f);
  SetGalacticBrightness(0.5f);

  SetConsoleDrive(0.5f); // Unity gain-ish

  SetMackityDrive(0.8f);      // High drive for visibility
  SetPressureThreshold(0.5f); // Medium compression
  SetTopeDrive(0.0f);         // Off by default
  SetTopeFuzz(0.0f);
}

AirwindowsWrapper::~AirwindowsWrapper() {
  delete galactic_;
  delete console_;
  delete mackity_;
  delete pressure_;
  delete tope_;

  delete[] temp_in_l_;
  delete[] temp_in_r_;
  delete[] temp_out_l_;
  delete[] temp_out_r_;
}

void AirwindowsWrapper::Init(float sample_rate) {
  if (galactic_)
    galactic_->setSampleRate(sample_rate);
  if (console_)
    console_->setSampleRate(sample_rate);
  if (mackity_)
    mackity_->setSampleRate(sample_rate);
  if (pressure_)
    pressure_->setSampleRate(sample_rate);
  if (tope_)
    tope_->setSampleRate(sample_rate);
}

void AirwindowsWrapper::SetEnabled(bool enabled) {
  enabled_ = enabled;
  std::cout << "Airwindows Enabled: " << (enabled ? "ON" : "OFF") << std::endl;
}

void AirwindowsWrapper::Process(float *left, float *right, size_t size) {
  static int debug_ctr = 0;
  if (debug_ctr++ % 100 == 0) {
    // std::cout << "Airwindows Process: Size=" << size << " Enabled=" <<
    // enabled_ << std::endl;
  }

  if (size > block_size_ || !enabled_)
    return; // Pass-through

  // 1. Copy Input to Temp Buffers
  for (size_t i = 0; i < size; i++) {
    temp_in_l_[i] = left[i];
    temp_in_r_[i] = right[i];
  }

  // 2. Process Mackity (Distortion/Drive)
  mackity_->processReplacing(inputs_, outputs_, size);

  // Copy back for next stage
  for (size_t i = 0; i < size; i++) {
    temp_in_l_[i] = temp_out_l_[i];
    temp_in_r_[i] = temp_out_r_[i];
  }

  // 3. Process Pressure5 (Compression)
  pressure_->processReplacing(inputs_, outputs_, size);

  // Copy back for next stage
  for (size_t i = 0; i < size; i++) {
    temp_in_l_[i] = temp_out_l_[i];
    temp_in_r_[i] = temp_out_r_[i];
  }

  // 4. Process Tope (Tape)
  // tope_->processReplacing(inputs_, outputs_, size);

  // Copy back for next stage
  for (size_t i = 0; i < size; i++) {
    temp_in_l_[i] = temp_out_l_[i];
    temp_in_r_[i] = temp_out_r_[i];
  }

  // 5. Process Console7 (Channel Color)
  console_->processReplacing(inputs_, outputs_, size);

  // Copy back for next stage
  for (size_t i = 0; i < size; i++) {
    temp_in_l_[i] = temp_out_l_[i];
    temp_in_r_[i] = temp_out_r_[i];
  }

  // 5. Process Galactic (Reverb)
  galactic_->processReplacing(inputs_, outputs_, size);

  // 6. Copy Output back to main buffers
  for (size_t i = 0; i < size; i++) {
    left[i] = temp_out_l_[i];
    right[i] = temp_out_r_[i];
  }
} // End Process

void AirwindowsWrapper::SetGalacticMix(float value) {
  galactic_->setParameter(airwindows::Galactic::kParamE, value);
}
void AirwindowsWrapper::SetGalacticSize(float value) {
  galactic_->setParameter(airwindows::Galactic::kParamD, value);
}
void AirwindowsWrapper::SetGalacticDetune(float value) {
  galactic_->setParameter(airwindows::Galactic::kParamC, value);
}
void AirwindowsWrapper::SetGalacticBrightness(float value) {
  galactic_->setParameter(airwindows::Galactic::kParamB, value);
}
void AirwindowsWrapper::SetConsoleDrive(float value) {
  console_->setParameter(airwindows::Console7Channel::kParamA, value);
}
void AirwindowsWrapper::SetMackityDrive(float value) {
  mackity_->setParameter(airwindows::Mackity::kParamA, value);
  std::cout << "Mackity Drive: " << value << std::endl;
}
void AirwindowsWrapper::SetPressureThreshold(float value) {
  pressure_->setParameter(airwindows::Pressure5::kParamA, value);
  std::cout << "Pressure Thresh: " << value << std::endl;
}
void AirwindowsWrapper::SetTopeDrive(float value) {
  tope_->setParameter(airwindows::Tope::kParamA, value);
}
void AirwindowsWrapper::SetTopeFuzz(float value) {
  tope_->setParameter(airwindows::Tope::kParamB, value);
}
