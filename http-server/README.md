# HTTP Server

See `./HTTP.md` for system design overview and results

## Setup

To continuously stream audio from the soundcard connected to the Zedboard's line-out, run `./ffmpeg_http.sh`. The three shell scripts (`./ffmpeg_http.sh`, `./ffmpeg_rtp.sh`, `./ffmpeg_rtsp_source`) can also stream the soundcard to VLC Media Player to validate successful audio streaming.

To set up the `systemd` service for the HTTP server, follow the general instructions within `./test_server/systemd_test` but catered towards `./http.service`.

## HTTP URL
The HTTP server runs on the Raspberry Pi's TCP port 8000 and can be accessed by devices on the same network (LAN) (http://localhost:8000, http://audiojak.local:8000)
