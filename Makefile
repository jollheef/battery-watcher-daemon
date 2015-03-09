all:
	gcc battery-watcher.c -o battery-watcher

install:
	cp battery-watcher /usr/bin/
	cp battery-watcherd /etc/init.d/

uninstall:
	rm /usr/bin/battery-watcher
	rm /etc/init.d/battery-watcherd
