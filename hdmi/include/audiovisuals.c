#include "audiovisuals.h"
#include "hdmi.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

audioInfo *waveform;

/* Helper function to map number into range */
int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* Initialize the audio visualization */
void initAudioVisualization()
{
    inithdmi();
}

/* Initialize the waveform drawing */
void initWaveform(char* filename, uint32_t *samples, int len, long sampleRate, uint32_t waveformColor, uint32_t textColor, uint32_t backgroundColor)
{
    waveform = (audioInfo *)malloc(sizeof(audioInfo));
    waveform->sampleRate = sampleRate;
    waveform->filename = filename;
    waveform->len = len;
    waveform->waveform_color = waveformColor;
    waveform->text_color = textColor;
    waveform->background_color = backgroundColor;
    waveform->samples = samples;
    waveform->duration = len / (double)sampleRate;
    waveform->cursor = 0;
    waveform->lcursor = -1;
    waveform->rcursor = -1;
    int w = getwidth();
    int h = getheight();
    waveform->wfsx = w/5;
    waveform->wfex = 4*w/5;
    waveform->wfsy = (h/5);
    waveform->wfey = 4*(h/5);
    printf("SX: %d, EX: %d\n", waveform->wfsx, waveform->wfex);
}

/* Draws the entire audio visualization screen, expensive and slow. Only use for major updates*/
void drawWholeScreen()
{
    int w = getwidth();
    int h = getheight();
    /* Draw background */
    drawRectangle(0,0,w,h, waveform->background_color);
    /* Draw waveform */
    //drawRectangle(waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfey, 0xFFFFFF);
    drawWaveform(waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfey, waveform->samples, waveform->len, waveform->waveform_color);

    drawWaveformBorderBulk();

    /* Draw cursors */
    updateCursor(waveform->lcursor, waveform->rcursor, waveform->cursor);

    /* Draw filename information */

    /* Draw progress bar */
}

/* Helper function to convert sample number to cursor position */
int cursorToX(int cursor)
{   
    int ans = map(cursor, 0, waveform->len, waveform->wfsx, waveform->wfex);
    //drawRectangleBulk(0, 0, getwidth(), waveform->wfsy, 0xFFFFFF);
    //drawRectangleBulk(ans - 1, 0, ans + 1, waveform->wfsy, 0xFF0000);
    //printf("cursor: %d (%d/%d), range:(%d, %d), answer: %d\n", cursor, 0, waveform->len, waveform->wfsx, waveform->wfex, ans);
    return ans;
}

/* Updates all of the cursors positions, setting cursor to -1 will not draw it */
void updateCursor(int lcursor, int rcursor, int cursor)
{
    //Only draw if there is more than a pixel change in the cursor position
    double sampleWidth = (double)(waveform->wfex-waveform->wfsx)/(double)waveform->len;
    int skip = (int)(1/sampleWidth); //Used for scaling/graphing samples
    if(fabs(lcursor - waveform->lcursor) >= skip || fabs(rcursor - waveform->rcursor) >= skip || fabs(cursor - waveform->cursor) >= skip)
    {
        startPixelBulkDraw();

        /* First you need to redraw the part of the waveform that was covered by the previous cursor */
        if(lcursor != -1)
        {
            drawPartialWaveformBulk(waveform->lcursor - 1000, waveform->lcursor, waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfey, waveform->samples, waveform->len, waveform->waveform_color);
        }
        
        if(rcursor != -1)
        {
            drawPartialWaveformBulk(waveform->rcursor - 1000, waveform->rcursor, waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfey, waveform->samples, waveform->len, waveform->waveform_color);
        }
        
        if(cursor != -1)
        {
            drawPartialWaveformBulk(waveform->cursor - 1000, waveform->cursor, waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfey, waveform->samples, waveform->len, waveform->waveform_color);
        }

        /* Now, redraw the cursors */
        if(lcursor != -1)
        {
            drawRectangleBulk(cursorToX(lcursor)-2, waveform->wfsy, cursorToX(lcursor)+2, waveform->wfey, 0x0000FF);
        }
        
        if(rcursor != -1)
        {
            drawRectangleBulk(cursorToX(rcursor)-2, waveform->wfsy, cursorToX(rcursor)+2, waveform->wfey, 0xFF0000);
        }
        
        if(cursor != -1)
        {
            drawRectangleBulk(cursorToX(cursor)-2, waveform->wfsy, cursorToX(cursor)+2, waveform->wfey, 0x0);
        }

        //drawWaveformBorderBulk();

        endPixelBulkDraw();

        waveform->lcursor = lcursor;
        waveform->rcursor = rcursor;
        waveform->cursor = cursor;
    }
}

/* Close the audio visualization */
void stopAudioVisualization()
{
    closehdmi();
}

/* Draw part of a waveform to screen */
void drawPartialWaveform(int snum_start, int snum_end, int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color)
{
    double sampleWidth = (double)(ex-sx)/(double)len;
    int skip = (int)(1/sampleWidth); //Used for scaling/graphing samples

    int8_t dsamps[len];
    startPixelBulkDraw();
    int start = (snum_start < 0) ? 0 : snum_start;
    int xpixel = cursorToX(start);
    int end = (snum_end >= len - 1000) ? len - 1 : snum_end;
    for(int snum = start; snum < end; snum += skip)
    {
        dsamps[snum] = (((int8_t)(samples[snum] >> 24)));
        int h = map(dsamps[snum], -128, 127, sy, ey);
        drawRectangleBulk(sx + xpixel, sy, sx + xpixel + 1, ey, waveform->background_color);
        if(dsamps[snum] < 0)
        {
            drawRectangleBulk(sx + xpixel, h, sx + xpixel + 1, sy + (ey-sy)/2, color);
        }
        else
        {
            drawRectangleBulk(sx + xpixel, sy + (ey-sy)/2, sx + xpixel + 1, h, color);
        }
        xpixel += 1;
    }
    endPixelBulkDraw();
}

/* Draw part of a waveform to screen in bulk draw mode*/
void drawPartialWaveformBulk(int snum_start, int snum_end, int sx, int sy, int ex, int ey, uint32_t *samples, int len, uint32_t color)
{
    double sampleWidth = (double)(ex-sx)/(double)len;
    int skip = (int)(1/sampleWidth); //Used for scaling/graphing samples

    int8_t dsamps[len];
    int start = (snum_start < 0) ? 0 : (snum_start / skip) * skip;
    int xpixel = cursorToX(start) - waveform->wfsx;
    int end = (snum_end >= len - 1000) ? len - 1: snum_end;
    for(int snum = start; snum < end; snum += skip)
    {
        dsamps[snum] = (((int8_t)(samples[snum] >> 24)));
        //printf("Sample: %d, %d/%d\n", dsamps[snum], snum, len);
        int h = map(dsamps[snum], -128, 127, sy, ey);
        drawRectangleBulk(sx + xpixel, sy, sx + xpixel + 1, ey, waveform->background_color);
        if(dsamps[snum] < 0)
        {
            drawRectangleBulk(sx + xpixel, h, sx + xpixel + 1, sy + (ey-sy)/2, color);
        }
        else
        {
            drawRectangleBulk(sx + xpixel, sy + (ey-sy)/2, sx + xpixel + 1, h, color);
        }
        xpixel += 1;
    }
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

/* Draw waveform border to screen */
void drawWaveformBorderBulk()
{
    //drawRectangleBulk(waveform->wfsx, waveform->wfsy, waveform->wfex, waveform->wfsy+4, 0x0);
    //if(cursorToX(waveform->lcursor) < waveform->wfsx + 4)
        drawRectangleBulk(waveform->wfex - 4, waveform->wfsy, waveform->wfex, waveform->wfey, 0x0);
    //drawRectangleBulk(waveform->wfsx, waveform->wfey-4, waveform->wfex, waveform->wfey, 0x0);
    drawRectangleBulk(waveform->wfsx, waveform->wfsy, waveform->wfsx + 4, waveform->wfey, 0x0);
}

int getScreenWidth()
{
    return getwidth();
}

int getScreenHeight()
{
    return getheight();
}