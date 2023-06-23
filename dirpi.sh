#!/bin/bash
cd "$HOME/dirpi"
PIDFILE="$HOME/dirpi/run.pid"
source "$HOME/dirpi/utils/_bash_utils.sh"

#################################################### Process management 
create_pidfile () {
  echo $$ > "$PIDFILE"
}

remove_pidfile () {
  [ -f "$PIDFILE" ] && rm "$PIDFILE"
}

# check if process in pidfile is active
previous_instance_active () {
  local prevpid
  if [ -f "$PIDFILE" ]; then
    prevpid=$(cat "$PIDFILE")
    # use pkill, send null to process (do nothing)
    pkill -P -0 $prevpid 
  else 
    false
  fi
}

#################################################### DAQ Custom Run Configurations
monitor() {
  if [ ! -f "plot_buffer.txt" ]; then 
    touch "plot_buffer.txt"
  fi 
  date +"PID: $$ Process started at %H:%M:%S"
  echo "Entering monitor mode... No data saved."
  sleep 1 
  make monitor 
}

#################################################### DAQ Helper Functions

# protocol that is trapped on exit 
exit_protocol(){
  manager stop
  
}

#################################################### Data Acqusition Method
DAQ () {
  case $1 in 
    start)
        # get next run # from runlist.txt
        date +"PID: $$ Process started at %H:%M:%S"
        run_num=$( tail -n 1 runlist.txt )
        run_num=$((run_num+1))
        printf "%i\n" $run_num >> runlist.txt

        output_folder="Run"$run_num

        if [ ! -z "$run_num" ]; then 

            # compile if not already compiled
            if [ ! -f "main" ]; then 
              run_with_timeout 60 make compiler 
            fi 

            if [ ! -f "compression/DiRPi" ]; then 
              run_with_timeout 60 make compression 
            fi 

            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            # get absolute lifetime (in seconds) of run
            echo "starting DAQ"]
            local abs_lifetime=$(parse_config "config/config.ini" "run_lifetime_s" 5)
            echo "Run=$run_num"
            rm ".stop"
            run_with_timeout $abs_lifetime make DAQ RUN=$run_num

        else
            make compiler
            rm ".stop"
	        make -j2 runner plotter
        fi
        ;;

    stop) 
        touch ".stop" 
        echo "Stopping. This may take several seconds."
        local max_count=10
        local count=0
        while [[ -f "$PIDFILE" && $count -lt $max_count ]];do 
          sleep 1
          printf '.' > /dev/tty
          ((count++))
        done 
        echo "Run cycle complete."
        ;;
    
    stop-hard)
        if [ -f $PIDFILE ]; then
          pid=$(cat "$PIDFILE")
          pkill -P $pid 
        fi
        ;;

    *) 
        echo "Nothing to do"
esac
  date +"PID: $$ Request completed at %H:%M:%S"
}

#################################################### Main Function
manager () {
  case $1 in 
    start)
      if previous_instance_active
      then 
        date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
      else 
      trap exit_protocol EXIT
      echo "Updating config file"
      source "$HOME/dirpi/utils/update_configs.sh"
      create_pidfile
      DAQ start
      remove_pidfile
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

    monitor)
      if previous_instance_active
      then 
        date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
      else 
      trap exit_protocol EXIT
      create_pidfile
      monitor 
      remove_pidfile
      fi 
      ;;

    *)
      date +"[%H:%M:%S] Nothing to do. Exiting..."
      ;;
  esac 
}

echo "Command received."
manager $1
