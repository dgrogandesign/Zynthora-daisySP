#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "mongoose.h"
#include "DaisySP/Source/daisysp.h"
#include "Filters/moogladder.h"
#include "Effects/reverbsc.h"
#include "Effects/overdrive.h"
#include "Effects/chorus.h"
#include "Control/adsr.h"
#include "Utility/delayline.h"
#include <atomic>
#include <iostream>
#include <string>

using namespace daisysp;

#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000
#define DELAY_MAX_SAMPLES   48000 

// --- GLOBAL STATE ---
std::atomic<float> g_frequency(440.0f);
std::atomic<float> g_amplitude(0.5f);
std::atomic<float> g_cutoff(20000.0f);
std::atomic<float> g_res(0.0f);
std::atomic<int>   g_waveform(Oscillator::WAVE_SAW);
std::atomic<bool>  g_gate(false); 

// Effects State
std::atomic<float> g_driveAmt(0.0f);
std::atomic<bool>  g_chorusOn(false);
std::atomic<bool>  g_reverbOn(true);
std::atomic<bool>  g_delayOn(false);
std::atomic<float> g_delayTime(0.3f);
std::atomic<float> g_delayFeed(0.4f);

// --- DSP OBJECTS ---
Oscillator osc;
Adsr       env;
MoogLadder flt;
ReverbSc   verb;
Overdrive  drive;
Chorus     chorus;
DelayLine<float, DELAY_MAX_SAMPLES> delL;
DelayLine<float, DELAY_MAX_SAMPLES> delR;

// --- AUDIO CALLBACK ---
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOut = (float*)pOutput;
    float sampleRate = (float)DEVICE_SAMPLE_RATE;
    
    // Update DSP Params
    osc.SetFreq(g_frequency.load());
    osc.SetAmp(g_amplitude.load());
    osc.SetWaveform(g_waveform.load());
    
    flt.SetFreq(g_cutoff.load());
    flt.SetRes(g_res.load());
    
    drive.SetDrive(g_driveAmt.load());
    
    // Envelope Params (Fixed for now, or add sliders later)
    env.SetTime(ADSR_SEG_ATTACK, 0.01f);
    env.SetTime(ADSR_SEG_DECAY, 0.1f);
    env.SetSustainLevel(0.8f);
    env.SetTime(ADSR_SEG_RELEASE, 0.2f);
    
    // Delay params
    float dTime = g_delayTime.load() * sampleRate;
    delL.SetDelay(dTime);
    delR.SetDelay(dTime);
    float dFeed = g_delayFeed.load();
    bool dOn = g_delayOn.load();

    bool cOn = g_chorusOn.load();
    if (cOn) {
        chorus.SetLfoFreq(0.3f); 
        chorus.SetLfoDepth(0.8f);
    }
    
    // Gate logic needs to be handled carefully in the block if it changes rapidly, 
    // but for now we poll it once per block is okayish for <10ms latency.
    bool gate = g_gate.load();

    for (ma_uint32 i = 0; i < frameCount; ++i) {
        // 1. Envelope
        float envVal = env.Process(gate);
        
        // 2. Oscillator
        float sig = osc.Process();
        
        // Apply Envelope BEFORE effects
        sig *= envVal;
        
        // 3. Overdrive
        sig = drive.Process(sig);
        
        // 4. Filter
        sig = flt.Process(sig);
        
        float left = sig;
        float right = sig;

        // 5. Chorus
        if (cOn) {
            chorus.Process(left);
            left = chorus.GetLeft();
            right = chorus.GetRight();
        }

        // 6. Delay
        if (dOn) {
            float dryL = left;
            float dryR = right;
            float readL = delL.Read();
            float readR = delR.Read();
            delL.Write(dryL + (readL * dFeed));
            delR.Write(dryR + (readR * dFeed));
            left = dryL + readL;
            right = dryR + readR;
        }

        // 7. Reverb
        float outL, outR;
        if (g_reverbOn.load()) {
            verb.Process(left, right, &outL, &outR);
        } else {
            outL = left;
            outR = right;
        }

        pOut[i * DEVICE_CHANNELS]     = outL;
        pOut[i * DEVICE_CHANNELS + 1] = outR;
    }
    (void)pInput;
}

// --- WEBSOCKET HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_POLL) return;

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    std::string uri(hm->uri.buf, hm->uri.len);
    // std::cout << "HTTP Request: " << uri << std::endl; 

    if (uri == "/websocket") {
        mg_ws_upgrade(c, hm, NULL);
    } else if (uri == "/") {
        struct mg_http_serve_opts opts = {};
        opts.root_dir = ".";
        mg_http_serve_file(c, hm, "index.html", &opts);
    } else {
        mg_http_reply(c, 404, "", "Not Found");
    }
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    std::string msg(wm->data.buf, wm->data.len);
    std::cout << "RX: " << msg << std::endl;
    
    size_t colon = msg.find(':');
    if (colon != std::string::npos) {
        std::string cmd = msg.substr(0, colon);
        std::string valStr = msg.substr(colon + 1);
        
        if (cmd == "wave") {
            if (valStr == "sine") g_waveform.store(Oscillator::WAVE_SIN);
            else if (valStr == "saw") g_waveform.store(Oscillator::WAVE_POLYBLEP_SAW);
            else if (valStr == "square") g_waveform.store(Oscillator::WAVE_POLYBLEP_SQUARE);
            else if (valStr == "triangle") g_waveform.store(Oscillator::WAVE_POLYBLEP_TRI);
            return;
        }

        try {
            float val = std::stof(valStr);
            if (cmd == "freq") g_frequency.store(val);
            else if (cmd == "note") g_frequency.store(mtof(val));
            else if (cmd == "gate") g_gate.store(val > 0.5f);
            else if (cmd == "amp") g_amplitude.store(val);
            else if (cmd == "cutoff") g_cutoff.store(val);
            else if (cmd == "res") g_res.store(val);
            else if (cmd == "reverb") g_reverbOn.store(val > 0.5f);
            else if (cmd == "drive") g_driveAmt.store(val);
            else if (cmd == "chorus") g_chorusOn.store(val > 0.5f);
            else if (cmd == "delay") g_delayOn.store(val > 0.5f);
            else if (cmd == "dtime") g_delayTime.store(val);
            else if (cmd == "dfeed") g_delayFeed.store(val);
        } catch (...) {
            std::cout << "Parse Error for: " << cmd << ":" << valStr << std::endl;
        }
    }
  }
}

int main() {
    float sampleRate = (float)DEVICE_SAMPLE_RATE;
    
    osc.Init(sampleRate);
    flt.Init(sampleRate);
    verb.Init(sampleRate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(10000.0f);
    
    env.Init(sampleRate);
    drive.Init();
    chorus.Init(sampleRate);
    delL.Init();
    delR.Init();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = DEVICE_FORMAT;
    config.playback.channels = DEVICE_CHANNELS;
    config.sampleRate        = DEVICE_SAMPLE_RATE;
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return -1;
    if (ma_device_start(&device) != MA_SUCCESS) return -1;

    std::cout << "Zynthora (Playable) Started." << std::endl;

    mg_log_set(0); 
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);
    
    while (true) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    ma_device_uninit(&device);
    return 0;
}
