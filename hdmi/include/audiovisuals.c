#include "audiovisuals.h"
#include "hdmi.h"
#include "spectrogram.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

audioInfo *waveform;
uint32_t *background;
uint32_t *spectrogram;
int useSpectrogram = 0;

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
void initWaveform(char* filename, char *ip, char* num_users, uint32_t *samples, int len, long sampleRate, uint32_t waveformColor, uint32_t textColor, uint32_t backgroundColor)
{
    if(!background) //Only realloc if its null
    {
        waveform = (audioInfo *)malloc(sizeof(audioInfo));
        background = (uint32_t *)malloc(sizeof(uint32_t) * getwidth() * getheight());
        useSpectrogram = initSpectrograph();
    }
    waveform->ip = ip;
    waveform->numUsers = num_users;
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
    waveform->w = w;
    waveform->h = h;
    waveform->ssx = 3*w/40;
    waveform->sex = 42*w/40;
    waveform->ssy = 2*(h/5);
    waveform->sey = 8*(h/8);
    waveform->sw = waveform->sex - waveform->ssx;
    waveform->sh = waveform->sey - waveform->ssy;
    if(useSpectrogram == 0)
    {
        waveform->wfsx = w/5;
        waveform->wfex = 4*w/5;
        waveform->wfsy = (h/5);
        waveform->wfey = 2*(h/5);
        
        if(!spectrogram)
            spectrogram = (uint32_t *)malloc(sizeof(uint32_t) * waveform->sw * waveform->sh);
    }
    else
    {
        waveform->wfsx = w/5;
        waveform->wfex = 4*w/5;
        waveform->wfsy = (h/5);
        waveform->wfey = 4*(h/5);
    }

    
}

/* Helper function to concatenate strings */
char* concat(const char *s1, const char *s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    
    char *result = malloc(len1 + len2 + 1); // +1 for the null-terminator
    
    if (result == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    
    strcpy(result, s1);
    strcat(result, s2);
    
    return result;
}

void write_uint32_array_to_file(const char *filename, uint32_t *array, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    size_t elements_written = fwrite(array, sizeof(uint32_t), size, file);
    if (elements_written != size) {
        fprintf(stderr, "Error writing to file\n");
    }

    fclose(file);
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

    /* Draw filename */
    char *fn = concat("Filename: ", waveform->filename);
    int fnlen = strlen(fn);
    int fontSize = (3*w/7)/(fnlen * 8);
    drawString(fn, w/20, h/20, fontSize, waveform->text_color);

    /* Draw Sample Rate */
    char sr[100];
    sprintf(sr, "%ld", waveform->sampleRate);
    fn = concat("Sample Rate: ", sr);
    drawString(fn, w/20, h/20 + fontSize*8, fontSize, waveform->text_color);

    /* Draw Duration */
    sprintf(sr, "%.2f", waveform->duration);
    fn = concat("Duration: ", sr);
    fn = concat(fn, "s");
    drawString(fn, w/20, h/20 + 2* fontSize*8, fontSize, waveform->text_color);

    /* Draw progress bar */
    //TBD If wanted, since cursors act as progress bar

    /* Draw the spectrogram */
    if(useSpectrogram == 0)
    {
        drawStringCentered("Loading Spectrogram", (waveform->wfex - waveform->wfsx)/2 + waveform->wfsx, (waveform->sey - waveform->ssy)/2 + waveform->ssy, fontSize, waveform->text_color);
        printf("Getting spectrogram....\n");
        //getSpectrogram(waveform->samples, waveform->sampleRate, waveform->len, spectrogram, waveform->sw, waveform->sh, waveform->background_color);
        getSpectrogram(waveform->filename, spectrogram, waveform->sw, waveform->sh, waveform->background_color);
        //printf("%d\n", (waveform->sey - waveform->ssy) * (waveform->sex - waveform->ssx));
        long c = 0;//376992/2;
        startPixelBulkDraw();
        for(int j = waveform->ssy ; j < waveform->sey; j++)
        {
            for(int i = waveform->ssx; i < waveform->sex; i++)
            {
                //printf("0x%X\n", spectrogram[c]);
                setPixelBulk(i, j, (spectrogram[c]));
                if(spectrogram[c] == 0x0)
                {
                    //printf("%ld\n", c);
                }
                c++;
            }
        }
        endPixelBulkDraw();
        printf("Finished spectrogram\n");
    }
    else{
        int sheight = fontSize * 8;
        drawStringCentered(waveform->ip, (waveform->wfex - waveform->wfsx)/2 + waveform->wfsx, (waveform->h - waveform->wfey)/2 + waveform->wfey - sheight/2, fontSize, waveform->text_color);
        drawStringCentered(waveform->numUsers, (waveform->wfex - waveform->wfsx)/2 + waveform->wfsx, (waveform->h - waveform->wfey)/2 + waveform->wfey + sheight/2, fontSize, waveform->text_color);
    }

    /* Copy background */
    getBuffer(background);

    /* Write to server file */
    write_uint32_array_to_file("http-server/web_root/hdmi.dat", background, waveform->w * waveform->h);

    /* Draw cursors */
    updateCursor(waveform->lcursor, waveform->rcursor, waveform->cursor);
}

/* Helper function to convert sample number to cursor position */
int cursorToX(int cursor)
{   
    int ans = map(cursor, 0, waveform->len, waveform->wfsx, waveform->wfex);
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
            drawRectangleFromBufferBulk(cursorToX(waveform->lcursor)-2, waveform->wfsy, cursorToX(waveform->lcursor)+2, waveform->wfey, background);
        }
        
        if(rcursor != -1)
        {
            drawRectangleFromBufferBulk(cursorToX(waveform->rcursor)-2, waveform->wfsy, cursorToX(waveform->rcursor)+2, waveform->wfey, background);
        }
        
        if(cursor != -1)
        {
            drawRectangleFromBufferBulk(cursorToX(waveform->cursor)-2, waveform->wfsy, cursorToX(waveform->cursor)+2, waveform->wfey, background);
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
    if(useSpectrogram == 0)
    {
        stopSpectrogram();
        free(spectrogram);
    }
    free(background);

    printf("Stopped Audio Visualization\n");

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
    drawRectangleBulk(waveform->wfex - 4, waveform->wfsy, waveform->wfex, waveform->wfey, 0x0);
    drawRectangleBulk(waveform->wfsx, waveform->wfsy, waveform->wfsx + 4, waveform->wfey, 0x0);
}

int getSampleDifference(int sx, int ex, int numSamples)
{
    double sampleWidth = (double)(ex-sx)/(double)numSamples;
    int skip = (int)(1/sampleWidth); //Used for scaling/graphing samples
    return skip;
}

int getScreenWidth()
{
    return getwidth();
}

int getScreenHeight()
{
    return getheight();
}

/* Get the current HDMI buffer */
void getHDMIBuffer(uint32_t *output)
{
    getBuffer(output);
}