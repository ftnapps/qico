#!/bin/sh
#
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
DAEMON=/usr/sbin/qico
QCTL=/usr/bin/qctl
FLAGS="defaults 63"

test -f $DAEMON || exit 0

case "$1" in
  start)
    echo -n "Starting FTN services: "
    start-stop-daemon --start --verbose --exec $DAEMON
    echo "qico"
    ;;
  stop)
    echo -n "Stopping FTN services: "
    start-stop-daemon --stop --verbose --exec $DAEMON
    echo "qico"
    ;;
  reload)
    if [ -f $QCTL ]; then
	$QCTL -R
    else
	start-stop-daemon --stop --signal 1 --verbose --exec $DAEMON
    fi
    ;;
  restart|force-reload)
    echo -n "Restarting FTN services: "
    start-stop-daemon --stop --verbose --exec $DAEMON
    sleep 1
    start-stop-daemon --start --verbose --exec $DAEMON
    echo "qico"
    ;;
  *)
    echo "Usage: /etc/init.d/qico {start|stop|restart|force-reload}"
    exit 1
    ;;
esac

exit 0
