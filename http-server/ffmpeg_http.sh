#!/bin/bash
SOURCE="hw:CARD=3,DEV=0"
AUDIO_OPTS="-re -f alsa -ac 2 -ar 48000 -sample_fmt s16"
OUTPUT_OPTS="-acodec libmp3lame -f hls -hls_time 2 -hls_list_size 15 -hls_playlist_type event -hls_flags delete_segments"
HLS_URL="http://localhost:8000/hls/stream.m3u8"

ffmpeg $AUDIO_OPTS -i "$SOURCE" $OUTPUT_OPTS $HLS_URL

# This shell script will capture audio and output it as an HLS playlist on a local HTTP server.
# ffmpeg -re -f alsa -ac 2 -ar 48000 -sample_fmt s16 -i hw:CARD=3,DEV=0 -acodec libmp3lame -f hls -hls_time 2 -hls_list_size 15 -hls_segment_filename '/stream/audio%02d.ts' -hls_flags delete_segments http://localhost:8000/hls/stream.m3u8