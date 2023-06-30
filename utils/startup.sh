#!/bin/bash
#
#
# Startup protocol for dirpi - designed to run on boot. 
# Cleans up from previous runs, recovers crashes, checks run conditions
# 
source "$HOME/dirpi/utils/_bash_utils.sh"
nodes_active=2 # number of nodes we are using

cleanup(){
    if [ -f "run.pid" ]; then 
        echo "Previous run did not shut down properly. Cleaning up..."
        rm "run.pid" 
        echo "Run log: "
        echo cat run.log
    fi 

    echo "Pruning dirpi directory..."
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
    echo "Setting up interfaces..."
    source $HOME/dirpi/utils/manage_nodes.sh ping_nodes > /dev/null
    if [ $active_nodes -eq $nodes_active ]; then 
        echo "Checking Nodes - [OK]"
    else 
        echo "Checking Nodes - [FAILED]"
        echo "$active_nodes Nodes found. Expected: $nodes_active"
    fi
}   

compile_DAQ(){
    echo "Compiling DAQ code..."
    make compiler 
}

compile_compression(){
    echo "Compiling compression code..."
    make compression
}

start(){
    cleanup

    # recompile source code 
    make clean 
    compile_compression 
    compile_DAQ

    # setup nodes 
    sudo source "$HOME/dirpi/utils/node_manager.sh"; setup_nodes
}

start