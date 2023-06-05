DURATION=$1
PIDFILE="/home/dirpi4/dirpi/run.pid"

create_pidfile () {
  echo $$ > "$PIDFILE"
}

remove_pidfile () {
  [ -f "$PIDFILE" ] && rm "$PIDFILE"
}

previous_instance_active () {
  local prevpid
  if [ -f "$PIDFILE" ]; then
    prevpid=$(cat "$PIDFILE")
    kill -0 $prevpid 
  else 
    false
  fi
}

start_DAQ () {
  date +"PID: $$ Action started at %H:%M:%S, ETA: $DURATION seconds"
  sleep $DURATION
  date +"PID: $$ Action finished at %H:%M:%S"
}

if previous_instance_active
then 
  date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
else 
trap remove_pidfile EXIT
create_pidfile
start_DAQ
remove_pidfile
fi