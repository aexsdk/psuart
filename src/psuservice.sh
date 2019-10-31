#!/bin/bash
#
### BEGIN INIT INFO
# Provides: parkserver
# Required-Start: $local_fs $network $named $remote_fs $syslog
# Required-Stop: $local_fs $network $named $remote_fs $syslog
# Short-Description: Very Secure Ftp Daemon
# Description: parkserver is a Very Secure FTP daemon. It was written completely from
#              scratch
### END INIT INFO

# parkserver      This shell script takes care of starting and stopping
#             standalone parkserver.
#
# chkconfig: - 60 50
# description: Vsftpd is a ftp daemon, which is the program \
#              that answers incoming ftp service requests.
# processname: parkserver
# config: /eztor/parkserver/cfg/parkserver.conf

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

BINPATH="/usr/bin"
CONFIGFILE="/etc/psuconfig.conf"
LOGFILE="/var/log/psuconfig.log"
BINFILE="$BINPATH/psuart"
RETVAL=0
prog="psuart"

start() {
        # Start daemons.
	
	# Check that networking is up.
	[ ${NETWORKING} = "no" ] && exit 1
	[ -x $BINFILE ] || exit 1
	[ -x $CONFIGFILE ] || exit 1
	echo -n $"Starting $prog : "
	daemon $BINFILE --config $CONFIGFILE --log $LOGFILE
	RETVAL=$?
	echo
        [ $RETVAL -eq 0 ]
        return $RETVAL
}

stop() {
        # Stop daemons.
        echo -n $"Shutting down $prog: "
        killproc $prog
        RETVAL=$?
        echo
        [ $RETVAL -eq 0 ] && rm -f /var/lock/subsys/$prog
        return $RETVAL
}

# See how we were called.
case "$1" in
  start)
        start
        ;;
  stop)
        stop
        ;;
  restart|reload)
        stop
        start
        RETVAL=$?
        ;;
  condrestart|try-restart|force-reload)
        if [ -f /var/lock/subsys/$prog ]; then
            stop
            start
            RETVAL=$?
        fi
        ;;
  status)
        status $prog
        RETVAL=$?
        ;;
  *)
        echo $"Usage: $0 {start|stop|restart|try-restart|force-reload|status}"
        exit 1
esac

exit $RETVAL
