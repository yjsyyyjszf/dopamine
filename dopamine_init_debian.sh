### BEGIN INIT INFO
# Provides:          dopamine
# Required-Start:    $network $remote_fs $syslog
# Required-Stop:     $network $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: A document-oriented PACS
# Description:       Dopamine is document-oriented PACS
### END INIT INFO

NAME=dopamine
DAEMON=/usr/bin/dopamine
DESC=PACS
RUNDIR=/var/run/dopamine
PIDFILE=${RUNDIR}/${NAME}.pid

if test ! -x ${DAEMON}; then
    echo "Could not find ${DAEMON}"
    exit 0
fi

. /lib/lsb/init-functions

STARTTIME=1
# Time to wait for the server to die, in seconds. If this value is set too low
# you might not let some servers to die gracefully and 'restart' will not work
DIETIME=10

DAEMONUSER=${DAEMONUSER:-dopamine}
DAEMON_OPTS=${DAEMON_OPTS:-""}

running_pid() {
# Check if a given process pid's cmdline matches a given name
    pid=$1
    name=$2
    [ -z "$pid" ] && return 1
    [ ! -d /proc/$pid ] &&  return 1
    cmd=`cat /proc/$pid/cmdline | tr "\000" "\n"|head -n 1 |cut -d : -f 1`
    # Is this the expected server
    [ "$cmd" != "$name" ] &&  return 1
    return 0
}

running() {
# Check if the process is running looking at /proc
# (works for all users)

    # No pidfile, probably no daemon present
    [ ! -f "$PIDFILE" ] && return 1
    pid=`cat $PIDFILE`
    running_pid $pid $DAEMON || return 1
    return 0
}

start_server() {
    test -e "${RUNDIR}" || install -m 755 -o dopamine -g dopamine -d "${RUNDIR}"
    start-stop-daemon --background --start --quiet --pidfile ${PIDFILE} \
        --make-pidfile --chuid ${DAEMONUSER} --exec ${DAEMON} -- ${DAEMON_OPTS}
    errcode=$?
    return ${errcode}
}

stop_server() {
    start-stop-daemon --stop --quiet --pidfile ${PIDFILE} \
        --user ${DAEMONUSER} --exec ${DAEMON}
    errcode=$?
    return ${errcode}
}

case "$1" in
    start)
        log_daemon_msg "Starting $DESC" "$NAME"
        # Check if it's running first
        if running ;  then
            log_progress_msg "apparently already running"
            log_end_msg 0
            exit 0
        fi
        if start_server ; then
            # NOTE: Some servers might die some time after they start,
            # this code will detect this issue if STARTTIME is set
            # to a reasonable value
            [ -n "$STARTTIME" ] && sleep $STARTTIME # Wait some time
            if  running ;  then
                # It's ok, the server started and is running
                log_end_msg 0
            else
                # It is not running after we did start
                log_end_msg 1
        fi
        else
            # Either we could not start it
            log_end_msg 1
        fi
        ;;
    stop)
        log_daemon_msg "Stopping $DESC" "$NAME"
        if running ; then
            # Only stop the server if we see it running
            errcode=0
            stop_server || errcode=$?
            log_end_msg $errcode
        else
            # If it's not running don't do anything
            log_progress_msg "apparently not running"
            log_end_msg 0
            exit 0
        fi
        ;;
    force-stop)
        # First try to stop gracefully the program
        $0 stop
        if running; then
            # If it's still running try to kill it more forcefully
            log_daemon_msg "Stopping (force) $DESC" "$NAME"
            errcode=0
            force_stop || errcode=$?
            log_end_msg $errcode
        fi
        ;;
    restart|force-reload)
        log_daemon_msg "Restarting $DESC" "$NAME"
        errcode=0
        stop_server || errcode=$?
        # Wait some sensible amount, some server need this
        [ -n "$DIETIME" ] && sleep $DIETIME
        start_server || errcode=$?
        [ -n "$STARTTIME" ] && sleep $STARTTIME
        running || errcode=$?
        log_end_msg $errcode
        ;;
    status)
        log_daemon_msg "Checking status of $DESC" "$NAME"
        if running ;  then
            log_progress_msg "running"
            log_end_msg 0
        else
            log_progress_msg "apparently not running"
            log_end_msg 1
            exit 1
        fi
        ;;
    # Dopamine can't reload its configuration.
    reload)
        log_warning_msg "Reloading $NAME daemon: not implemented, as the daemon"
        log_warning_msg "cannot re-read the config file (use restart)."
        ;;
    *)
        N=/etc/init.d/$NAME
        echo "Usage: $N {start|stop|force-stop|restart|force-reload|status}" >&2
        exit 1
        ;;
esac

exit 0
