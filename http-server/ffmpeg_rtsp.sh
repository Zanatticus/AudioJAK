#!/bin/bash
VIDSOURCE="rtsp://audiojak:audiojak@129.10.158.202:554"
AUDIO_OPTS="-c:a aac -b:a 160000 -ac 2"
VIDEO_OPTS="-s 854x480 -c:v libx264 -b:v 800000"
OUTPUT_HLS="-hls_time 10 -hls_list_size 10 -start_number 1"
ffmpeg -i "$VIDSOURCE" -y $AUDIO_OPTS $VIDEO_OPTS $OUTPUT_HLS mystream.m3u8

# This shell script is for streaming RTSP to HLS. It uses ffmpeg to convert 
# the RTSP stream sourced by the Raspberry Pi capturing audio and HDMI, to HLS 
# format for the HTTP server to display.