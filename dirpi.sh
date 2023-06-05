#!/bin/bash

run_num=$( tail -n 1 runlist.txt )
((run_num=run_num+1))
printf "%i\n" $run_num >> runlist.txt

output_folder="Run"$run_num

case $1 in 
    start)
        if [ ! -z "$run_num" ]; then 
            make compiler 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            rm ".stop"
            make -j4 RUN=$run_num

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
