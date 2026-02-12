/* ========================================
 *  Tope - Tope.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 * ======================================== */

#ifndef __Tope_H
#define __Tope_H

#ifndef __audioeffect__
#include "audioeffectx.h"
#endif

#include <math.h>
#include <set>
#include <string>

namespace airwindows {

class Tope : public AudioEffectX {
public:
  enum { kParamA = 0, kParamB = 1, kNumParameters = 2 }; //

  Tope(audioMasterCallback audioMaster);
  ~Tope();
  virtual bool getEffectName(char *name); // The plug-in name
  virtual VstInt32 getVendorVersion();
  virtual bool getVendorString(char *text);  // This is a C-string parameter
  virtual bool getProductString(char *text); // This is a C-string parameter
  virtual VstInt32 canDo(char *text);

  virtual VstInt32 getChunk(void **data, bool isPreset);
  virtual VstInt32 setChunk(void *data, VstInt32 byteSize, bool isPreset);

  virtual float getParameter(VstInt32 index);
  virtual void setParameter(VstInt32 index, float value);
  virtual void getParameterLabel(VstInt32 index, char *label);
  virtual void getParameterDisplay(VstInt32 index, char *text);
  virtual void getParameterName(VstInt32 index, char *text);

  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);

  virtual void processReplacing(float **inputs, float **outputs,
                                VstInt32 sampleFrames);
  virtual void processDoubleReplacing(double **inputs, double **outputs,
                                      VstInt32 sampleFrames);

private:
  double iirMidRolloffL;
  double iirMidRolloffR;
  double iirHeadBumpL;
  double iirHeadBumpR;

  double iirSampleA;
  double iirSampleB;
  double iirSampleC;
  double iirSampleD;
  double iirSampleE;
  double iirSampleF;
  double iirSampleG;
  double iirSampleH;
  double iirSampleI;
  double iirSampleJ;
  double iirSampleK;
  double iirSampleL;
  double iirSampleM;
  double iirSampleN;
  double iirSampleO;
  double iirSampleP;
  double iirSampleQ;
  double iirSampleR;
  double iirSampleS;
  double iirSampleT;
  double iirSampleU;
  double iirSampleV;
  double iirSampleW;
  double iirSampleX;
  double iirSampleY;
  double iirSampleZ;

  float A;
  float B;

  char _programName[kVstMaxProgNameLen + 1];
  std::set<std::string> _canDo;

  uint32_t fpdL;
  uint32_t fpdR;
  // default stuff
};

} // end namespace airwindows

#endif
