#!/bin/sh
# initscript for qico.

PATH=/bin:/usr/bin:/sbin:/usr/sbin
DAEMON=/usr/sbin/qico
QCTL=/usr/bin/qctl
FLAGS="defaults 63"

test -f $DAEMON || exit 0

case "$1" in
  start)
    echo -n "Starting FTN services: qico"
    start-stop-daemon --start --verbose --exec $DAEMON
    echo "."
    ;;
  stop)
    echo -n "Stopping FTN services: qico"
    if [ -f $QCTL ]; then
	$QCTL -q
    else
	start-stop-daemon --stop --verbose --exec $DAEMON
    fi
    echo "."
    ;;
  reload)
    if [ -f $QCTL ]; then
	$QCTL -R
    else
	start-stop-daemon --stop --signal 1 --verbose --exec $DAEMON
    fi
    ;;
  restart|force-reload)
    echo -n "Restarting FTN services: qico"
    if [ -f $QCTL ]; then
	$QCTL -q
    else
	start-stop-daemon --stop --verbose --exec $DAEMON
    fi
    sleep 1
    start-stop-daemon --start --verbose --exec $DAEMON
    echo "."
    ;;
  *)
    echo "Usage: /etc/init.d/qico {start|stop|restart|force-reload}"
    exit 1
    ;;
esac

exit 0
