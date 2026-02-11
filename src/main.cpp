#define MINIAUDIO_IMPLEMENTATION
#include "events.h"
#include "lockfree_queue.h"
#include "miniaudio.h"
#include "mongoose.h"

// DaisySP Headers
#include "Control/adsr.h"
#include "Control/phasor.h"
#include "DaisySP/Source/daisysp.h"
#include "Effects/chorus.h"
#include "Effects/overdrive.h"
#include "Effects/reverbsc.h"
#include "Filters/moogladder.h"
#include "Synthesis/fm2.h"
#include "Utility/delayline.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace daisysp;

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000
#define DELAY_MAX_SAMPLES 48000
#define WAVE_FM 100
#define WAVE_WT 101
#define WAVE_LOGUE 102
#define WAVE_BRAIDS 103
#define WAVE_PLAITS 104
#include "wavetables.h"

// --- Logue SDK Adapter ---
#include "braids_wrapper.h"
#include "para_saw.h" // User Unit
#include "plaits_wrapper.h"
#include "unit_osc.h" // Shim

struct LogueWrapper {
  Osc logueOsc;
  unit_runtime_desc_t desc;
  float paramDetune = 0.0f;
  int paramOct = 0;

  // Internal state
  uint8_t lastNote = 255;

  void Init(float sr) {
    // Setup mock descriptor
    desc.target = 0;
    desc.api = 0;
    desc.samplerate = 48000;
    desc.input_channels = 2;
    desc.output_channels = 1;
    desc.hooks = nullptr;

    logueOsc.Init(&desc);
    logueOsc.Reset();
  }

  void SetFreq(float freq) {
    // Korg oscillators usually track note/pitch internally via NoteOn/Off
    // usage in Process().
    // However, `para_saw` uses `NoteOn` events to set base pitch.
    // We will map our mono-synth logic to this.

    float midiNote = 69.0f + 12.0f * log2f(freq / 440.0f);
    uint8_t noteInt = (uint8_t)(midiNote + 0.5f); // Round to nearest semitone

    // Retrigger if note changed significantly?
    // For now, para_saw might expect NoteOn for pitch.
    // Let's just update the internal state if needed, or rely on Process().
    // *Correction*: para_saw uses `osc_w0f_for_note` in Process loop based on
    // `voice[i].note`. So we MUST call NoteOn to set the pitch.

    if (noteInt != lastNote) {
      std::cout << "LOGUE ON: " << (int)noteInt << std::endl;
      logueOsc.NoteOn(noteInt, 100);
      lastNote = noteInt;
    }
  }

  void NoteOff(uint8_t note) {
    std::cout << "LOGUE OFF: " << (int)note << std::endl;
    logueOsc.NoteOff(note); // Tell Logue engine to release this specific voice
  }

  // Map Detune (0.0 - 1.0) -> Detune (0-1023)
  void SetDetune(float val) {
    int32_t korgVal = (int32_t)(val * 1023.0f);
    logueOsc.setParameter(Osc::DETUNE, korgVal);
  }

  // Map Sub/Oct (-1 to 1) -> Oct (-2 to 2)
  void SetOctave(int val) { logueOsc.setParameter(Osc::OCT, val); }

  float Process() {
    // Logue Process works in blocks. We are processing sample-by-sample here.
    // This is inefficient but compatible.
    float in[2] = {0.0f, 0.0f};
    float out[1] = {0.0f};

    logueOsc.Process(in, out, 1);
    return out[0];
  }

  void ResetVoices() {
    // Clear all voices and reset note tracking
    logueOsc.AllNoteOff();
    lastNote = 255;
  }
};

// ============================================================================
// CUSTOM WAVETABLE OSCILLATOR (No Library Class Available)
// ============================================================================
struct WavetableOsc {
  daisysp::Phasor phs;
  float sampleRate;
  float morphPos = 0.0f;

  void Init(float sr) {
    sampleRate = sr;
    phs.Init(sr);
    phs.SetFreq(440.0f);
  }

  void SetFreq(float freq) { phs.SetFreq(freq); }

  // Morph Position (0.0 to 63.0)
  void SetMorph(float m) {
    if (m < 0.0f)
      m = 0.0f;
    if (m > 63.0f)
      m = 63.0f;
    morphPos = m;
  }

  // 2D Interpolation: Phase X Morph
  float Process() {
    float p = phs.Process(); // 0.0 to 1.0

    // 1. Determine which two tables to blend
    int tableA = (int)morphPos;
    int tableB = tableA + 1;
    if (tableB >= 64)
      tableB = 63; // Clamp top

    float morphFrac = morphPos - tableA;
    if (tableA == 63)
      morphFrac = 0.0f; // Fully at top table

    // 2. Read from Table A
    float idxStr = p * 2048.0f;
    int idxInt = (int)idxStr;
    float frac = idxStr - idxInt;
    int idxNext = (idxInt + 1) % 2048;

    float valA1 = WAVETABLE_BANK[tableA][idxInt];
    float valA2 = WAVETABLE_BANK[tableA][idxNext];
    float outA = valA1 + frac * (valA2 - valA1);

    // 3. Read from Table B (if morphing)
    float outB = outA;
    if (morphFrac > 0.001f) {
      float valB1 = WAVETABLE_BANK[tableB][idxInt];
      float valB2 = WAVETABLE_BANK[tableB][idxNext];
      outB = valB1 + frac * (valB2 - valB1);
    }

    // 4. Blend
    return outA + morphFrac * (outB - outA);
  }
};

// ============================================================================
// SYSTEM STATE
// ============================================================================

// The Mailbox: Main Thread -> Audio Thread
EventQueue g_eventQueue(512);

// --- DSP OBJECTS (Reside in Audio Thread) ---
struct Engine {
  Oscillator osc;
  Oscillator mod; // The "Dirty" Modulator
  Fm2 fm;
  Adsr env;
  MoogLadder flt;
  ReverbSc verb;
  Overdrive drive;
  Chorus chorus;        // Legacy DaisySP Chorus
  WavetableOsc wt;      // NEW Wavetable Engine
  LogueWrapper logue;   // Custom Logue Adapter
  BraidsWrapper braids; // Braids Engine
  PlaitsWrapper plaits; // Plaits Engine

  // Chorus 2 (Custom)
  DelayLine<float, 4800> choDelayL; // ~100ms buffer
  DelayLine<float, 4800> choDelayR;
  Oscillator choLfo;

  DelayLine<float, DELAY_MAX_SAMPLES> delL;
  DelayLine<float, DELAY_MAX_SAMPLES> delR;

  // Local Parameter Cache (to avoid recalculating every sample)
  int waveform = Oscillator::WAVE_SIN;
  float amp = 0.5f;
  float driveAmt = 0.0f;

  // Effects State
  bool chorusOn = false;
  bool chorus2On = false; // New Custom Chorus
  float chorusRate = 0.3f;
  float chorusDepth = 0.8f;

  bool reverbOn = false;
  float reverbTime = 0.85f;    // Feedback
  float reverbTone = 10000.0f; // LP Cutoff

  bool delayOn = false;
  float delayFeed = 0.4f;
  bool gate = false;
  int activeNoteCount = 0;
  float baseFreq = 440.0f; // Stored pitch for FM calculation

  // Custom FM State
  bool customFmOn = false;
  float modRatio = 2.0f;
  float modIndex = 1.0f;

  void Init(float sampleRate) {
    osc.Init(sampleRate);
    mod.Init(sampleRate);
    mod.SetWaveform(Oscillator::WAVE_SIN); // Default modulator

    fm.Init(sampleRate);
    flt.Init(sampleRate);
    verb.Init(sampleRate);
    verb.SetFeedback(reverbTime);
    verb.SetLpFreq(reverbTone);
    env.Init(sampleRate);
    env.SetTime(ADSR_SEG_ATTACK, 0.01f);
    env.SetTime(ADSR_SEG_DECAY, 0.1f);
    env.SetSustainLevel(0.8f);
    env.SetTime(ADSR_SEG_RELEASE, 0.2f);
    drive.Init();
    chorus.Init(sampleRate);
    wt.Init(sampleRate);
    logue.Init(sampleRate);
    braids.Init(sampleRate);
    plaits.Init(sampleRate);

    // Init Chorus 2
    choDelayL.Init();
    choDelayR.Init();
    choLfo.Init(sampleRate);
    choLfo.SetWaveform(Oscillator::WAVE_SIN);
    choLfo.SetFreq(0.5f);
    choLfo.SetAmp(1.0f);

    delL.Init();
    delR.Init();
  }
};

Engine g_engine;

// ============================================================================
// AUDIO CALLBACK (CONSUMER)
// ============================================================================
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  float *pOut = (float *)pOutput;
  float sampleRate = (float)DEVICE_SAMPLE_RATE;

  // 1. PROCESS EVENTS (The "Mailbox Check")
  // We drain the queue completely at the start of each block.
  SynthEvent evt;
  while (g_eventQueue.pop(evt)) {
    switch (evt.type) {
    case TYPE_NOTE_ON:
      // Note On typically implies setting freq + gate
      if (g_engine.activeNoteCount == 0 || !g_engine.gate) {
        // New phrase: Clear old voices (fixes Muddy Drone)
        g_engine.logue.ResetVoices();
      }
      g_engine.activeNoteCount++;
      g_engine.gate = true;

      // Force update freq to ensure Logue triggers even if Note event came
      // early/late or if ResetVoices cleared it.
      g_engine.logue.SetFreq(g_engine.baseFreq);

      // Trigger other engines
      g_engine.plaits.Trigger();
      break;
    case TYPE_NOTE_OFF:
      if (g_engine.activeNoteCount > 0) {
        g_engine.activeNoteCount--;
      }

      // Always tell Logue to release this note
      g_engine.logue.NoteOff((uint8_t)evt.id);

      if (g_engine.activeNoteCount == 0) {
        g_engine.gate = false;
      }
      break;
    case TYPE_PARAM_CHANGE:
      switch (evt.id) {
      // --- OSCILLATOR ---
      case P_SYN1_OSC_FREQ:
        g_engine.baseFreq = evt.value;
        g_engine.osc.SetFreq(evt.value);
        g_engine.fm.SetFrequency(evt.value);
        g_engine.mod.SetFreq(evt.value * g_engine.modRatio);
        g_engine.wt.SetFreq(evt.value);
        g_engine.logue.SetFreq(evt.value); // FIX: Send freq to Logue
        g_engine.braids.SetFreq(evt.value);
        g_engine.plaits.SetFrequency(evt.value);
        break;
      case P_SYN1_OSC_WAVE:
        g_engine.waveform = (int)evt.value;
        if (g_engine.waveform != WAVE_FM) {
          g_engine.osc.SetWaveform(g_engine.waveform);
        }
        break;

      // --- AMP / ENV ---
      case 999: // Temporary ID for Amp
        g_engine.amp = evt.value;
        break;

      // --- FILTER ---
      case P_SYN1_FLT_CUTOFF:
        g_engine.flt.SetFreq(evt.value); // Expect Hz
        break;
      case P_SYN1_FLT_RES:
        g_engine.flt.SetRes(evt.value);
        break;

      // --- EFFECTS ---
      case P_SYN1_DRIVE:
        g_engine.driveAmt = evt.value;
        g_engine.drive.SetDrive(evt.value);
        break;

      // --- FM / CROSS MOD ---
      case 200: // FM Ratio
        g_engine.fm.SetRatio(evt.value);
        // Update custom modulator too
        g_engine.modRatio = evt.value;
        g_engine.mod.SetFreq(g_engine.baseFreq * g_engine.modRatio);
        break;
      case 201: // FM Index
        g_engine.fm.SetIndex(evt.value);
        g_engine.modIndex = evt.value;
        break;
      case 202: // Custom FM Enable (Bool)
        g_engine.customFmOn = (evt.value > 0.5f);
        std::cout << "CMD: Custom FM " << (g_engine.customFmOn ? "ON" : "OFF")
                  << std::endl;
        break;
      case 203: // Modulator Waveform
        g_engine.mod.SetWaveform((int)evt.value);
        break;

      // --- WAVETABLE ---
      case P_SYN1_WT_INDEX:
        g_engine.wt.SetMorph(evt.value);
        // Limit log spam for continuous slider
        // std::cout << "CMD: WT Morph " << evt.value << std::endl;
        break;

      // --- LOGUE PARAPHONIC ---
      case 220: // Logue Detune
        g_engine.logue.SetDetune(evt.value);
        break;
      case 221: // Logue Octave
        g_engine.logue.SetOctave((int)evt.value);
        break;

      // --- EFFECTS TOGGLES ---
      case P_MIX_REV_SEND:
        g_engine.reverbOn = (evt.value > 0.5f);
        break;
      case P_MIX_DLY_SEND:
        g_engine.delayOn = (evt.value > 0.5f);
        break;
      case 300:
        g_engine.chorusOn = (evt.value > 0.5f);
        break;

      // --- DELAY PARAMS ---
      case 301: // Delay Time
        g_engine.delL.SetDelay(evt.value * sampleRate);
        g_engine.delR.SetDelay(evt.value * sampleRate);
        break;
      case 302: // Delay Feed
        g_engine.delayFeed = evt.value;
        break;

      // --- CHORUS PARAMS ---
      case 310:
        g_engine.chorusRate = evt.value;
        g_engine.chorus.SetLfoFreq(evt.value);
        g_engine.choLfo.SetFreq(evt.value); // Update custom LFO too
        std::cout << "CMD: Chorus Rate " << evt.value << std::endl;
        break;
      case 311:
        g_engine.chorusDepth = evt.value;
        g_engine.chorus.SetLfoDepth(evt.value);
        std::cout << "CMD: Chorus Depth " << evt.value << std::endl;
        break;

      // --- REVERB PARAMS ---
      case 320:
        g_engine.reverbTime = evt.value;
        g_engine.verb.SetFeedback(g_engine.reverbTime);
        std::cout << "CMD: Reverb Time " << evt.value << std::endl;
        break;
      case 321:
        g_engine.reverbTone = evt.value;
        g_engine.verb.SetLpFreq(g_engine.reverbTone);
        std::cout << "CMD: Reverb Tone " << evt.value << std::endl;
        break;
      }
      break;
    }
  }

  // 2. GENERATE AUDIO
  static bool printedFmDebug = false;

  for (ma_uint32 i = 0; i < frameCount; ++i) {
    // ... (rest of audio loop same as before)
    // Envelope
    float envVal = g_engine.env.Process(g_engine.gate);

    // Source
    float sig;
    if (g_engine.waveform == WAVE_FM) {
      // Pure Fm2 Engine
      sig = g_engine.fm.Process();
    } else if (g_engine.waveform == WAVE_WT) {
      // Wavetable Engine
      sig = g_engine.wt.Process();
    } else if (g_engine.waveform == WAVE_LOGUE) {
      // Logue SDK Engine
      sig = g_engine.logue.Process();
    } else if (g_engine.waveform == WAVE_BRAIDS) {
      sig = g_engine.braids.Process();
    } else if (g_engine.waveform == WAVE_PLAITS) {
      sig = g_engine.plaits.Process();
    } else {
      // Standard Oscillator + Custom FM
      if (g_engine.customFmOn) {
        float modSig = g_engine.mod.Process();
        // Linear Frequency Modulation
        float fmAmt = modSig * g_engine.modIndex * g_engine.baseFreq;
        g_engine.osc.SetFreq(g_engine.baseFreq + fmAmt);

        if (!printedFmDebug) {
          std::cout << "DEBUG: FM ACTIVE. ModSig: " << modSig
                    << " Index: " << g_engine.modIndex
                    << " Base: " << g_engine.baseFreq << std::endl;
          printedFmDebug = true;
        }
      } else {
        printedFmDebug = false; // Reset if turned off
      }
      sig = g_engine.osc.Process();
    }

    sig *= envVal;

    // Drive
    if (g_engine.driveAmt > 0.01f) {
      sig = g_engine.drive.Process(sig);
    }

    // Filter
    sig = g_engine.flt.Process(sig);

    float left = sig;
    float right = sig;

    // Chorus (Custom "Chorus2" Implementation)
    if (g_engine.chorusOn) {
      // 1. Write to Delay
      g_engine.choDelayL.Write(left);
      g_engine.choDelayR.Write(right);

      // 2. Calculate LFO
      // Sine LFO (-0.5 to 0.5)
      float lfo = g_engine.choLfo.Process();

      // 3. Modulate Delay Time (10ms to 30ms)
      // Base = 20ms (960 samples @ 48k)
      // Swing = +/- 10ms * depth
      float baseDelay = 960.0f;
      float swing = 480.0f * g_engine.chorusDepth;

      float modL = baseDelay + (lfo * swing);
      float modR = baseDelay + (-lfo * swing); // Stereo phase inverted LFO

      // 4. Read Interpolated
      float wetL = g_engine.choDelayL.Read(modL);
      float wetR = g_engine.choDelayR.Read(modR);

      // 5. Mix (50/50)
      left = (left + wetL) * 0.707f;
      right = (right + wetR) * 0.707f;
    }

    // Delay
    if (g_engine.delayOn) {
      float dryL = left;
      float dryR = right;
      float readL = g_engine.delL.Read();
      float readR = g_engine.delR.Read();
      g_engine.delL.Write(dryL + (readL * g_engine.delayFeed));
      g_engine.delR.Write(dryR + (readR * g_engine.delayFeed));
      left = dryL + readL;
      right = dryR + readR;
    }

    // Reverb
    float outL, outR;
    if (g_engine.reverbOn) {
      g_engine.verb.Process(left, right, &outL, &outR);
    } else {
      outL = left;
      outR = right;
    }

    // Master Output
    pOut[i * DEVICE_CHANNELS] = outL * g_engine.amp;
    pOut[i * DEVICE_CHANNELS + 1] = outR * g_engine.amp;
  }
  (void)pInput;
}

// ============================================================================
// MAIN THREAD (PRODUCER)
// ============================================================================

// Helper to push updates
void send_param(uint16_t param_id, float value) {
  SynthEvent evt;
  evt.source_id = SRC_GUI_MAIN; // We assume GUI for now
  evt.type = TYPE_PARAM_CHANGE;
  evt.id = param_id;
  evt.value = value;
  // timestamp ignored for now
  g_eventQueue.push(evt);
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_POLL)
    return;

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    std::string uri(hm->uri.buf, hm->uri.len);

    if (uri == "/websocket") {
      mg_ws_upgrade(c, hm, NULL);
    } else if (uri == "/") {
      struct mg_http_serve_opts opts = {};
      opts.root_dir = "."; // Serve from root (index.html is one level up?)
      // Actually, main is in src/, index is in root. We might need to adjust.
      // For now, let's assume running from root: ./zynthora
      mg_http_serve_file(c, hm, "index.html", &opts);
    } else {
      mg_http_reply(c, 404, "", "Not Found");
    }
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
    std::string msg(wm->data.buf, wm->data.len);
    std::cout << "RX: " << msg << std::endl; // Debug logging ENABLED

    size_t colon = msg.find(':');
    if (colon != std::string::npos) {
      std::string cmd = msg.substr(0, colon);
      std::string valStr = msg.substr(colon + 1);

      try {
        // String Commands Map to Enums
        if (cmd == "wave") {
          float waveVal = 0.0f;
          if (valStr == "sine")
            waveVal = (float)Oscillator::WAVE_SIN;
          else if (valStr == "saw")
            waveVal = (float)Oscillator::WAVE_POLYBLEP_SAW;
          else if (valStr == "square")
            waveVal = (float)Oscillator::WAVE_POLYBLEP_SQUARE;
          else if (valStr == "triangle")
            waveVal = (float)Oscillator::WAVE_POLYBLEP_TRI;
          else if (valStr == "fm")
            waveVal = (float)WAVE_FM;
          else if (valStr == "wavetable")
            waveVal = (float)WAVE_WT;
          else if (valStr == "logue")
            waveVal = (float)WAVE_LOGUE;
          else if (valStr == "braids")
            waveVal = (float)WAVE_BRAIDS;
          else if (valStr == "plaits")
            waveVal = (float)WAVE_PLAITS;
          send_param(P_SYN1_OSC_WAVE, waveVal);
          return;
        }

        // Numeric Commands
        float val = std::stof(valStr);

        if (cmd == "note") {
          float freq = daisysp::mtof(val);
          send_param(P_SYN1_OSC_FREQ, freq);
        } else if (cmd == "gate") {
          SynthEvent evt;
          evt.source_id = SRC_GUI_MAIN;
          evt.type = (val > 0.5f) ? TYPE_NOTE_ON : TYPE_NOTE_OFF;
          g_eventQueue.push(evt);
        } else if (cmd == "off") {
          // Specific Note Off
          // We reuse send_note_off but pass ID
          SynthEvent e;
          e.source_id = SRC_GUI_MAIN;
          e.type = TYPE_NOTE_OFF;
          e.id = (uint16_t)val; // Pass Note Number
          e.value = 0.0f;
          g_eventQueue.push(e);
        } else if (cmd == "freq")
          send_param(P_SYN1_OSC_FREQ, val);

        // Wavetable Commands
        else if (cmd == "wt_index")
          send_param(P_SYN1_WT_INDEX, val);

        // Logue Commands
        else if (cmd == "logue_detune")
          send_param(220, val);
        else if (cmd == "logue_oct")
          send_param(221, val);

        // Braids Commands
        else if (cmd == "braids_model")
          g_engine.braids.SetModel((int)val); // Direct call for now
        else if (cmd == "braids_timbre")
          g_engine.braids.SetTimbre(val);
        else if (cmd == "braids_color")
          g_engine.braids.SetColor(val);
        else if (cmd == "braids_fine")
          g_engine.braids.SetFine(val);
        else if (cmd == "braids_coarse")
          g_engine.braids.SetCoarse(val);
        else if (cmd == "braids_fm")
          g_engine.braids.SetFM(val);
        else if (cmd == "braids_modulation")
          g_engine.braids.SetModulation(val);

        // Plaits Commands
        else if (cmd == "plaits_model")
          g_engine.plaits.SetModel((int)val);
        else if (cmd == "plaits_harmonics")
          g_engine.plaits.SetHarmonics(val);
        else if (cmd == "plaits_timbre")
          g_engine.plaits.SetTimbre(val);
        else if (cmd == "plaits_morph")
          g_engine.plaits.SetMorph(val);
        else if (cmd == "plaits_fm")
          g_engine.plaits.SetFMAmount(val);
        else if (cmd == "plaits_mod")
          g_engine.plaits.SetModAmount(val);
        else if (cmd == "plaits_decay")
          g_engine.plaits.SetDecay(val);
        else if (cmd == "plaits_coarse")
          g_engine.plaits.SetCoarse(val);
        else if (cmd == "plaits_fine")
          g_engine.plaits.SetFine(val);

        else if (cmd == "amp")
          send_param(999, val);
        else if (cmd == "cutoff")
          send_param(P_SYN1_FLT_CUTOFF, val);
        else if (cmd == "res")
          send_param(P_SYN1_FLT_RES, val);
        else if (cmd == "drive")
          send_param(P_SYN1_DRIVE, val);

        // Toggles
        else if (cmd == "reverb")
          send_param(P_MIX_REV_SEND, val);
        else if (cmd == "chorus")
          send_param(300, val);
        else if (cmd == "delay")
          send_param(P_MIX_DLY_SEND, val);

        // Delay Params
        else if (cmd == "dtime")
          send_param(301, val);
        else if (cmd == "dfeed")
          send_param(302, val);

        // Chorus Params
        else if (cmd == "chorus_rate")
          send_param(310, val);
        else if (cmd == "chorus_depth")
          send_param(311, val);

        // Reverb Params
        else if (cmd == "reverb_time")
          send_param(320, val);
        else if (cmd == "reverb_tone")
          send_param(321, val);

        // FM Params
        else if (cmd == "fm_ratio")
          send_param(200, val);
        else if (cmd == "fm_index")
          send_param(201, val);
        else if (cmd == "fm_enable")
          send_param(202, val);
        else if (cmd == "mod_wave")
          send_param(203, val);

      } catch (...) {
        // Ignore parse errors
      }
    }
  }
}

int main() {
  g_engine.Init((float)DEVICE_SAMPLE_RATE);

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = DEVICE_FORMAT;
  config.playback.channels = DEVICE_CHANNELS;
  config.sampleRate = DEVICE_SAMPLE_RATE;
  config.dataCallback = data_callback;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
    return -1;
  if (ma_device_start(&device) != MA_SUCCESS)
    return -1;

  std::cout << "Zynthora (Event Backbone) Started." << std::endl;

  mg_log_set(0);
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);

  while (true)
    mg_mgr_poll(&mgr, 10); // Polling every 10ms

  mg_mgr_free(&mgr);
  ma_device_uninit(&device);
  return 0;
}
