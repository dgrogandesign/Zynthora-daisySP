#pragma once

#include <algorithm>
#include <stdint.h>

// --- Korg Logue SDK Utils Shim ---

// clamp(val, min, max) for 32-bit integers
static inline int32_t clipminmaxi32(int32_t x, int32_t min, int32_t max) {
  return std::clamp(x, min, max);
}

// Convert float to Q31 (if needed) but para_saw seemingly uses this header for
// clipminmax mostly
static inline float linintf(float fr, float x0, float x1) {
  return x0 + fr * (x1 - x0);
}
