#ifndef audiovisuals_H
#define audiovisuals_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct audioInfo
{
    char* filename;
    long sampleRate;
    double duration;
    uint32_t *samples;
    int len;
    uint32_t waveform_color;
    uint32_t text_color;
    uint32_t background_color;
    int lcursor;
    int rcursor;
    int cursor;
    int wfsx;
    int wfex;
    int wfsy;
    int wfey;
    int ssx;
    int sex;
    int ssy;
    int sey;
    int sw;
    int sh;
    char* ip;
    char* numUsers;
    int w;
    int h;
} audioInfo;

/* Initialize the audio visualization */
void initAudioVisualization();

/* Initialize the waveform drawing */
void initWaveform(char* filename, char *ip, char* num_users, uint32_t *samples, int len, long sampleRate, uint32_t waveformColor, uint32_t textColor, uint32_t backgroundColor);

/* Draws the entire audio visualization screen, expensive and slow. Only use for major updates*/
void drawWholeScreen();

/* Updates all of the cursors positions, setting cursor to -1 will not draw it */
void updateCursor(int lcursor, int rcursor, int cursor);

/* Close the audio visualization */
void stopAudioVisualization();

/* Draw waveform to screen */
void drawWaveform(int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color);

/* Draw part of a waveform to screen */
void drawPartialWaveform(int snum_start, int snum_end, int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color);

/* Draw part of a waveform to screen in bulk draw mode*/
void drawPartialWaveformBulk(int snum_start, int snum_end, int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color);

/* Draw waveform border to screen */
void drawWaveformBorderBulk();

/* Get the amount of samples that correspond to each pixel */
int getSampleDifference(int sx, int ex, int numSamples);

/* Get the current HDMI buffer */
void getHDMIBuffer(uint32_t *output);

int getScreenWidth();

int getScreenHeight();

#endif