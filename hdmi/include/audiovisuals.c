#include "audiovisuals.h"
#include "hdmi.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* Initialize the audio visualization */
void initAudioVisualization()
{
    inithdmi();
}

/* Close the audio visualization */
void stopAudioVisualization()
{
    closehdmi();
}

/* Helper function to map number into range */
int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* Draw waveform to screen */
void drawWaveform(int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color)
{
    double sampleWidth = (double)(ex-sx)/(double)len;
    int skip = (int)(1/sampleWidth); //Used for scaling/graphing samples

    int8_t dsamps[len];
    startPixelBulkDraw();
    int xpixel = 0;
    for(int snum = 0; snum < len - 1; snum += skip)
    {
        dsamps[snum] = (((int8_t)(samples[snum] >> 24)));
        //printf("Sample: %d, %d/%d\n", dsamps[snum], snum, len);
        int h = map(dsamps[snum], -128, 127, sy, ey);
        if(dsamps[snum] < 0)
        {
            drawRectangleBulk(sx + xpixel, h, sx + xpixel + 1, sy + (ey-sy)/2, color);
        }
        else{
            drawRectangleBulk(sx + xpixel, sy + (ey-sy)/2, sx + xpixel + 1, h, color);
        }
        xpixel += 1;
    }
    endPixelBulkDraw();
}

int getScreenWidth()
{
    return getwidth();
}

int getScreenHeight()
{
    return getheight();
}