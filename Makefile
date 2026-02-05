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
	-IDaisySP/DaisySP-LGPL/Source/Filters

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

# Main Sources
SRCS = src/main.cpp mongoose.c $(DAISY_SRCS) $(DAISY_LGPL_SRCS)

all: zynthora

zynthora: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o zynthora $(LIBS)

clean:
	rm -f zynthora
