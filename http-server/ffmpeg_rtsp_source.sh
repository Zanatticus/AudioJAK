#!/bin/bash
RASPBERRY_PI_IP="129.10.158.202"
SOURCE="hw:CARD=3,DEV=0"
AUDIO_OPTS="-re -f alsa -ac 2 -ar 48000 -sample_fmt s16"
OUTPUT_OPTS="-acodec libmp3lame -f rtsp"
RTSP_URL="rtsp://$RASPBERRY_PI_IP:8554"

ffmpeg $AUDIO_OPTS -i "$SOURCE" $OUTPUT_OPTS $RTSP_URL

# This shell script now streams audio from the Raspberry Pi using RTSP.
