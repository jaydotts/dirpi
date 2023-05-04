#!/bin/bash

output_folder="Run"$2
run_dir=`pwd`

case $1 in 
    start)
        if [ -f ".start" ]; then 
            echo "[WARNING] Previous run not stopped properly." 
            echo "Run './dirpi stop' to stop the process."
            echo "Exiting..."
        else 
            touch ".start" 
            if [ ! -z "$2" ]; then 
                make compiler DIR=$run_dir
                if [ ! -d "$output_folder" ]; then 
                    mkdir "$output_folder"
                    wait
                fi
                make -j4 RUN=$2 DIR=$run_dir

            else
                make compiler DIR=$run_dir
                make runner DIR=$run_dir
            fi
        fi
        ;;

    stop) 
        if [ -f ".start" ]; then 
            echo "Stopping..."
            touch ".stop" 
            sleep 3
            rm ".stop"
            rm ".start"
        else 
            echo "No process running."
        exit
        fi 
        ;;

    restart)
        if [ -f ".start" ]; then 
            echo "Stopping..."
            touch ".stop" 
            sleep 3
            rm ".stop"
        else 
            echo "No process running."
        fi 

        echo "Restarting..." 
        touch ".start" 
        if [ ! -z "$2" ]; then 
            make compiler DIR=$run_dir
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            make -j4 RUN=$2 DIR=$run_dir

        else
            make compiler DIR=$run_dir
            make runner DIR=$run_dir
        fi
        ;;
    
    start-soft)
        echo "[WARNING]: Code is not be re-compiled."
        echo "Configuration changes WILL be loaded" 
        if [ -f ".start" ]; then 
            echo "[WARNING] Process not stopped properly." 
            echo "Run './dirpi stop' to stop the process."
            echo "Exiting..."
        else 
            touch ".start"
            if [ ! -z "$2" ]; then 
                if [ ! -d "$output_folder" ]; then 
                    mkdir "$output_folder"
                    wait
                fi
            fi
            make soft-reset RUN=$2 DIR=$run_dir
        fi 
    ;;

    *) 
        echo "Nothing to do"
esac
