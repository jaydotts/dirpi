#!/bin/bash

output_folder="Run"$2

case $1 in 
    start)
        if [ ! -z "$2" ]; then 
            make compiler 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
            make -j4 RUN=$2

        else
            make compiler
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
        
        if [ ! -z "$2" ]; then 
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
        if [ ! -z "$2" ]; then 
            if [ ! -d "$output_folder" ]; then 
                mkdir "$output_folder"
                wait
            fi
        fi
        make soft-reset RUN=$2
    ;;

    *) 
        echo "Nothing to do"
esac
