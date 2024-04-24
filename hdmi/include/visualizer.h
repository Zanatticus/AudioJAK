#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Initialize the visuals */
void initVisuals(char* filename, char* ip, char* numUsers, uint32_t **samples, int len, int sampleRate, uint32_t waveformColor, uint32_t textColor, uint32_t backgroundColor);

/* Stop visuals */
void stopVisuals();

/* Updates the cursor values */
void updateCursorValues(int cursor, int lcursor, int rcursor);

/* Updates the waveform with new samples */
void updateWaveform(char* newName, uint32_t **samples, int len, int sampleRate, char *ip, char *numUsers);

#endif