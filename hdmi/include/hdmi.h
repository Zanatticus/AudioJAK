#ifndef HDMI_H
#define HDMI_H

#include <stdint.h>

/* Initialize the HDMI */
void inithdmi();

/* Closes the HDMI */
void closehdmi();

/* Set a specific pixel to a color */
void setPixel(int x, int y, uint32_t color);

/* Sends the current buffer to the screen. Note, no drawings will appear until this function is called */
void paint();

/* Gets the width of the screen */
int getwidth();

/* Gets the height of the screen */
int getheight();


#endif