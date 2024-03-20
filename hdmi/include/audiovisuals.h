#ifndef audiovisuals_H
#define audiovisuals_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Initialize the audio visualization */
void initAudioVisualization();

/* Close the audio visualization */
void stopAudioVisualization();

/* Draw waveform to screen */
void drawWaveform(int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color);

int getScreenWidth();

int getScreenHeight();

#endif