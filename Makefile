CC = g++

# Include all subdirectories of DaisySP/Source and DaisySP-LGPL/Source
INCLUDES = -I. -Isrc \
	-IDaisySP/Source \
	-IDaisySP/Source/Control \
	-IDaisySP/Source/Drums \
	-IDaisySP/Source/Dynamics \
	-IDaisySP/Source/Effects \
	-IDaisySP/Source/Filters \
	-IDaisySP/Source/Noise \
	-IDaisySP/Source/PhysicalModeling \
	-IDaisySP/Source/Sampling \
	-IDaisySP/Source/Synthesis \
	-IDaisySP/Source/Utility \
	-IDaisySP/DaisySP-LGPL/Source \
	-IDaisySP/DaisySP-LGPL/Source/Effects \
	-IDaisySP/DaisySP-LGPL/Source/Filters \
	-Isrc/logue_shim \
	-Ilogue_units/para_saw \
	-Isrc/braids \
	-Isrc/braids/stmlib

CFLAGS = $(INCLUDES) -O3 -Wall -D__LINUX_ALSA__
LIBS = -lpthread -ldl -lm

# DaisySP Sources (Main Library)
DAISY_SRCS = \
	DaisySP/Source/Control/adenv.cpp \
	DaisySP/Source/Control/adsr.cpp \
	DaisySP/Source/Control/phasor.cpp \
	DaisySP/Source/Effects/flanger.cpp \
	DaisySP/Source/Effects/overdrive.cpp \
	DaisySP/Source/Effects/chorus.cpp \
	DaisySP/Source/Filters/svf.cpp \
	DaisySP/Source/Synthesis/oscillator.cpp \
	DaisySP/Source/Synthesis/variablesawosc.cpp \
	DaisySP/Source/Synthesis/fm2.cpp \
	DaisySP/Source/Utility/metro.cpp \
	DaisySP/Source/Utility/dcblock.cpp

# DaisySP LGPL Sources (Reverb, Moog Filter)
DAISY_LGPL_SRCS = \
	DaisySP/DaisySP-LGPL/Source/Effects/reverbsc.cpp \
	DaisySP/DaisySP-LGPL/Source/Filters/moogladder.cpp

# Braids Sources
BRAIDS_SRCS = \
	src/braids/analog_oscillator.cc \
	src/braids/digital_oscillator.cc \
	src/braids/macro_oscillator.cc \
	src/braids/quantizer.cc \
	src/braids/resources.cc \
	src/braids/stmlib/utils/random.cc \
	src/braids/stmlib/dsp/atan.cc \
	src/braids/stmlib/dsp/units.cc

# Main Sources
SRCS = src/main.cpp src/braids_wrapper.cpp mongoose.c $(DAISY_SRCS) $(DAISY_LGPL_SRCS) $(BRAIDS_SRCS)

all: zynthora

zynthora: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o zynthora $(LIBS)

clean:
	rm -f zynthora
