# Battery watcher daemon

Daemon for auto execute $COMMAND on low battery.

# Install
    $ git clone https://github.com/jollheef/battery-watcher-daemon
    $ make
	$ sudo make install
    $ sudo rc-update add battery-watcherd default
	
# Configuration
Use DAEMON_ARGS variable in battery-watcherd.

For example:

* Shutdown on 50%

    DAEMON_ARGS="50"

* Execute pm-suspend on 30%

    DAEMON_ARGS="30 pm-suspend"
