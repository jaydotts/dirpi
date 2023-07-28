#!/bin/bash
cd "$HOME/dirpi"
PIDFILE="$HOME/dirpi/run.pid"

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
    pkill -P -0 $prevpid 
  else 
    false
  fi
}

# data acquisition manager
DAQ () {
  case $1 in 
    start)
        date +"PID: $$ Process started at %H:%M:%S"
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
        echo "Stopping... This may take several seconds."
        sleep 1
        local wait_s=0
        while [ -f $PIDFILE ] && [ $wait_s -lt 10 ]; do
          sleep 1 
          ((wait_s++))
        done
        rm ".stop"
        ;;
    
    stop-hard)
        if [ -f $PIDFILE ]; then
          pid=$(cat "$PIDFILE")
          pkill -P $pid 
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
  date +"PID: $$ Request completed at %H:%M:%S"
}

# main execution
manager () {
  case $1 in 
    start)
      if previous_instance_active
      then 
        date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
      else 
      trap remove_pidfile EXIT
      create_pidfile
      DAQ start
      #remove_pidfile
      source "$HOME/dirpi/check_network.sh"
      fi
      ;;

    stop)
      DAQ stop
      if [ -f $PIDFILE ]; then 
        if [ previous_instance_active ]; then 
          date +"[%H:%M:%S] There was an issue stopping the run. Attempting to force stop..."
          DAQ stop-hard
          if [ -f $PIDFILE ]; then 
            if [ $(ps -p $(cat $PIDFILE) > /dev/null) ]; then
              date +"[%H:%M:%S] ERR_CRITICAL: PID $$ could not be stopped."
            else
              if [ -f $PIDFILE ]; then 
                rm $PIDFILE 
              fi
              date +"PID: $$ Process terminated at %H:%M:%S"
            fi
          fi
        else 
          date +"PID: $$ Process terminated at %H:%M:%S"
        fi 
      else 
        date +"[%H:%M:%S] Process exited successfully."
      fi 
      ;;

    restart) 
      manager stop
      date +"PID: $$ Restarting... %H:%M:%S"
      manager start
      ;;

    *)
      date +"[%H:%M:%S] Nothing to do. Exiting..."
      ;;
  esac 
}
echo "Triggering DAQ"
manager $1
