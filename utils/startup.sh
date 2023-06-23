#!/bin/bash
#
#
# Startup protocol for dirpi - designed to run on boot. 
# Cleans up from previous runs, recovers crashes, checks run conditions
# 
source "$HOME/dirpi/utils/_bash_utils.sh"

cleanup(){
    if [ -f "run.pid" ]; then 
        echo "Previous run did not exit properly. Cleaning up..."
        rm "run.pid" 
        echo "Run log: "
        echo cat run.log
    fi 

    echo "Removing hanging files..."
    for folder in Run*; do
        if [[ -d "$folder" && $(find "$folder" -maxdepth 1 -type f | wc -l) -lt 10 ]]; then
        rm -rf "$folder"
        echo "Removed folder: $folder"
        elif [ -d "$folder" ]; then
            echo "$folder has too many files. Archiving..." 
            if [ -d $usb ]; then 
                tar czf "$usb/$folder.tar.gz" -C "$folder" .
            else 
                echo "No USB inserted. Deleting." 
                rm -rf "$folder"
            fi
        fi
    done
}

setup_interfaces(){
    echo "Setting up..."
    source $HOME/dirpi/utils/manage_nodes.sh ping_nodes > /dev/null
    if [ $active_nodes -eq 2 ]; then 
        echo "Checking Nodes - [OK]"
    else 
        echo "Checking Nodes - [FAILED]"
        echo "$active_nodes" Nodes found.
    fi
}   

compile_DAQ(){
    echo "Compiling DAQ code..."
    run_with_timeout 60 make compiler 
}

compile_compression(){
    echo "Compiling compression code..."
    run_with_timeout 60 make compression
}

start(){
    cleanup

    # recompile source code 
    make clean 
    compile_compression 
    compile_DAQ

    # setup nodes 
    source "$HOME/dirpi/utils/node_manager.sh"; setup_nodes
}

start