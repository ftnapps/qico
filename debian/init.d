#!/bin/sh
# initscript for qico.
# $Id: init.d,v 1.4 2004/06/13 17:05:46 sisoft Exp $

PATH=/bin:/usr/bin:/sbin:/usr/sbin
DAEMON=/usr/sbin/qico
QCTL=/usr/bin/qctl
FLAGS="defaults 63"

test -f $DAEMON || exit 0
if ! test -f /etc/ftn/qico.conf; then
  if [ "$1" = "start" ]; then
    echo "Before run qico, rename /etc/ftn/qico.conf.sample to qico.conf and edit it!"
  fi
  exit 0
fi

case "$1" in
  start)
    echo -n "Starting FTN services: qico"
    start-stop-daemon --start --exec $DAEMON
    echo "."
    ;;
  stop)
    echo -n "Stopping FTN services: qico"
    if [ -f $QCTL ]; then
	$QCTL -q
    else
	start-stop-daemon --stop --exec $DAEMON
    fi
    echo "."
    ;;
  reload)
    if [ -f $QCTL ]; then
	$QCTL -R
    else
	start-stop-daemon --stop --signal 1 --exec $DAEMON
    fi
    ;;
  restart|force-reload)
    echo -n "Restarting FTN services: qico"
    if [ -f $QCTL ]; then
	$QCTL -q
    else
	start-stop-daemon --stop --exec $DAEMON
    fi
    sleep 1
    start-stop-daemon --start --exec $DAEMON
    echo "."
    ;;
  *)
    echo "Usage: /etc/init.d/qico {start|stop|restart|force-reload}"
    exit 1
    ;;
esac

exit 0
