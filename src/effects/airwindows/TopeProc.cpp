/* ========================================
 *  Tope - Tope.h
 *  Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 * ======================================== */

#ifndef __Tope_H
#include "Tope.h"
#endif

namespace airwindows {

void Tope::processReplacing(float **inputs, float **outputs,
                            VstInt32 sampleFrames) {
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  double overallscale = 1.0;
  overallscale /= 44100.0;
  overallscale *= getSampleRate();

  double inputgain = pow(10.0, ((A * 24.0) - 12.0) / 20.0);
  double compfactor = 0.012 * (A / 135.0);
  double range = pow(B, 3) * 25;
  if (range < 0.01)
    range = 0.01;
  // high numbers are shorter, tighter processing

  while (--sampleFrames >= 0) {
    double inputSampleL = *in1;
    double inputSampleR = *in2;
    if (fabs(inputSampleL) < 1.18e-23)
      inputSampleL = fpdL * 1.18e-17;
    if (fabs(inputSampleR) < 1.18e-23)
      inputSampleR = fpdR * 1.18e-17;

    if (inputgain != 1.0) {
      inputSampleL *= inputgain;
      inputSampleR *= inputgain;
    }

    double bridgerectifierL = fabs(inputSampleL);
    if (bridgerectifierL > 1.2533141373155)
      bridgerectifierL = 1.2533141373155;
    // clip infinite input to reach the saturation logic
    if (bridgerectifierL > 1.0)
      bridgerectifierL = sin(bridgerectifierL);
    else
      bridgerectifierL = 1 - cos(bridgerectifierL);
    // produce the interaction that will cut back the treble

    double bridgerectifierR = fabs(inputSampleR);
    if (bridgerectifierR > 1.2533141373155)
      bridgerectifierR = 1.2533141373155;
    // clip infinite input to reach the saturation logic
    if (bridgerectifierR > 1.0)
      bridgerectifierR = sin(bridgerectifierR);
    else
      bridgerectifierR = 1 - cos(bridgerectifierR);
    // produce the interaction that will cut back the treble

    if (inputSampleL > 0)
      inputSampleL -= (bridgerectifierL * compfactor);
    else
      inputSampleL += (bridgerectifierL * compfactor);

    if (inputSampleR > 0)
      inputSampleR -= (bridgerectifierR * compfactor);
    else
      inputSampleR += (bridgerectifierR * compfactor);
    // inverse gain for high frequencies

    iirMidRolloffL = (iirMidRolloffL * (1.0 - range)) + (inputSampleL * range);
    inputSampleL = iirMidRolloffL;
    iirMidRolloffR = (iirMidRolloffR * (1.0 - range)) + (inputSampleR * range);
    inputSampleR = iirMidRolloffR;
    // slew limit against the rectified stuff, which is high frequency
    // associated

    // saturate to prevent explosion
    if (inputSampleL > 4.0)
      inputSampleL = 4.0;
    if (inputSampleL < -4.0)
      inputSampleL = -4.0;
    if (inputSampleR > 4.0)
      inputSampleR = 4.0;
    if (inputSampleR < -4.0)
      inputSampleR = -4.0;

    iirHeadBumpL += (inputSampleL * 0.05);
    iirHeadBumpL -= (iirHeadBumpL * iirHeadBumpL * iirHeadBumpL * 0.002);
    iirHeadBumpL = sin(iirHeadBumpL);
    inputSampleL -= (iirHeadBumpL * 0.5);

    iirHeadBumpR += (inputSampleR * 0.05);
    iirHeadBumpR -= (iirHeadBumpR * iirHeadBumpR * iirHeadBumpR * 0.002);
    iirHeadBumpR = sin(iirHeadBumpR);
    inputSampleR -= (iirHeadBumpR * 0.5);
    // highpass/EQ

    iirSampleA = (iirSampleA * 0.999) + (inputSampleL * 0.001);
    inputSampleL -= iirSampleA;
    iirSampleB = (iirSampleB * 0.999) + (inputSampleR * 0.001);
    inputSampleR -= iirSampleB;
    // highpass

    if (inputgain != 1.0) {
      inputSampleL /= inputgain;
      inputSampleR /= inputgain;
    }

    // SANITY CHECK
    if (std::isnan(inputSampleL))
      inputSampleL = 0.0;
    if (std::isnan(inputSampleR))
      inputSampleR = 0.0;

    // begin 32 bit stereo floating point dither
    int expon;
    frexpf((float)inputSampleL, &expon);
    fpdL ^= fpdL << 13;
    fpdL ^= fpdL >> 17;
    fpdL ^= fpdL << 5;
    inputSampleL +=
        ((double(fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
    frexpf((float)inputSampleR, &expon);
    fpdR ^= fpdR << 13;
    fpdR ^= fpdR >> 17;
    fpdR ^= fpdR << 5;
    inputSampleR +=
        ((double(fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
    // end 32 bit stereo floating point dither

    *out1 = inputSampleL;
    *out2 = inputSampleR;

    *in1++;
    *in2++;
    *out1++;
    *out2++;
  }
}

void Tope::processDoubleReplacing(double **inputs, double **outputs,
                                  VstInt32 sampleFrames) {
  double *in1 = inputs[0];
  double *in2 = inputs[1];
  double *out1 = outputs[0];
  double *out2 = outputs[1];

  double overallscale = 1.0;
  overallscale /= 44100.0;
  overallscale *= getSampleRate();

  double inputgain = pow(10.0, ((A * 24.0) - 12.0) / 20.0);
  double compfactor = 0.012 * (A / 135.0);
  double range = pow(B, 3) * 25;
  if (range < 0.01)
    range = 0.01;
  // high numbers are shorter, tighter processing

  while (--sampleFrames >= 0) {
    double inputSampleL = *in1;
    double inputSampleR = *in2;
    if (fabs(inputSampleL) < 1.18e-23)
      inputSampleL = fpdL * 1.18e-17;
    if (fabs(inputSampleR) < 1.18e-23)
      inputSampleR = fpdR * 1.18e-17;

    if (inputgain != 1.0) {
      inputSampleL *= inputgain;
      inputSampleR *= inputgain;
    }

    double bridgerectifierL = fabs(inputSampleL);
    if (bridgerectifierL > 1.2533141373155)
      bridgerectifierL = 1.2533141373155;
    // clip infinite input to reach the saturation logic
    if (bridgerectifierL > 1.0)
      bridgerectifierL = sin(bridgerectifierL);
    else
      bridgerectifierL = 1 - cos(bridgerectifierL);
    // produce the interaction that will cut back the treble

    double bridgerectifierR = fabs(inputSampleR);
    if (bridgerectifierR > 1.2533141373155)
      bridgerectifierR = 1.2533141373155;
    // clip infinite input to reach the saturation logic
    if (bridgerectifierR > 1.0)
      bridgerectifierR = sin(bridgerectifierR);
    else
      bridgerectifierR = 1 - cos(bridgerectifierR);
    // produce the interaction that will cut back the treble

    if (inputSampleL > 0)
      inputSampleL -= (bridgerectifierL * compfactor);
    else
      inputSampleL += (bridgerectifierL * compfactor);

    if (inputSampleR > 0)
      inputSampleR -= (bridgerectifierR * compfactor);
    else
      inputSampleR += (bridgerectifierR * compfactor);
    // inverse gain for high frequencies

    iirMidRolloffL = (iirMidRolloffL * (1.0 - range)) + (inputSampleL * range);
    inputSampleL = iirMidRolloffL;
    iirMidRolloffR = (iirMidRolloffR * (1.0 - range)) + (inputSampleR * range);
    inputSampleR = iirMidRolloffR;
    // slew limit against the rectified stuff, which is high frequency
    // associated

    iirHeadBumpL += (inputSampleL * 0.05);
    iirHeadBumpL -= (iirHeadBumpL * iirHeadBumpL * iirHeadBumpL * 0.002);
    iirHeadBumpL = sin(iirHeadBumpL);
    inputSampleL -= (iirHeadBumpL * 0.5);

    iirHeadBumpR += (inputSampleR * 0.05);
    iirHeadBumpR -= (iirHeadBumpR * iirHeadBumpR * iirHeadBumpR * 0.002);
    iirHeadBumpR = sin(iirHeadBumpR);
    inputSampleR -= (iirHeadBumpR * 0.5);
    // highpass/EQ

    iirSampleA = (iirSampleA * 0.999) + (inputSampleL * 0.001);
    inputSampleL -= iirSampleA;
    iirSampleB = (iirSampleB * 0.999) + (inputSampleR * 0.001);
    inputSampleR -= iirSampleB;
    // highpass

    if (inputgain != 1.0) {
      inputSampleL /= inputgain;
      inputSampleR /= inputgain;
    }

    // begin 64 bit stereo floating point dither
    // int expon; frexp((double)inputSampleL, &expon);
    fpdL ^= fpdL << 13;
    fpdL ^= fpdL >> 17;
    fpdL ^= fpdL << 5;
    // inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l *
    // pow(2,expon+62)); frexp((double)inputSampleR, &expon);
    fpdR ^= fpdR << 13;
    fpdR ^= fpdR >> 17;
    fpdR ^= fpdR << 5;
    // inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l *
    // pow(2,expon+62)); end 64 bit stereo floating point dither

    *out1 = inputSampleL;
    *out2 = inputSampleR;

    *in1++;
    *in2++;
    *out1++;
    *out2++;
  }
}
} // end namespace airwindows
