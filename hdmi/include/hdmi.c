#include "hdmi.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "font.h"

extern char font8x8_basic[128][8];

int hdmifd;
struct fb_var_screeninfo screeninfo;
uint32_t *hdmidata, *drawbuffer;
uint32_t hdmix, hdmiy, hdmiwidth, hdmiheight;

pthread_t paintthread;
pthread_mutex_t mutex;
uint32_t *buffer;
volatile int stophdmi = 0;

/* Initialize the HDMI */
void inithdmi()
{
    hdmifd = open("/dev/fb0", O_RDWR); //Open /dev/fb0 to draw images to the screen
    if (hdmifd == -1)
    {
        perror("open /dev/fb0");
        exit(EXIT_FAILURE);
    }

    ioctl(hdmifd, FBIOGET_VSCREENINFO, &screeninfo); //Get all of the info about the screen we are displaying on
    if (screeninfo.bits_per_pixel != 32)
    {
        fprintf(stderr, "Expected 32 bits per pixel\n");
        exit(EXIT_FAILURE);
    }

    //Force resolution if wanted, not needed though because graphics will auto adjust to screen size
    /*
    screeninfo.xres_virtual = 1920;
    screeninfo.yres_virtual = 1080;

    if (ioctl(hdmifd, FBIOPUT_VSCREENINFO, &screeninfo)) {
        perror("Error setting variable information");
    }
    */

    hdmiwidth = screeninfo.xres;
    hdmiheight = screeninfo.yres;

    //Map the buffer to an array so it is easy to access and change
    hdmidata = (uint32_t *)mmap(0, hdmiwidth * hdmiheight * 4, PROT_READ | PROT_WRITE, 
                            MAP_SHARED, hdmifd, 0);
    if (hdmidata == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    //Draw buffer to help abstract access
    drawbuffer = (uint32_t *)malloc(sizeof(uint32_t) * hdmiheight * hdmiwidth);

    pthread_mutex_init(&mutex, NULL);

    if (pthread_create(&paintthread, NULL, refreshThread, NULL) != 0) {
        perror("Error creating HDMI write thread");
    }

}

/* Closes the HDMI */
void closehdmi()
{
    stophdmi = 1;
    usleep(100000); //Give enough time to stop
    //Destroy the mutex
    pthread_mutex_destroy(&mutex);
    pthread_join(paintthread, NULL);
    //Unmap the hdmi
    munmap(hdmidata, hdmiwidth * hdmiheight * 4);
}

/* Sends the current buffer to the screen. Note, no drawings will appear until this function is called */
void paint()
{
    for (int y = 0; y < hdmiheight; y++)
    {
        for (int x = 0; x < hdmiwidth; x++)
        {   
            //Copy the buffer into memory
            hdmidata[y * hdmiwidth + x] = drawbuffer[y * hdmiwidth + x];
        }
    }
}

/* Set a specific pixel to a color */
void setPixel(int x, int y, uint32_t color)
{
    pthread_mutex_lock(&mutex);
    drawbuffer[y * hdmiwidth + x] = color;
    pthread_mutex_unlock(&mutex);
}

/* Enables bulk drawing to draw quicker */
void startPixelBulkDraw()
{
    pthread_mutex_lock(&mutex);
}

/* Disables bulk drawing and enables screen refresh */
void endPixelBulkDraw()
{
    pthread_mutex_unlock(&mutex);
}

/* Sets a specific pixel while in bulk draw mode */
void setPixelBulk(int x, int y, uint32_t color)
{
    drawbuffer[y * hdmiwidth + x] = color;
}

/* draws a rectangle */
void drawRectangle(int x1, int y1, int x2, int y2, uint32_t color)
{
    startPixelBulkDraw();
    for(int j = y1; j < y2; j++) //Loop through and set all the pixels to red
    {
        for(int i = x1; i < x2; i++)
        {
            setPixelBulk(i, j, color);
        }
    }
    endPixelBulkDraw();
}

/* draws a rectangle in bulk mode */
void drawRectangleBulk(int x1, int y1, int x2, int y2, uint32_t color)
{
    for(int j = y1; j < y2; j++) //Loop through and set all the pixels to red
    {
        for(int i = x1; i < x2; i++)
        {
            setPixelBulk(i, j, color);
        }
    }
}

/* copys part of a given buffer into a rectangle on the screen */
void drawRectangleFromBufferBulk(int x1, int y1, int x2, int y2, uint32_t *buf)
{
    for(int j = y1; j < y2; j++) //Loop through and set all the pixels to red
    {
        for(int i = x1; i < x2; i++)
        {
            setPixelBulk(i, j, buf[j * hdmiwidth + i]);
        }
    }
}

/* Draws a character */
void drawCharacter(char ch, int x, int y, int fontSize, uint32_t color)
{
    // ASCII characters start from 32
    if (ch < 0x20 || ch > 0x7E) return;

    for (int i = 0; i < 8; ++i) {
        char pixel = font8x8_basic[(int)(ch)][i];
        for (int j = 0; j < 8; ++j) {
            if (pixel & (1 << (j))) {
                for(int mx = 0; mx < fontSize; mx++)
                    for(int my = 0; my < fontSize; my++)
                    {
                        setPixel(x + (j * fontSize) + mx, y + (i * fontSize) + my, color);
                    }
            } else {
                // Set pixel to black
                //fb_ptr[(y + i) * fb_width + (x + j)] = 0;
            }
        }
    }
}

/* Draws a character in bulk write mode*/
void drawCharacterBulk(char ch, int x, int y, int fontSize, uint32_t color)
{
    // ASCII characters start from 32
    if (ch < 0x20 || ch > 0x7E) return;

    for (int i = 0; i < 8; ++i) {
        char pixel = font8x8_basic[(int)(ch)][i];
        for (int j = 0; j < 8; ++j) {
            if (pixel & (1 << (j))) {
                for(int mx = 0; mx < fontSize; mx++)
                    for(int my = 0; my < fontSize; my++)
                    {
                        setPixelBulk(x + (j * fontSize) + mx, y + (i * fontSize) + my, color);
                    }
            } else {
                // Set pixel to black
                //fb_ptr[(y + i) * fb_width + (x + j)] = 0;
            }
        }
    }
}

/* Draws a string to the screen */
void drawString(char *word, int x, int y, int fontSize, uint32_t color)
{
    int len = strlen(word);
    startPixelBulkDraw();
    for(int i = 0; i < len; i++)
    {
        drawCharacterBulk(word[i], x + (i * 8 * fontSize), y, fontSize, color);
    }
    endPixelBulkDraw();
}

/* Draws a string to the screen */
void drawStringCentered(char *word, int x, int y, int fontSize, uint32_t color)
{
    int len = strlen(word);
    int offw = ((len) * (fontSize * 8)) / 2;
    int offh = (fontSize * 8) / 2;
    startPixelBulkDraw();
    for(int i = 0; i < len; i++)
    {
        drawCharacterBulk(word[i], x + (i * 8 * fontSize) - offw, y - offh, fontSize, color);
    }
    endPixelBulkDraw();
}

/* Gets the height of the screen */
int getheight()
{
    return hdmiheight;
}

/* Gets the width of the screen */
int getwidth()
{
    return hdmiwidth;
}

#include <time.h>
#define TIME_COUNT 200.0

/* Thread function to constantly repaint screen */
void *refreshThread(void *arg)
{
    clock_t start, end;
    double cpu_time_used;

    double times[(int)(TIME_COUNT)];
    int c = 0;


    while(!stophdmi)
    {
        start = clock();
        pthread_mutex_lock(&mutex);
        paint();
        pthread_mutex_unlock(&mutex);
        usleep(100);
        end = clock();
        if(c < TIME_COUNT)
        {
            times[c] = ((double) (end - start)) / CLOCKS_PER_SEC * 1000;
            c++;
        }
        else if(c == TIME_COUNT)
        {
            double avg=0, min=times[0], max=times[0];
            printf("Finished taking times\n");
            for(int i = 0; i < TIME_COUNT;i++)
            {
                avg += times[i];
                if(times[i] < min)
                {
                    min = times[i];
                }
                if(times[i] > max)
                {
                    max = times[i];
                }
            }

            printf("Avg time: %f ms\nMininum Time: %f ms\nMaximum Time: %f ms\nAvg FPS: %f\n", (avg/TIME_COUNT), min, max, 1/((avg/TIME_COUNT) / 1000));
            c++;
        }
            
    }
    pthread_exit(NULL);

    printf("HDMI Closed\n");
}

/* Get the current HDMI buffer */
void getBuffer(uint32_t *output)
{
    pthread_mutex_lock(&mutex);
    memcpy(output, drawbuffer, sizeof(uint32_t) * hdmiheight * hdmiwidth);
    pthread_mutex_unlock(&mutex);
}