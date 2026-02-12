#!/bin/bash
set -e

FLAGS="-I. -Isrc -IDaisySP/Source -IDaisySP/Source/Control -IDaisySP/Source/Drums -IDaisySP/Source/Dynamics -IDaisySP/Source/Effects -IDaisySP/Source/Filters -IDaisySP/Source/Noise -IDaisySP/Source/PhysicalModeling -IDaisySP/Source/Sampling -IDaisySP/Source/Synthesis -IDaisySP/Source/Utility -IDaisySP/DaisySP-LGPL/Source -IDaisySP/DaisySP-LGPL/Source/Effects -IDaisySP/DaisySP-LGPL/Source/Filters -Isrc/logue_shim -Ilogue_units/para_saw -Isrc/braids -Isrc/braids/stmlib -Isrc/effects/airwindows -O1 -Wall -D__LINUX_ALSA__ -DTEST"

echo "Compiling DaisySP..."
g++ -c $FLAGS DaisySP/Source/Control/adenv.cpp -o adenv.o
g++ -c $FLAGS DaisySP/Source/Control/adsr.cpp -o adsr.o
g++ -c $FLAGS DaisySP/Source/Control/phasor.cpp -o phasor.o
g++ -c $FLAGS DaisySP/Source/Effects/flanger.cpp -o flanger.o
g++ -c $FLAGS DaisySP/Source/Effects/overdrive.cpp -o overdrive.o
g++ -c $FLAGS DaisySP/Source/Effects/chorus.cpp -o chorus.o
g++ -c $FLAGS DaisySP/Source/Filters/svf.cpp -o svf.o
g++ -c $FLAGS DaisySP/Source/Synthesis/oscillator.cpp -o oscillator.o
g++ -c $FLAGS DaisySP/Source/Synthesis/variablesawosc.cpp -o variablesawosc.o
g++ -c $FLAGS DaisySP/Source/Synthesis/fm2.cpp -o fm2.o
g++ -c $FLAGS DaisySP/Source/Utility/metro.cpp -o metro.o
g++ -c $FLAGS DaisySP/Source/Utility/dcblock.cpp -o dcblock.o
g++ -c $FLAGS DaisySP/DaisySP-LGPL/Source/Effects/reverbsc.cpp -o reverbsc.o
g++ -c $FLAGS DaisySP/DaisySP-LGPL/Source/Filters/moogladder.cpp -o moogladder.o

echo "Compiling Braids..."
g++ -c $FLAGS src/braids/analog_oscillator.cc -o analog_oscillator.o
g++ -c $FLAGS src/braids/digital_oscillator.cc -o digital_oscillator.o
g++ -c $FLAGS src/braids/macro_oscillator.cc -o macro_oscillator.o
g++ -c $FLAGS src/braids/quantizer.cc -o quantizer.o
g++ -c $FLAGS src/braids/resources.cc -o braids_resources.o
g++ -c $FLAGS src/braids/stmlib/utils/random.cc -o random.o
g++ -c $FLAGS src/braids/stmlib/dsp/atan.cc -o atan.o
g++ -c $FLAGS src/braids/stmlib/dsp/units.cc -o units.o

echo "Compiling Plaits..."
g++ -c $FLAGS src/plaits/resources.cc -o plaits_resources.o
g++ -c $FLAGS src/plaits/dsp/voice.cc -o voice.o
g++ -c $FLAGS src/plaits/dsp/engine/additive_engine.cc -o additive_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/bass_drum_engine.cc -o bass_drum_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/chord_engine.cc -o chord_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/fm_engine.cc -o fm_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/grain_engine.cc -o grain_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/hi_hat_engine.cc -o hi_hat_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/modal_engine.cc -o modal_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/noise_engine.cc -o noise_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/particle_engine.cc -o particle_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/snare_drum_engine.cc -o snare_drum_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/speech_engine.cc -o speech_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/string_engine.cc -o string_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/swarm_engine.cc -o swarm_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/virtual_analog_engine.cc -o virtual_analog_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/waveshaping_engine.cc -o waveshaping_engine.o
g++ -c $FLAGS src/plaits/dsp/engine/wavetable_engine.cc -o wavetable_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/chiptune_engine.cc -o chiptune_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/phase_distortion_engine.cc -o phase_distortion_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/six_op_engine.cc -o six_op_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/string_machine_engine.cc -o string_machine_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/virtual_analog_vcf_engine.cc -o virtual_analog_vcf_engine.o
g++ -c $FLAGS src/plaits/dsp/engine2/wave_terrain_engine.cc -o wave_terrain_engine.o
g++ -c $FLAGS src/plaits/dsp/chords/chord_bank.cc -o chord_bank.o
g++ -c $FLAGS src/plaits/dsp/fm/algorithms.cc -o algorithms.o
g++ -c $FLAGS src/plaits/dsp/fm/dx_units.cc -o dx_units.o
g++ -c $FLAGS src/plaits/dsp/physical_modelling/modal_voice.cc -o modal_voice.o
g++ -c $FLAGS src/plaits/dsp/physical_modelling/resonator.cc -o resonator.o
g++ -c $FLAGS src/plaits/dsp/physical_modelling/string.cc -o string.o
g++ -c $FLAGS src/plaits/dsp/physical_modelling/string_voice.cc -o string_voice.o
g++ -c $FLAGS src/plaits/dsp/speech/lpc_speech_synth.cc -o lpc_speech_synth.o
g++ -c $FLAGS src/plaits/dsp/speech/lpc_speech_synth_controller.cc -o lpc_speech_synth_controller.o
g++ -c $FLAGS src/plaits/dsp/speech/lpc_speech_synth_phonemes.cc -o lpc_speech_synth_phonemes.o
g++ -c $FLAGS src/plaits/dsp/speech/lpc_speech_synth_words.cc -o lpc_speech_synth_words.o
g++ -c $FLAGS src/plaits/dsp/speech/naive_speech_synth.cc -o naive_speech_synth.o
g++ -c $FLAGS src/plaits/dsp/speech/sam_speech_synth.cc -o sam_speech_synth.o

echo "Compiling Airwindows..."
g++ -c $FLAGS src/effects/airwindows/AirwindowsWrapper.cpp -o AirwindowsWrapper.o
g++ -c $FLAGS src/effects/airwindows/Galactic.cpp -o Galactic.o
g++ -c $FLAGS src/effects/airwindows/GalacticProc.cpp -o GalacticProc.o
g++ -c $FLAGS src/effects/airwindows/Console7Channel.cpp -o Console7Channel.o
g++ -c $FLAGS src/effects/airwindows/Console7ChannelProc.cpp -o Console7ChannelProc.o

echo "Compiling Main..."
g++ -c $FLAGS mongoose.c -o mongoose.o
g++ -c $FLAGS src/braids_wrapper.cpp -o braids_wrapper.o
g++ -c $FLAGS src/plaits_wrapper.cpp -o plaits_wrapper.o
g++ -c $FLAGS src/main.cpp -o main.o

echo "Linking..."
g++ adenv.o adsr.o phasor.o flanger.o overdrive.o chorus.o svf.o oscillator.o variablesawosc.o fm2.o metro.o dcblock.o reverbsc.o moogladder.o \
    analog_oscillator.o digital_oscillator.o macro_oscillator.o quantizer.o braids_resources.o random.o atan.o units.o \
    plaits_resources.o voice.o additive_engine.o bass_drum_engine.o chord_engine.o fm_engine.o grain_engine.o hi_hat_engine.o modal_engine.o noise_engine.o particle_engine.o snare_drum_engine.o speech_engine.o string_engine.o swarm_engine.o virtual_analog_engine.o waveshaping_engine.o wavetable_engine.o chiptune_engine.o phase_distortion_engine.o six_op_engine.o string_machine_engine.o virtual_analog_vcf_engine.o wave_terrain_engine.o chord_bank.o algorithms.o dx_units.o modal_voice.o resonator.o string.o string_voice.o lpc_speech_synth.o lpc_speech_synth_controller.o lpc_speech_synth_phonemes.o lpc_speech_synth_words.o naive_speech_synth.o sam_speech_synth.o \
    AirwindowsWrapper.o Galactic.o GalacticProc.o Console7Channel.o Console7ChannelProc.o \
    mongoose.o braids_wrapper.o plaits_wrapper.o main.o \
    -o zynthora -lpthread -ldl -lm

echo "Done!"
