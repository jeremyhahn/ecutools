#!/bin/bash

# BeagleBone Black Debian Installer

if [ "`id -u`" != "0" ];then
  echo "You must be root to run the installer"
  exit 1
fi

apt-get update
apt-get remove -y lightdm xserver-* apache2* --purge
apt-get install -y cmake libssl-dev libjansson-dev libcurl4-openssl-dev can-utils
apt-get autoremove -y

git clone https://github.com/jeremyhahn/ecutools.git
cd ecutools

./autogen.sh && ./configure && make mbedtls

MYUID=ecutune
MYGID=ecutools
LOGDIR=/var/log/ecutools
CERTDIR=/etc/ecutools/certs

groupadd $MYGID
useradd -G $MYGID -r $MYUID -s /bin/false

mkdir $LOGDIR
chown root.$MYGID $LOGDIR
chmod 775 $LOGDIR

mkdir -p $CERTDIR

echo "Create Thing, copy certs to $CERTDIR, and configure aws_iot_config.h."
read -n1 -r -p "Press any key to continue..." key
make && sudo make install

chown -R root.$MYGID $CERTDIR
chmod 775 $CERTDIR
chmod 660 $CERTDIR/*

echo '
#!/bin/sh

set -e

MYUID=ecutune
MYGID=ecutools
LOGDIR=/var/log/ecutools

NAME=ecutuned
PIDFILE=/var/run/$NAME.pid
DAEMON=/usr/local/bin/ecutuned
DAEMON_OPTS="-d -l $LOGDIR"

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"

case "$1" in
  start)
        echo -n "Starting daemon: "$NAME
	start-stop-daemon --chuid $MYUID:$MYGID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
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
	start-stop-daemon --chuid $MYUID:$MYGID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
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

#rm -rf ecutools

