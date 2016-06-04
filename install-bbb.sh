#!/bin/sh

# BeagleBone Black Debian Jessie Installer

if [ "`id -u`" != "0" ];then
  echo "You must be root to run the installer"
  exit 1
fi

apt-get update
apt-get remove -y lightdm xserver-* apache2* chromium-* lxqt-* x11-* xauth xbitmaps xfonts-* --purge
apt-get install -y cmake libssl-dev libjansson-dev libcurl4-openssl-dev can-utils
apt-get autoremove -y

MYUID=ecutune
MYGID=ecutools
LOGDIR=/var/log/ecutools
CERTDIR=/etc/ecutools/certs
CACHEDIR=/var/ecutools/cache

groupadd $MYGID
useradd -G $MYGID -r $MYUID -s /bin/false

mkdir $LOGDIR
chown root.$MYGID $LOGDIR
chmod 775 $LOGDIR

mkdir -p $CERTDIR
chown -R root.$MYGID $CERTDIR
chmod 775 $CERTDIR
chmod 660 $CERTDIR/*

mkdir -p $CACHEDIR
chown -R root.$MYGID $CACHEDIR
chmod 775 $CACHEDIR

git clone https://github.com/jeremyhahn/ecutools.git
cd ecutools

./autogen.sh
./configure
make mbedtls

echo "Create Thing, copy certs to $CERTDIR, and configure aws_iot_config.h."
read -n1 -r -p "Then, press any key to continue..." key

make
make install

echo '#!/bin/sh

### BEGIN INIT INFO
# Provides:          ecutuned
# Required-Start:    $local_fs $remote_fs $network $syslog $named
# Required-Stop:     $local_fs $remote_fs $network $syslog $named
# Default-Start:     3 5
# Default-Stop:      0 1 6
# Short-Description: starts the ecutuned service
# Description:       starts ecutuned using start-stop-daemon
### END INIT INFO

MYUID=ecutune
MYGID=ecutools

NAME=ecutuned
PIDFILE=/var/run/$NAME.pid
LOGDIR=/var/log/ecutools
DAEMON=/usr/local/bin/ecutuned
DAEMON_OPTS="-l $LOGDIR -d"

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"

case "$1" in
  start)
        echo -n "Starting daemon: "$NAME
        start-stop-daemon --chuid $MYUID:$MYGID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
        ;;
  stop)
        echo -n "Stopping daemon: "$NAME
        pkill ecutuned
        #start-stop-daemon --stop --quiet --oknodo --pidfile $PIDFILE
        ;;
  restart)
        echo -n "Restarting daemon: "$NAME
        #start-stop-daemon --stop --quiet --oknodo --retry 30 --pidfile $PIDFILE
        pkill ecutuned
        start-stop-daemon --chuid $MYUID:$MYGID --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
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

rm -rf ecutools

