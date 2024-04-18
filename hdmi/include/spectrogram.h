#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Initialize the spectrogram */
void initSpectrograph();

/* Get the pixels of the spectrogram */
//void getSpectrogram(uint32_t *samples, int sampleRate, int len, uint32_t *output, int w, int h, uint32_t backgroundColor);
void getSpectrogram(char* filename, uint32_t *output, int w, int h, uint32_t backgroundColor);

/* Stops spectrogram */
void stopSpectrogram();

#endif