#ifndef recording_h
#define recording__h

#include "global.h"
#include "config.h"


void writeWavHeader(File &file, int sampleRate);

void i2s_adc();

String audioTranscription();

#endif