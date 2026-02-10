#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: para_saw.h
 *
 *  Simple paraphonic saw oscillator.
 *  Please turn off Legato in the global settings.
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "unit_osc.h"   // Note: Include base definitions for osc units
#include "osc_api.h"

#include "utils/int_math.h"   // for clipminmaxi32()

class Osc {
public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    DETUNE = 0U,
    OCT,
    NUM_PARAMS
  };

  enum {
    VOICE_NUM = 4U,
    VOICE_IDX_LAST = (VOICE_NUM-1),
    VOICE_IDX_UNDEF = 0xFFU,
    VOICE_NOTE_UNDEF = 0xFFU,    
  };

  typedef enum {
    voice_state_off,
    voice_state_on,
  } voice_state_t;

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params {
    float detune{0.f};
    int8_t oct{0};

    void reset() {
      detune = 0.f;
      oct = 0;
    }
  };

  struct voice_t {
    uint8_t note;
    voice_state_t state;
    uint8_t idx;
    uint8_t velo;
  };

  enum {
    k_flags_none = 0,
    k_flag_reset = 1 << 0,
  };

  struct State
  {
    float phase_arr[VOICE_NUM]{0.f, 0.f, 0.f, 0.f};
    float w0[VOICE_NUM]{440.f * k_samplerate_recipf,
			440.f * k_samplerate_recipf,
			440.f * k_samplerate_recipf,
			440.f * k_samplerate_recipf}; // phase increment
    std::atomic_uint_fast32_t flags{k_flags_none};

    inline void Reset(void)
    {
      for(uint8_t i=0; i<VOICE_NUM; i++){
	phase_arr[i] = 0.f;
	w0[i] = 440.f * k_samplerate_recipf;
      }      
      flags = k_flags_none;
    }
  };


  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Osc(void) {}
  ~Osc(void) {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;

    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    // Note: NTS-1 mkII oscillators can make use of the audio input depending on the routing options in global settings, see product documentation for details.
    if (desc->input_channels != 2 || desc->output_channels != 1)  // should be stereo input / mono output
      return k_unit_err_geometry;

    // Note: SDRAM is not available from the oscillator runtime environment
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    // Make sure parameters are reset to default values
    params_.reset();
    g_voice_count = 0;

    for(uint8_t i=0; i<VOICE_NUM; i++){
      voice[i].note = VOICE_NOTE_UNDEF;
      voice[i].state = voice_state_off;
      voice[i].idx = VOICE_IDX_UNDEF;
      voice[i].velo = 63;
    }
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: cleanup and release resources if any
  }

  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
    g_voice_count = 0;
    voice_reset();
  }

  inline void Resume() {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
  }

  inline void Suspend() {
    // Note: Effect will enter suspend state. Usually means another effect was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) {

    const float *__restrict in_p = in;
    float *__restrict out_p = out;
    const float *out_e = out_p + frames; // assuming mono output

    const uint8_t flags = state_.flags;
    state_.flags = k_flags_none;

    float w0[VOICE_NUM];     
    float phase_arr[4];
    for (int i = 0; i < VOICE_NUM; i++){
      float mod = si_floorf((params_.detune * detune_coeff_arr[i]) * 127);
      w0[i] = state_.w0[i] = osc_w0f_for_note(voice[i].note + (params_.oct * 12),
					      (uint8_t)mod);
      phase_arr[i] = state_.phase_arr[i];
    }

    for (; out_p != out_e;){

      float phase = 0.f;
      float sig = 0.f;
      for (int i = 0; i < VOICE_NUM; i++){
	if(voice[i].state == voice_state_on && voice[i].note != VOICE_NOTE_UNDEF){
	  phase = phase_arr[i];
	  phase = (phase <= 0) ? 1.f - phase : phase - (uint32_t)phase;
	
	  sig += osc_sawf(phase);
	  sig *= 0.8f;

	  phase_arr[i] += w0[i];
	  phase_arr[i] -= (uint32_t)phase_arr[i];

	} else {
	  phase_arr[i] = 0.f;
	}
      }

      sig = clip1m1f(sig);

      *(out_p++) = sig;
    }

    for (int i = 0; i < VOICE_NUM; i++){
      state_.phase_arr[i] = phase_arr[i];
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {

    case DETUNE:
      value = clipminmaxi32(0, value, 1023);
      params_.detune = param_10bit_to_f32(value);
      break;

    case OCT:
      params_.oct = value;
      break;
      
    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {

    case DETUNE:
      return param_f32_to_10bit(params_.detune);
      break;

    case OCT:
      return params_.oct;
      break;

    default:
      break;
    }

    return INT_MIN; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    // const float bpmf = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
    (void)tempo;
  }

  inline void tempo4ppqnTick(uint32_t counter) {
    (void)counter;
  }

  inline void NoteOn(uint8_t note, uint8_t velo) {

    if(g_voice_count==0){
      voice_reset();
    }

    // Ignore if note number is duplicated.
    for(uint8_t i=0; i<VOICE_NUM; i++){
      if(note == voice[i].note && voice[i].state == voice_state_on)
	return;
    }
    // Exploration of released voices
    for(uint8_t i=0; i<VOICE_NUM; i++){
      if(voice[i].state == voice_state_off){
	voice[i].note = note;
	voice[i].idx = g_voice_count;
	voice[i].state = voice_state_on;
	voice[i].velo = velo;
	g_voice_count++;
	return;
      }
    }
    // Overwrite older voices
    uint8_t minIdx = 0;
    uint8_t tmpIdx = voice[0].idx;
    for(uint8_t i=0; i<VOICE_NUM; i++){
      if(voice[i].state == voice_state_on){
	if(voice[i].idx < tmpIdx)
	  minIdx = i;
      }
    }
    voice[minIdx].note = note;
    voice[minIdx].idx = g_voice_count;
    voice[minIdx].velo = velo;
    //g_voice_count++;
  }


  inline void NoteOff(uint8_t note) {
    for(uint8_t i=0; i<VOICE_NUM; i++){
      if(note == voice[i].note && voice[i].state == voice_state_on){
	voice[i].note = VOICE_NOTE_UNDEF;	
	voice[i].state = voice_state_off;	
	voice[i].idx = 0;
	g_voice_count--;
	return;
      }
    }
    g_voice_count--;
  }


  inline void AllNoteOff() {
    voice_reset();
    g_voice_count = 0;
  }

  inline void PitchBend(uint8_t bend) {
    (uint8_t)bend;
  }

  inline void ChannelPressure(uint8_t press) {
    (uint8_t)press;
  }

  inline void AfterTouch(uint8_t note, uint8_t press) {
    (uint8_t)note;
    (uint8_t)press;
  }

  
  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/
  
private:
  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  std::atomic_uint_fast32_t flags_;

  unit_runtime_desc_t runtime_desc_;

  Params params_;
  State state_;

  uint8_t g_voice_count;
  voice_t voice[VOICE_NUM];

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/
  void voice_reset(void)
  {
    for(uint8_t i=0; i<VOICE_NUM; i++){
      voice[i].note = VOICE_NOTE_UNDEF;
      voice[i].state = voice_state_off;
      voice[i].idx = VOICE_IDX_UNDEF;
      voice[i].velo = 63;
    }    
  }

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/

  const float detune_coeff_arr[4] = {
    0.05f, 0.35f, 0.5f, 0.45f
  };
};
