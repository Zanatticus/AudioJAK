#!/bin/bash
SOURCE="hw:CARD=3,DEV=0"
AUDIO_OPTS="-re -f alsa -ac 2 -ar 48000 -sample_fmt s16"
OUTPUT_OPTS="-acodec libmp3lame -f rtp"
OUTPUT="rtp://127.0.0.1:1234"
ffmpeg $AUDIO_OPTS -i "$SOURCE" $OUTPUT_OPTS $OUTPUT

# Essentially does the below command:
# ffmpeg -re -f alsa -ac 2 -ar 48000 -sample_fmt s16 -i hw:CARD=3,DEV=0 -acodec libmp3lame -f rtp rtp://127.0.0.1:1234
