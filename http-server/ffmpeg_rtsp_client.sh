#!/bin/bash
RASPBERRY_PI_IP="129.10.158.202"
VIDSOURCE="rtsp://audiojak:audiojak@$RASPBERRY_PI_IP:8554"
AUDIO_OPTS="-c:a aac -b:a 160000 -ac 2"
VIDEO_OPTS="-s 854x480 -c:v libx264 -b:v 800000"
OUTPUT_HLS="-f hls -hls_time 1 -hls_list_size 15 -start_number 1 -hls_flags delete_segments"
ffmpeg -i "$VIDSOURCE" -y $AUDIO_OPTS $VIDEO_OPTS $OUTPUT_HLS mystream.m3u8

# This shell script is for streaming RTSP to HLS. It uses ffmpeg to convert 
# the RTSP stream sourced by the Raspberry Pi capturing audio and HDMI, to HLS 
# format for the HTTP server to display.