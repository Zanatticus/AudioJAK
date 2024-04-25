# Remote Audio Visualization and Streaming

## System Diagram

![remote_http_system_diagram](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/1bec6696-e483-4875-9e07-3484c47652e1)

Zedboard:
- Outputs audio signals on the line-out audio jack into the USB audio capture card
- Copies the HDMI pixel buffer (`/dev/fb0`) background to the Raspberry Pi Linux server/HTTP server
  - `ssh-keygen` initially set up to enable `scp` commands to the HTTP server directory
  - OR use curl/wget to send data via HTTP PUT requests to the HTTP server

Raspberry Pi (Linux Server):
- Contains the Mongoose HTTP server for serving static HDMI waveforms and "streaming" audio from the Zedboard
  - HTML file for reading HDMI pixel data files and HLS playlist files
  - Signal interrupt handling for clean server shutdown
  - systemd service for starting the HTTP server and restarting a crashed HTTP server
- Uses FFmpeg to stream audio to the HTTP server
  - Continuously streams the Pulse-Code Modulation (PCM) signal detected by the USB capture card as re-encoded libmp3lame formatted HTTP-Live Streaming (HLS) files
  - Can also stream audio over RTP/RTSP to VLC Media Player

## Experimental Results
- Sound capture card re-streaming latency: averaged ~3-5 seconds from initial Zedboard WAV file playing
- Webserver HLS.min.js script loading: ~10-20 seconds
- Webserver HLS.min.js script playlist parsing: Network Timeout Error
- SCP HDMI Data: ~3 seconds
- Curl HDMI Data: ~60 seconds

We also tested streaming an MP4 file (audio & video) over RTP to VLC Media Player on the Raspberry Pi which we found to have a refresh rate of 1 frame every 15 seconds (i.e. an unusable streaming method)

## Webserver Examples
![image](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/f2599ab7-04bf-4bc8-9348-fb5828fda85a)
Accessing HTTP server from LAN (non-NUWave)

![image](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/3e326887-2223-498b-9d85-5b255c479c5f)
Accessing HTTP server from Raspberry Pi IP address
