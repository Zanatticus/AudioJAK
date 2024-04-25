# Ubuntu Server (systemd) Test Service

To debug, use `journalctl -xe`

Make sure systemd is available on the system.
`systemd --version`

If not present, 
`sudo apt-get install -y systemd`

Write a new service script (located at ./test.service) to run the test.py file. Note that modifying the file name to be test@.service will indicate the service as a template, and that a new instance of the service will be run on every connection.
`sudo nano /etc/systemd/system/test.service`

Reload the daemon
`sudo systemctl daemon-reload`

Enable the test service so that it doesn't get disabled if the server restarts (i.e. enable for service start on boot)
`sudo systemctl enable test.service`

Start the test service
`sudo systemctl start test.service`

At this point, the service is up and running

To stop the service
`sudo systemctl stop test`

To restart the service
`sudo systemctl restart test`

To check the status of the service
`sudo systemctl status test`

## Socket Exposure

To expose a socket that the Ubuntu service/server and the Mongoose embedded web client can communicate through, a respective test.socket file must be made (copy the contents of ./test.socket). Socket must start before service!
`sudo nano /etc/systemd/system/test.socket`

Reload the daemon
`sudo systemctl daemon-reload`

Start test.socket
`sudo systemctl start test.socket`
