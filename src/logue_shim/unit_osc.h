#pragma once

#include <stddef.h>
#include <stdint.h>

// --- Validation Macros ---
#define UNIT_API_IS_COMPAT(x) (1)

// --- Error Codes ---
enum {
  k_unit_err_none = 0,
  k_unit_err_undef = 1,
  k_unit_err_target = 2,
  k_unit_err_api_version = 3,
  k_unit_err_samplerate = 4,
  k_unit_err_geometry = 5
};

// --- Runtime Descriptor ---
typedef struct unit_runtime_desc {
  uint32_t target;
  uint32_t api;
  uint32_t samplerate;
  uint32_t input_channels;
  uint32_t output_channels;
  void *hooks;
} unit_runtime_desc_t;

// --- Mock Unit Header ---
struct UnitHeader {
  uint32_t target;
  uint32_t api;
};

// Global instance to match against
static const UnitHeader unit_header = {0, 0};
