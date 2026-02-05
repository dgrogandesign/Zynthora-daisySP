#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "mongoose.h"
#include "events.h"
#include "lockfree_queue.h"

// DaisySP Headers
#include "DaisySP/Source/daisysp.h"
#include "Filters/moogladder.h"
#include "Effects/reverbsc.h"
#include "Effects/overdrive.h"
#include "Effects/chorus.h"
#include "Control/adsr.h"
#include "Utility/delayline.h"
#include "Synthesis/fm2.h"

#include <iostream>
#include <string>
#include <vector>

using namespace daisysp;

#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000
#define DELAY_MAX_SAMPLES   48000 
#define WAVE_FM             100

// ============================================================================
// SYSTEM STATE
// ============================================================================

// The Mailbox: Main Thread -> Audio Thread
EventQueue g_eventQueue(512);

// --- DSP OBJECTS (Reside in Audio Thread) ---
struct Engine {
    Oscillator osc;
    Oscillator mod; // The "Dirty" Modulator
    Fm2        fm;
    Adsr       env;
    MoogLadder flt;
    ReverbSc   verb;
    Overdrive  drive;
    Chorus     chorus;
    DelayLine<float, DELAY_MAX_SAMPLES> delL;
    DelayLine<float, DELAY_MAX_SAMPLES> delR;

    // Local Parameter Cache (to avoid recalculating every sample)
    int   waveform   = Oscillator::WAVE_SIN;
    float amp        = 0.5f;
    float driveAmt   = 0.0f;
    
    // Effects State
    bool  chorusOn   = false;
    float chorusRate = 0.3f;
    float chorusDepth= 0.8f;
    
    bool  reverbOn   = false;
    float reverbTime = 0.85f; // Feedback
    float reverbTone = 10000.0f; // LP Cutoff
    
    bool  delayOn    = false;
    float delayFeed  = 0.4f;
    bool  gate       = false;
    float baseFreq   = 440.0f; // Stored pitch for FM calculation
    
    // Custom FM State
    bool  customFmOn = false;
    float modRatio   = 2.0f;
    float modIndex   = 1.0f;

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
        delL.Init();
        delR.Init();
    }
};

Engine g_engine;

// ============================================================================
// AUDIO CALLBACK (CONSUMER)
// ============================================================================
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOut = (float*)pOutput;
    float sampleRate = (float)DEVICE_SAMPLE_RATE;
    
    // 1. PROCESS EVENTS (The "Mailbox Check")
    // We drain the queue completely at the start of each block.
    SynthEvent evt;
    while (g_eventQueue.pop(evt)) {
        switch (evt.type) {
            case TYPE_NOTE_ON:
                // Note On typically implies setting freq + gate
                g_engine.gate = true;
                break;
            case TYPE_NOTE_OFF:
                g_engine.gate = false;
                break;
            case TYPE_PARAM_CHANGE:
                switch (evt.id) {
                    // --- OSCILLATOR ---
                    case P_SYN1_OSC_FREQ:
                        g_engine.baseFreq = evt.value;
                        g_engine.osc.SetFreq(evt.value);
                        g_engine.fm.SetFrequency(evt.value);
                        g_engine.mod.SetFreq(evt.value * g_engine.modRatio); 
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
                        std::cout << "CMD: Custom FM " << (g_engine.customFmOn ? "ON" : "OFF") << std::endl;
                        break;
                    case 203: // Modulator Waveform
                        g_engine.mod.SetWaveform((int)evt.value);
                        break;

                    // --- EFFECTS TOGGLES ---
                    case P_MIX_REV_SEND: g_engine.reverbOn = (evt.value > 0.5f); break;
                    case P_MIX_DLY_SEND: g_engine.delayOn  = (evt.value > 0.5f); break;
                    case 300: g_engine.chorusOn = (evt.value > 0.5f); break;
                    
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
        } else {
            // Standard Oscillator + Custom FM
            if (g_engine.customFmOn) {
                float modSig = g_engine.mod.Process();
                // Linear Frequency Modulation
                float fmAmt = modSig * g_engine.modIndex * g_engine.baseFreq;
                g_engine.osc.SetFreq(g_engine.baseFreq + fmAmt);
                
                if (!printedFmDebug) {
                    std::cout << "DEBUG: FM ACTIVE. ModSig: " << modSig << " Index: " << g_engine.modIndex << " Base: " << g_engine.baseFreq << std::endl;
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

        // Chorus
        if (g_engine.chorusOn) {
            g_engine.chorus.Process(left);
            left = g_engine.chorus.GetLeft() * 1.4f; 
            right = g_engine.chorus.GetRight() * 1.4f;
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
        pOut[i * DEVICE_CHANNELS]     = outL * g_engine.amp;
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
  if (ev == MG_EV_POLL) return;

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
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
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
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
                if (valStr == "sine") waveVal = (float)Oscillator::WAVE_SIN;
                else if (valStr == "saw") waveVal = (float)Oscillator::WAVE_POLYBLEP_SAW;
                else if (valStr == "square") waveVal = (float)Oscillator::WAVE_POLYBLEP_SQUARE;
                else if (valStr == "triangle") waveVal = (float)Oscillator::WAVE_POLYBLEP_TRI;
                else if (valStr == "fm") waveVal = (float)WAVE_FM;
                send_param(P_SYN1_OSC_WAVE, waveVal);
                return;
            }

            // Numeric Commands
            float val = std::stof(valStr);
            
            if (cmd == "note") {
                float freq = daisysp::mtof(val);
                send_param(P_SYN1_OSC_FREQ, freq);
            }
            else if (cmd == "gate") {
                SynthEvent evt;
                evt.source_id = SRC_GUI_MAIN;
                evt.type = (val > 0.5f) ? TYPE_NOTE_ON : TYPE_NOTE_OFF;
                g_eventQueue.push(evt);
            }
            else if (cmd == "freq")   send_param(P_SYN1_OSC_FREQ, val);
            else if (cmd == "amp")    send_param(999, val);
            else if (cmd == "cutoff") send_param(P_SYN1_FLT_CUTOFF, val);
            else if (cmd == "res")    send_param(P_SYN1_FLT_RES, val);
            else if (cmd == "drive")  send_param(P_SYN1_DRIVE, val);
            
            // Toggles
            else if (cmd == "reverb") send_param(P_MIX_REV_SEND, val);
            else if (cmd == "chorus") send_param(300, val);
            else if (cmd == "delay")  send_param(P_MIX_DLY_SEND, val);
            
            // Delay Params
            else if (cmd == "dtime")  send_param(301, val);
            else if (cmd == "dfeed")  send_param(302, val);
            
            // Chorus Params
            else if (cmd == "chorus_rate")  send_param(310, val);
            else if (cmd == "chorus_depth") send_param(311, val);
            
            // Reverb Params
            else if (cmd == "reverb_time") send_param(320, val);
            else if (cmd == "reverb_tone") send_param(321, val);
            
            // FM Params
            else if (cmd == "fm_ratio") send_param(200, val);
            else if (cmd == "fm_index") send_param(201, val);
            else if (cmd == "fm_enable") send_param(202, val);
            else if (cmd == "mod_wave")  send_param(203, val);

        } catch (...) {
            // Ignore parse errors
        }
    }
  }
}

int main() {
    g_engine.Init((float)DEVICE_SAMPLE_RATE);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = DEVICE_FORMAT;
    config.playback.channels = DEVICE_CHANNELS;
    config.sampleRate        = DEVICE_SAMPLE_RATE;
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return -1;
    if (ma_device_start(&device) != MA_SUCCESS) return -1;

    std::cout << "Zynthora (Event Backbone) Started." << std::endl;

    mg_log_set(0); 
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);
    
    while (true) mg_mgr_poll(&mgr, 10); // Polling every 10ms

    mg_mgr_free(&mgr);
    ma_device_uninit(&device);
    return 0;
}
