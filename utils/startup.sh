#!/bin/bash
#
#
# Startup protocol for dirpi - designed to run on boot. 
# Cleans up from previous runs, recovers crashes, checks run conditions
# 

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
        else 
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

cleanup