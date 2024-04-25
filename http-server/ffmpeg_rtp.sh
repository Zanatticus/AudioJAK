#!/bin/bash
RASPBERRY_PI_IP="129.10.158.202" # IP address of the Raspberry Pi. Could use localhost IP
SOURCE="hw:CARD=3,DEV=0"
AUDIO_OPTS="-re -f alsa -ac 2 -ar 48000 -sample_fmt s16"
OUTPUT_OPTS="-acodec libmp3lame -f rtp"
OUTPUT="rtp://$RASPBERRY_PI_IP:1234"
ffmpeg $AUDIO_OPTS -i "$SOURCE" $OUTPUT_OPTS $OUTPUT

# This shell script is for streaming audio from the Raspberry Pi 
# its localhost RTP port to verify that it can capture audio from the soundcard.

# ffmpeg -re -f alsa -ac 2 -ar 48000 -sample_fmt s16 -i hw:CARD=3,DEV=0 -acodec libmp3lame -f rtp rtp://127.0.0.1:1234