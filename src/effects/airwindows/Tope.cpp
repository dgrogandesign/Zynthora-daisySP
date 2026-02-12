/* ========================================
 *  Tope - Tope.cpp
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 * ======================================== */

#ifndef __Tope_H
#include "Tope.h"
#endif

namespace airwindows {

// AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {return
// new Tope(audioMaster);}

Tope::Tope(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 0, 2) {
  A = 0.5;
  B = 0.5;

  iirMidRolloffL = 0.0;
  iirMidRolloffR = 0.0;
  iirHeadBumpL = 0.0;
  iirHeadBumpR = 0.0;

  iirSampleA = 0.0;
  iirSampleB = 0.0;
  iirSampleC = 0.0;
  iirSampleD = 0.0;
  iirSampleE = 0.0;
  iirSampleF = 0.0;
  iirSampleG = 0.0;
  iirSampleH = 0.0;
  iirSampleI = 0.0;
  iirSampleJ = 0.0;
  iirSampleK = 0.0;
  iirSampleL = 0.0;
  iirSampleM = 0.0;
  iirSampleN = 0.0;
  iirSampleO = 0.0;
  iirSampleP = 0.0;
  iirSampleQ = 0.0;
  iirSampleR = 0.0;
  iirSampleS = 0.0;
  iirSampleT = 0.0;
  iirSampleU = 0.0;
  iirSampleV = 0.0;
  iirSampleW = 0.0;
  iirSampleX = 0.0;
  iirSampleY = 0.0;
  iirSampleZ = 0.0;

  fpdL = 1.0;
  while (fpdL < 16386)
    fpdL = rand() *
           16384; // UINT32_MAX is too large for float/double multiplication
  fpdR = 1.0;
  while (fpdR < 16386)
    fpdR = rand() * 16384;
  // this is reset: values being initialized only once. Startup values, whatever
  // they are.

  _canDo.insert(
      "plugAsChannelInsert"); // plug-in can be used as a channel insert effect.
  _canDo.insert("plugAsSend"); // plug-in can be used as a send effect.
  _canDo.insert("x2in2out");
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID('tope');
  canProcessReplacing(); // supports output replacing
  canDoubleReplacing();  // supports double precision processing
  programsAreChunks(true);
  vst_strncpy(_programName, "Default",
              kVstMaxProgNameLen); // default program name
}

Tope::~Tope() {}
VstInt32 Tope::getVendorVersion() { return 1000; }
void Tope::setProgramName(char *name) {
  vst_strncpy(_programName, name, kVstMaxProgNameLen);
}
void Tope::getProgramName(char *name) {
  vst_strncpy(name, _programName, kVstMaxProgNameLen);
}
// airwindows likes to ignore this stuff. Make your own programs, and make a
// different plugin rather than trying to do versioning and preventing people
// from using older versions. Maybe they like the old one!

static float pinParameter(float data) {
  if (data < 0.0f)
    return 0.0f;
  if (data > 1.0f)
    return 1.0f;
  return data;
}

VstInt32 Tope::getChunk(void **data, bool isPreset) {
  float *chunkData = (float *)calloc(2, sizeof(float));
  chunkData[0] = A;
  chunkData[1] = B;
  /* Note: The way this is set up, it will break if you manage to save settings
   on an Intel machine and load them on a PPC Mac. However, it's fine if you
   stick to the machine you started with. */

  *data = chunkData;
  return 2 * sizeof(float);
}

VstInt32 Tope::setChunk(void *data, VstInt32 byteSize, bool isPreset) {
  float *chunkData = (float *)data;
  A = pinParameter(chunkData[0]);
  B = pinParameter(chunkData[1]);
  /* We're ignoring byteSize as we found it to be a filthy liar */

  /* calculate any other fields you need here - you could copy in
   code from setParameter() here. */
  return 0;
}

void Tope::setParameter(VstInt32 index, float value) {
  switch (index) {
  case kParamA:
    A = value;
    break;
  case kParamB:
    B = value;
    break;
  default:
    break; // unknown parameter, shouldn't happen!
  }
}

float Tope::getParameter(VstInt32 index) {
  switch (index) {
  case kParamA:
    return A;
    break;
  case kParamB:
    return B;
    break;
  default:
    break; // unknown parameter, shouldn't happen!
  }
  return 0.0; // we only need to update the relevant name, this is noops for
              // everything else
}

void Tope::getParameterName(VstInt32 index, char *text) {
  switch (index) {
  case kParamA:
    vst_strncpy(text, "Tope", kVstMaxParamStrLen);
    break;
  case kParamB:
    vst_strncpy(text, "Fuzz", kVstMaxParamStrLen);
    break;
  default:
    break; // unknown parameter, shouldn't happen!
  } // this is our labels for displaying in the VST host
}

void Tope::getParameterDisplay(VstInt32 index, char *text) {
  switch (index) {
  case kParamA:
    float2string(A, text, kVstMaxParamStrLen);
    break;
  case kParamB:
    float2string(B, text, kVstMaxParamStrLen);
    break;
  default:
    break; // unknown parameter, shouldn't happen!
  } // this displays the values and handles 'popups' where it's discrete choices
}

void Tope::getParameterLabel(VstInt32 index, char *text) {
  switch (index) {
  case kParamA:
    vst_strncpy(text, "", kVstMaxParamStrLen);
    break;
  case kParamB:
    vst_strncpy(text, "", kVstMaxParamStrLen);
    break;
  default:
    break; // unknown parameter, shouldn't happen!
  }
}

bool Tope::getEffectName(char *name) {
  vst_strncpy(name, "Tope", kVstMaxProductStrLen);
  return true;
}

VstInt32 Tope::canDo(char *text) {
  if (_canDo.find(text) == _canDo.end())
    return 0;
  else
    return 1;
}

bool Tope::getVendorString(char *text) {
  vst_strncpy(text, "airwindows", kVstMaxVendorStrLen);
  return true;
}

bool Tope::getProductString(char *text) {
  vst_strncpy(text, "Tope", kVstMaxProductStrLen);
  return true;
}

} // end namespace airwindows
