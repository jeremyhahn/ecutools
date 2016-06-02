#!/bin/bash

# BeagleBone Black / Debian Installer

sudo apt-get update
sudo apt-get install -y cmake libssl-dev libjansson-dev libcurl4-openssl-dev can-utils
sudo apt-get autoremove -y

git clone https://github.com/jeremyhahn/ecutools.git
cd ecutools

./autogen.sh && ./configure && make mbedtls && make && sudo make install

THING_NAME=myj2534
UID=ecutune
GID=ecutools
LOGDIR=/var/log/ecutools
groupadd $GID
useradd -r ecutune -s /bin/false -G $GID
mkdir $LOGDIR
chown $UID.$GID $LOGDIR
chmod 664 $UID.$GID $LOGDIR

echo '
#!/bin/sh

set -e

UID=ecutune
GID=ecutools
LOGDIR=/var/log/ecutools

NAME=ecutuned
PIDFILE=/var/run/$NAME.pid
DAEMON=/usr/local/bin/ecutuned
DAEMON_OPTS="-d -l $LOGDIR -n $THING_NAME"

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"

case "$1" in
  start)
        echo -n "Starting daemon: "$NAME
	start-stop-daemon --chuid $UID:$GID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
        echo "."
	;;
  stop)
        echo -n "Stopping daemon: "$NAME
	start-stop-daemon --stop --quiet --oknodo --pidfile $PIDFILE
        echo "."
	;;
  restart)
        echo -n "Restarting daemon: "$NAME
	start-stop-daemon --stop --quiet --oknodo --retry 30 --pidfile $PIDFILE
	start-stop-daemon --chuid $UID:$GID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
	echo "."
	;;
  *)
	echo "Usage: "$1" {start|stop|restart}"
	exit 1
esac

exit 0
' > /etc/init.d/ecutuned

chmod 755 /etc/init.d/ecutuned
update-rc.d ecutuned defaults

/etc/init.d/ecutuned start

