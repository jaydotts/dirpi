#!/bin/bash

cd /home/dirpi4/digi_refactor

output_folder="Run"$2

kill_recurse() {
    cpids=`pgrep -P $1|xargs`
    for cpid in $cpids;
    do
        kill_recurse $cpid
    done
    echo "killing $1"
    kill -9 $1
}

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
            make runner
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
        fi
        ;;

    *) 
        echo "Nothing to do"
esac
