# HTTP Server

See `./HTTP.md` for system design overview and results

## Setup

To continuously stream audio from the soundcard connected to the Zedboard's line-out, run `./ffmpeg_http.sh`. The three shell scripts (`./ffmpeg_http.sh`, `./ffmpeg_rtp.sh`, `./ffmpeg_rtsp_source`) can also stream the soundcard to VLC Media Player to validate successful audio streaming.

To set up the `systemd` service for the HTTP server, follow the general instructions within `./test_server/systemd_test` but catered towards `./http.service`.

## HTTP URL
The HTTP server runs on the Raspberry Pi's TCP port 8000 and can be accessed by devices on the same network (LAN) (http://localhost:8000, http://audiojak.local:8000)

## Overall Directory
- `./hls/` contains the directory in which FFmpeg will stream the HLS playlist (m3u8 file) and Transport Stream (TS) files
- `./test_server/` contains the directory for all initial work and testing to get a working HTTP server implementation
- `./upload/` contains the HDMI pixel data directory which the Zedboard uploads to, along with some miscellaneous test files
- `./mongoose.c` and `./mongoose.h` are the Mongoose embedded web server library files
- `./hls.min.js` is the HLS Javascript needed to play an HLS playlist in HTML
- `./http.service` is the `systemd` service for starting and restarting the HTTP server
- `./main.c` is the custom HTTP server implementation that handles HLS streaming requests, file upload requests, and generic HTTP services
- `./index.html` serves the audio/video contents on the web page
- `./ffmpeg_http.sh` streams the capture card audio to the HTTP server (or VLC)
