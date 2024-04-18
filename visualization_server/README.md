# Audio Visualizer Ubuntu Server

To debug, use `journalctl -xe`

Make sure systemd is available on the system.
`systemd --version`

If not present, 
`sudo apt-get install -y systemd`

Write a new service script (located at ./visualization_server/visualizer.service) to run the visualizer.py file. Note that modifying the file name to be visualizer@.service will indicate the service as a template, and that a new instance of the service will be run on every connection.
`sudo nano /etc/systemd/system/visualizer.service`

Reload the daemon
`sudo systemctl daemon-reload`

Enable the visualizer service so that it doesn't get disabled if the server restarts (i.e. enable for service start on boot)
`sudo systemctl enable visualizer.service`

Start the visualizer service
`sudo systemctl start visualizer.service`

At this point, the service is up and running

To stop the service
`sudo systemctl stop visualizer`

To restart the service
`sudo systemctl restart visualizer`

To check the status of the service
`sudo systemctl status visualizer`

## Socket Exposure

To expose a socket that the Ubuntu service/server and the Mongoose embedded web client can communicate through, a respective visualizer.socket file must be made (copy the contents of /prj-audiojak/visualization_server/visualizer.socket). Socket must start before service!
`sudo nano /etc/systemd/system/visualizer.socket`

Reload the daemon
`sudo systemctl daemon-reload`

Start visualizer.socket
`sudo systemctl start visualizer.socket`
