#include "include/hdmi.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
    inithdmi();

    while(1)
    {
        for(int j = 0; j < getheight(); j++) //Loop through and set all the pixels to red
        {
            for(int i = 0; i < getwidth(); i++)
            {
                setPixel(i, j, 0xFF0000);
            }
        }

        paint(); //Needs to be called often in order to refresh the screen
    }

    closehdmi();
}