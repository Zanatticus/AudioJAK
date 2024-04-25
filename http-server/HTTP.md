# Remote Audio Visualization and Streaming

## System Diagram

![remote_http_system_diagram](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/1bec6696-e483-4875-9e07-3484c47652e1)

Zedboard:
- Outputs audio signals on the line-out audio jack into the USB audio capture card
- Copies the HDMI pixel buffer (`/dev/fb0`) background to the Raspberry Pi Linux server/HTTP server
  - `ssh-keygen` initially set up to enable `scp` commands to the HTTP server directory
  - OR use curl/wget to send data via HTTP PUT requests to the HTTP server

Raspberry Pi (Linux Server):
- Contains the Mongoose HTTP server (see [mongoose.md](https://github.com/neu-ece-4534-sp24/prj-audiojak/blob/main/http-server/mongoose.md) for more details) for serving static HDMI waveforms and "streaming" audio from the Zedboard
  - HTML file for reading HDMI pixel data files and HLS playlist files
  - Signal interrupt handling for clean server shutdown
  - systemd service for starting the HTTP server and restarting a crashed HTTP server
- Uses FFmpeg to stream audio to the HTTP server
  - Continuously streams the Pulse-Code Modulation (PCM) signal detected by the USB capture card as re-encoded libmp3lame formatted HTTP-Live Streaming (HLS) files
  - Can also stream audio over RTP/RTSP to VLC Media Player

## Webserver Examples
![image](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/f2599ab7-04bf-4bc8-9348-fb5828fda85a)
Accessing HTTP server from LAN (non-NUWave)

![image](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/3e326887-2223-498b-9d85-5b255c479c5f)
Accessing HTTP server from Raspberry Pi IP address

## Physical Setup
![IMG_2541](https://github.com/neu-ece-4534-sp24/prj-audiojak/assets/106758747/d254cfdf-ee62-4c4b-9b34-98096279a68a)
