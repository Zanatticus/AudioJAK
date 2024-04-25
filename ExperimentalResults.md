# Experimental Results

## HDMI Display Experimental Results
The most important factor with displays is refresh rate. This was tested when under immense stress (aka refreshing as often as possible), and then normal stress (aka normal application operation).

It was found that immense stress, we were able to acheive 10.48 frames per second on average, and under normal usage we achieved 12.20 frames per second on average. We consider this satisfactory since there are not many fast or moving components which allows the application to work without a hitch.

## Audio Player Experimental Results
When running the player.c file, it was observed that the CPU utilization was 150%. This is a direct result of the multithreaded design of the HDMI visualization. Without the HDMI display code being used in the player.c file, the CPU utilization was around 4%.

## Experimental Results
- Sound capture card re-streaming latency: averaged ~3-5 seconds from initial Zedboard WAV file playing
- Webserver HLS.min.js script loading: ~10-20 seconds
- Webserver HLS.min.js script playlist parsing: Network Timeout Error
- SCP HDMI Data: ~3 seconds
- Curl HDMI Data: ~60 seconds

We also tested streaming an MP4 file (audio & video) over RTP to VLC Media Player on the Raspberry Pi which we found to have a refresh rate of 1 frame every 15 seconds (i.e. an unusable streaming method)
