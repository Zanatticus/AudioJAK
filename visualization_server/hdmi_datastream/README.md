Create a file: 
`sudo nano /etc/systemd/system/hdmi.service`
And copy the contents of hdmi.service into it.

Create another file:
`sudo nano /etc/systemd/system/hdmi.socket`
And copy the contents of hdmi.socket into it.

Reload the systemctl daemon
`sudo systemctl daemon-reload`

Start the hdmi data streaming socket
`sudo systemctl start hdmi.socket`