Create a file: 
`sudo nano /etc/systemd/system/mongoose.service`
And copy the contents of mongoose.service into it.

Reload the systemctl daemon
`sudo systemctl daemon-reload`

Start the hdmi data streaming socket
`sudo systemctl start mongoose.service`
This will start up a mongoose websocket SERVER that idles until the mongoose websocket CLIENT connects to it via port 8080

To connect with client, run `make` within `./mongoose_client` or the executable `./mongoose_client/ws_client`

To stop the mongoose server,
`sudo systemctl stop mongoose.service`