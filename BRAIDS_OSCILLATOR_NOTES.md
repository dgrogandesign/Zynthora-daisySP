# Braids Oscillator Integration Notes

This document summarizes the integration of the Mutable Instruments Braids oscillator into the Zynthora-DSP project. Use this as a reference for the current implementation and parameter mappings.

## Technical Overview
- **Core Engine:** Mutable Instruments Braids (`MacroOscillator` class).
- **Wrapper:** `BraidsWrapper` (located in `src/braids_wrapper.cpp` and `src/braids_wrapper.h`).
- **Waveform ID:** `WAVE_BRAIDS` (103).
- **Processing:** Block-based (24 samples per block).

## Parameter Mapping (7 Knobs)

| UI Label | Parameter | Behavior | Usually (Hardware) |
| :--- | :--- | :--- | :--- |
| **EDIT (MODEL)** | `SetModel` | Selects one of the 48 Braids algorithms. | Model Selection |
| **TIMBRE** | `timbre_` | Main morphing/spectral parameter. | Timbre Pot |
| **COLOR** | `color_` | Secondary morphing parameter. | Color Pot |
| **FINE** | `fine_` | Fine tuning (+/- 1 semitone). | Fine Tune Pot |
| **COARSE** | `coarse_` | Transposition (+/- 2 octaves). | Coarse Tune Pot |
| **FM** | `fm_amount_` | **Modulation Depth:** Controls an internal **6Hz Sine LFO** modulating **Pitch**. | External CV Pitch Input |
| **MOD** | `mod_amount_` | **Modulation Depth:** Controls an internal **6Hz Sine LFO** modulating **Timbre**. | External CV Timbre Input |

## Internal Modulation Details
To breathe life into the sound without external CV inputs, the following modulations are hard-coded in the `BraidsWrapper::Process()` loop:

1.  **Pitch Modulation (FM Knob):**
    - Range: Up to +/- 1 octave.
    - Rate: Fixed at 6.0 Hz.
2.  **Timbre Modulation (MOD Knob):**
    - Range: Up to 50% of the Timbre range.
    - Rate: Fixed at 6.0 Hz.

## Tips for Sound Design
- **Metallic FM Sounds:** While the "FM" knob is a simple vibrato in this wrapper, you can get classic "metallic" FM by selecting the dedicated FM models via the **EDIT** knob. Look for models around indices **25 (FM)**, **26 (Feedback FM)**, and **27 (Chaotic FM)**.
- **Static vs. Moving:** If you want a dry, non-wobbly sound, keep **FM** and **MOD** at 0.
- **Tuning:** Use **Coarse** for octave shifts and **Fine** to tune against other oscillators.

## Git Info
- **Branch:** `feat/braids`
- **Remote:** `origin/feat/braids`
- **Last Status:** All changes committed and pushed to GitHub.
