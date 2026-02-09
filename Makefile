CC = g++
CFLAGS = -I. -IDaisySP/Source -IDaisySP/Source/Control -IDaisySP/Source/Drums -IDaisySP/Source/Dynamics -IDaisySP/Source/Effects -IDaisySP/Source/Filters -IDaisySP/Source/Noise -IDaisySP/Source/PhysicalModeling -IDaisySP/Source/Sampling -IDaisySP/Source/Synthesis -IDaisySP/Source/Utility -IDaisySP/DaisySP-LGPL/Source -IDaisySP/DaisySP-LGPL/Source/Effects -IDaisySP/DaisySP-LGPL/Source/Filters -O2 -Wall -D__LINUX_ALSA__ -pipe --param ggc-min-expand=10 --param ggc-min-heapsize=8192
LIBS = -lpthread -ldl -lm -lasound

SRCS_CPP = main.cpp \
	DaisySP/Source/Control/adenv.cpp \
	DaisySP/Source/Control/adsr.cpp \
	DaisySP/Source/Control/phasor.cpp \
	DaisySP/Source/Effects/flanger.cpp \
	DaisySP/Source/Effects/overdrive.cpp \
	DaisySP/Source/Effects/chorus.cpp \
	DaisySP/Source/Filters/svf.cpp \
	DaisySP/Source/Synthesis/oscillator.cpp \
	DaisySP/Source/Synthesis/variablesawosc.cpp \
	DaisySP/Source/Utility/metro.cpp \
	DaisySP/Source/Utility/dcblock.cpp \
	DaisySP/DaisySP-LGPL/Source/Effects/reverbsc.cpp \
	DaisySP/DaisySP-LGPL/Source/Filters/moogladder.cpp

SRCS_C = mongoose.c

OBJS = $(SRCS_CPP:.cpp=.o) $(SRCS_C:.c=.o)

all: zynthora

zynthora: $(OBJS)
	$(CC) $(OBJS) -o zynthora $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f zynthora $(OBJS)
