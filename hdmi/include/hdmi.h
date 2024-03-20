#ifndef HDMI_H
#define HDMI_H

#include <stdint.h>

/* Initialize the HDMI */
void inithdmi();

/* Closes the HDMI */
void closehdmi();

/* Set a specific pixel to a color */
void setPixel(int x, int y, uint32_t color);

/* Enables bulk drawing to draw quicker */
void startPixelBulkDraw();

/* Disables bulk drawing and enables screen refresh */
void endPixelBulkDraw();

/* Sets a specific pixel while in bulk draw mode */
void setPixelBulk(int x, int y, uint32_t color);

/* draws a rectangle */
void drawRectangle(int x1, int y1, int x2, int y2, uint32_t color);

/* draws rectangle in bulk mode */
void drawRectangleBulk(int x1, int y1, int x2, int y2, uint32_t color);

/* Sends the current buffer to the screen. Note, no drawings will appear until this function is called */
void paint();

/* Gets the width of the screen */
int getwidth();

/* Gets the height of the screen */
int getheight();

/* Thread function to constantly repaint screen */
void *refreshThread(void *arg);


#endif