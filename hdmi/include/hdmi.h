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

/* copys part of a given buffer into a rectangle on the screen */
void drawRectangleFromBufferBulk(int x1, int y1, int x2, int y2, uint32_t *buf);

/* Draws a character */
void drawCharacter(char ch, int x, int y, int fontSize, uint32_t color);

/* Draws a character in bulk mode*/
void drawCharacterBulk(char ch, int x, int y, int fontSize, uint32_t color);

/* Draws a string to the screen */
void drawString(char *string, int x, int y, int fontSize, uint32_t color);

/* Draws a string centered around the x, y */
void drawStringCentered(char *word, int x, int y, int fontSize, uint32_t color);

/* Sends the current buffer to the screen. Note, no drawings will appear until this function is called */
void paint();

/* Gets the width of the screen */
int getwidth();

/* Gets the height of the screen */
int getheight();

/* Thread function to constantly repaint screen */
void *refreshThread(void *arg);

/* Get the current HDMI buffer */
void getBuffer(uint32_t *output);

#endif