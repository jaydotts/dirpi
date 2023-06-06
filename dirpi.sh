#!/bin/bash
cd /home/dirpi4/dirpi/
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
  date +"PID: $$ Action started at %H:%M:%S"
  case $1 in 
    start)
        run_num=$( tail -n 1 runlist.txt )
        run_num=$((run_num+1))
        printf "%i\n" $run_num >> runlist.txt

        output_folder="Run"$run_num

        if [ ! -z "$run_num" ]; then 
            make compiler 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            echo "starting DAQ"
            echo "Run=$run_num"
            rm ".stop"
            make -j2 RUN=$run_num

        else
            make compiler
            rm ".stop"
	        make -j2 runner plotter
        fi
        ;;

    stop) 
        touch ".stop" 
        echo "Stopping..."
        sleep 3
        rm ".stop"
        exit
        ;;

    restart)
        touch ".stop" 
        echo "Stopping..."
        sleep 3
        rm ".stop"
        
        echo "Restarting..." 
        
        if [ ! -z "$run_num" ]; then 
            make compiler 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            make -j4 RUN=$2

        else
            make compiler
            make runner
	    make plotter
        fi
        ;;
    
    start-soft)
        echo "[WARNING]: Code is not be re-compiled."
        echo "Configuration changes WILL be loaded" 
        if [ ! -z "$run_num" ]; then 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
        fi
        make soft-reset RUN=$run_num
    ;;

    *) 
        echo "Nothing to do"
esac
  date +"PID: $$ Action finished at %H:%M:%S"
}

if previous_instance_active
then 
  date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
else 
trap remove_pidfile EXIT
create_pidfile
start_DAQ $1
remove_pidfile
fi