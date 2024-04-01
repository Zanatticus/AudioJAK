Create a file: 
`sudo nano /etc/systemd/system/mongoose.service`
And copy the contents of mongoose.service into it.

Reload the systemctl daemon
`sudo systemctl daemon-reload`

Start the hdmi data streaming socket
`sudo systemctl start mongoose.service`
This will start up a mongoose websocket SERVER that idles until the mongoose websocket CLIENT connects to it via port 8080