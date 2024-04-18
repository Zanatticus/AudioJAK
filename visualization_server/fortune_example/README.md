# Example systemd socket activation

The following example was taken from https://mgdm.net/weblog/systemd-socket-activation/

Make sure fortune is installed with
`sudo apt install fortune-mod`

Create a file called `/etc/systemd/system/fortune@.service` and copy the contents of `fortune@.service` into it.

`sudo nano /etc/systemd/system/fortune@.service`

Note the @ in the filename - this is significant as it indicates the service is a template, and that a new instance of the service will be run on every connection.

Create another file called `/etc/systemd/system/fortune.socket` and copy the contents of `fortune.socket` into it.

Reload the systemctl daemon
`sudo systemctl daemon-reload`

Start the fortune game socket
`sudo systemctl start fortune.socket`

If you want to start fortune.socket on boot, enable fortune.socket
`sudo systemctl enable fortune.socket`

Now that fortune is running on port 17, connect to the port with another application (nc or telnet)

`nc localhost 17`

Example response:
"Never trust an operating system"