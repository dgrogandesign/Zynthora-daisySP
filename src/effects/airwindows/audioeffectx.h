#ifndef __audioeffect__
#define __audioeffect__

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

typedef int VstInt32;

class AudioEffectX;
typedef AudioEffectX AudioEffect;

class AudioEffectX {
public:
  AudioEffectX(void *audioMaster, int programs, int params) {}
  virtual ~AudioEffectX() {}

  virtual void setParameter(VstInt32 index, float value) = 0;
  virtual float getParameter(VstInt32 index) = 0;
  virtual void processReplacing(float **inputs, float **outputs,
                                VstInt32 sampleFrames) = 0;
  virtual void processDoubleReplacing(double **inputs, double **outputs,
                                      VstInt32 sampleFrames) = 0;

  // Stub VST functions
  virtual bool getEffectName(char *name) { return false; }
  virtual bool getVendorString(char *text) { return false; }
  virtual bool getProductString(char *text) { return false; }
  virtual VstInt32 getVendorVersion() { return 0; }
  virtual void setProgramName(char *name) {}
  virtual void getProgramName(char *name) {}
  virtual void suspend() {}
  virtual void resume() {}

  // Added stubs for Airwindows compatibility
  virtual void setNumInputs(int inputs) {}
  virtual void setNumOutputs(int outputs) {}
  virtual void setUniqueID(int id) {}
  virtual void canProcessReplacing() {}
  virtual void canDoubleReplacing() {}
  virtual void programsAreChunks(bool v) {}

  // Add other mocks as needed by Airwindows plugins
  bool isOutputConnected(long index) { return true; }

  // Helper to get chunk (not used usually in Airwindows simple plugins)
  virtual VstInt32 getChunk(void **data, bool isPreset) { return 0; }
  virtual VstInt32 setChunk(void *data, VstInt32 byteSize, bool isPreset) {
    return 0;
  }

  // Constants
  static const int kVstMaxProgNameLen = 24;
  static const int kVstMaxParamStrLen = 24;
  static const int kVstMaxVendorStrLen = 64;
  static const int kVstMaxProductStrLen = 64;

  // Helpers
  static void vst_strncpy(char *dst, const char *src, size_t maxLen) {
    strncpy(dst, src, maxLen);
    dst[maxLen] = 0;
  }

  static void float2string(float value, char *text, size_t maxLen) {
    snprintf(text, maxLen, "%.2f", value);
  }

  static void int2string(int value, char *text, size_t maxLen) {
    snprintf(text, maxLen, "%d", value);
  }

  // Added for functionality
  virtual void setSampleRate(float sr) { sampleRate_ = sr; }
  virtual float getSampleRate() { return sampleRate_; }

protected:
  float sampleRate_ = 44100.0f;
};

// Typedef for callback (unused)
typedef void *audioMasterCallback;

// Categories
enum VstPlugCategory { kPlugCategEffect = 0 };

#endif
