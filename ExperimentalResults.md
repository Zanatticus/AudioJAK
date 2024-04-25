# Experimental Results

## HDMI Display Experimental Results
The most important factor with displays is refresh rate. This was tested when under immense stress (aka refreshing as often as possible), and then normal stress (aka normal application operation).

It was found that immense stress, we were able to acheive 10.48 frames per second on average, and under normal usage we achieved 12.20 frames per second on average. We consider this satisfactory since there are not many fast or moving components which allows the application to work without a hitch.

## Audio Player Experimental Results
When running the player.c file, it was observed that the CPU utilization was 150%. This is a direct result of the multithreaded design of the HDMI visualization. Without the HDMI display code being used in the player.c file, the CPU utilization was around 4%.
