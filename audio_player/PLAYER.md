# Audio Player 

## System Diagram 
![image](playerSD.png)

## player.c:

This file is responsible for the main logic of the audio visualization and player serving as the interface between the audio player and the HDMI display. It updates the HDMI display with audio file information such as left, right, and current cursors and board information such as the IP address of the Zedboard and the number of users on the board. 

The player.c file provides a user interface from which the user can select from the various audio playing and editing options through the main menu and CLI commands. 
The main menu provides users with 5 options: 1. Play audio file, 2. Play audio file in reverse, 3. Cut audio file, 4. Inverse cut audio file, 5. Exit. 

## audio_player.c /audio_player.h file:

These files are responsible for all of the audio playing and editing that can be performed on the Zedboard. This includes playing the audio file within specified cursors, playing the entire audio file in reverse, looping through the audio file for a finite or infinite number of times, pause and play functionality, and cutting out a portion of the audio file. They are also responsible for getting simple Zedboard information such as the IP address and the number of users on the board. 

