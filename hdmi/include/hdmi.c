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

int hdmifd;
struct fb_var_screeninfo screeninfo;
uint32_t *hdmidata, *drawbuffer;
uint32_t *buffer;
uint32_t hdmix, hdmiy, hdmiwidth, hdmiheight;

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
}

/* Closes the HDMI */
void closehdmi()
{
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
    drawbuffer[y * hdmiwidth + x] = color;
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