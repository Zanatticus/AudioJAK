#include "visualizer.h"
#include "audiovisuals.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_t changethread;

uint32_t bcolor, tcolor, wcolor;

uint32_t *_buffer;
char* _wavefilename;
int _len;
int _sampleRate;

int lcursor, rcursor, cursor;

//1 for cursor change, 2 for waveform change
volatile int change = 0;
volatile int stopTheVisuals = 0;

/* Thread function to constantly check for changes, but doesn't block when making these changes */
void *changeThread(void *arg)
{
    while(!stopTheVisuals)
    {
        if(change == 1)
        {
            updateCursor(cursor, lcursor, rcursor);
            change = 0;
        }
        else if(cursor == 2)
        {
            initWaveform(_wavefilename, _buffer, _len, _sampleRate, wcolor, tcolor, bcolor);
            drawWholeScreen();
        }
    }

    pthread_exit(NULL);
}

/* Initialize the visuals */
void initVisuals(char* filename, uint32_t **samples, int len, int sampleRate, uint32_t waveformColor, uint32_t textColor, uint32_t backgroundColor)
{
    initAudioVisualization();
    initWaveform(filename, *samples, len, sampleRate, waveformColor, textColor, backgroundColor);
    wcolor = waveformColor;
    tcolor = textColor;
    bcolor = backgroundColor;
    _wavefilename = filename;
    _buffer = *samples;
    _len = len;
    drawWholeScreen();

    if (pthread_create(&changethread, NULL, changeThread, NULL) != 0) {
        perror("Error creating change thread");
    }

}

/* Stop visuals */
void stopVisuals()
{
    stopTheVisuals = 1;
    stopAudioVisualization();
    usleep(1000000);
}

/* Updates the cursor values */
void updateCursorValues(int cur, int lcur, int rcur)
{
    //while(change != 0){ /* Wait for change */}
    cursor = cur;
    lcursor = lcur;
    rcursor = rcur;
    change = 1;
}

/* Updates the waveform with new samples */
void updateWaveform(uint32_t **samples, int len, int sampleRate)
{
    //while(change != 0){ /* Wait for change */}
    _buffer = *samples;
    _len = len;
    _sampleRate = sampleRate;
    change = 2;
}