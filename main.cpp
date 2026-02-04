#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "mongoose.h"
#include "DaisySP/Source/daisysp.h"
#include "Filters/moogladder.h"
#include "Effects/reverbsc.h"
#include <atomic>
#include <iostream>
#include <string>

using namespace daisysp;

#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000

// --- GLOBAL STATE ---
std::atomic<float> g_frequency(440.0f);
std::atomic<float> g_amplitude(0.5f);
std::atomic<float> g_cutoff(20000.0f);
std::atomic<float> g_res(0.0f);
std::atomic<int>   g_waveform(Oscillator::WAVE_SAW);
std::atomic<bool>  g_reverbOn(true);

// --- DSP OBJECTS ---
Oscillator osc;
MoogLadder flt;
ReverbSc   verb;

// --- AUDIO CALLBACK ---
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    float* pOut = (float*)pOutput;
    
    // Update DSP Params from Global State
    osc.SetFreq(g_frequency.load());
    osc.SetAmp(g_amplitude.load());
    osc.SetWaveform(g_waveform.load());
    
    flt.SetFreq(g_cutoff.load());
    flt.SetRes(g_res.load());

    for (ma_uint32 i = 0; i < frameCount; ++i) {
        // 1. Generate
        float sig = osc.Process();
        
        // 2. Filter
        sig = flt.Process(sig);
        
        // 3. Reverb (Stereo)
        float outL, outR;
        if (g_reverbOn.load()) {
            verb.Process(sig, sig, &outL, &outR);
        } else {
            outL = sig;
            outR = sig;
        }

        // 4. Output
        pOut[i * DEVICE_CHANNELS]     = outL;
        pOut[i * DEVICE_CHANNELS + 1] = outR;
    }
    (void)pInput;
}

// --- WEBSOCKET HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_POLL) return; // Don't log poll
  // std::cout << "Event: " << ev << std::endl;

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    std::string uri(hm->uri.buf, hm->uri.len);
    std::cout << "HTTP Request: " << uri << std::endl; // Debug Print

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
    std::cout << "RX: " << msg << std::endl; // Debug print
    
    size_t colon = msg.find(':');
    if (colon != std::string::npos) {
        std::string cmd = msg.substr(0, colon);
        std::string valStr = msg.substr(colon + 1);
        try {
            float val = std::stof(valStr);
            if (cmd == "freq") g_frequency.store(val);
            else if (cmd == "amp") g_amplitude.store(val);
            else if (cmd == "cutoff") g_cutoff.store(val);
            else if (cmd == "res") g_res.store(val);
            else if (cmd == "reverb") g_reverbOn.store(val > 0.5f);
            else if (cmd == "wave") {
                if (valStr == "sine") g_waveform.store(Oscillator::WAVE_SIN);
                else if (valStr == "saw") g_waveform.store(Oscillator::WAVE_POLYBLEP_SAW);
                else if (valStr == "square") g_waveform.store(Oscillator::WAVE_POLYBLEP_SQUARE);
                else if (valStr == "triangle") g_waveform.store(Oscillator::WAVE_POLYBLEP_TRI);
            }
        } catch (...) {}
    }
  }
}

int main() {
    // DSP Init
    float sampleRate = (float)DEVICE_SAMPLE_RATE;
    osc.Init(sampleRate);
    flt.Init(sampleRate);
    verb.Init(sampleRate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(10000.0f);

    // Audio Init
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = DEVICE_FORMAT;
    config.playback.channels = DEVICE_CHANNELS;
    config.sampleRate        = DEVICE_SAMPLE_RATE;
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return -1;
    if (ma_device_start(&device) != MA_SUCCESS) return -1;

    std::cout << "Zynthora (DaisySP Edition) Started." << std::endl;

    mg_log_set(0); // Disable debug logs

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);
    
    while (true) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    ma_device_uninit(&device);
    return 0;
}
