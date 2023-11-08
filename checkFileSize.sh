#!/bin/bash
ID=$(head -n 1 metadata/ID.txt)
dir="/home/dirpi$ID/dirpi"
run_num=$1
output_folder="Run$run_num"
fullpath="$dir/$output_folder"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
stop_runs=0

check_usb () { 

    stop_runs=0

    local min_space=1000000000
    local danger_zone=5000000000
    echo "Checking USB" 
    if [ ! -d $USB_DIR ]; then 
        echo "CRITICAL ERROR: USB NOT FOUND" 
        return
    fi 
    
#    local size=$(
#        du -bs $USB_DIR | awk '{print $1}'
#    )
    local space=$(
        df -B 1 --output=avail "$USB_DIR" | tail -n 1
    )
    
    if [ $space -lt $min_space ]; then 
        echo "[ERROR] Space is critical (<1GB). Pausing runs.."
        echo "Bytes available: $space"
        stop_runs=1
    elif [ $space -lt $danger_zone ]; then
        echo "[WARNING] Space is getting low. Removing old waveforms..."
        clean_usb
    else
        echo "Space OK" 
        echo "Bytes available: $space" 
    fi 
}

clean_usb () { 
    echo "Removing waveform and gz files"
    local old_data=$(find -name "$USB_DIR/*.gz" -o -name "$USB_DIR/*.drpw" -type f -mtime +10)
    local deleted_size=$(du -bs $old_data)

    if [ -z "$deleted_size" ]; then
        printf "Deleting the following:\n\n[SIZE]]\n$deleted_size\n"
        echo $old_data
        rm $old_data
    else 
        echo "Nothing to clean."
    fi 
}

moveTo_usb(){

    usb=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
    #usb="/home/struble/dirpi/tmpout"
    if [ ! -d "$usb" ]; then 
        echo "[WARNING] NO USB INSERTED - DATA NOT SAVED" 
        rm "$fullpath/$1"
        return
    fi 

    check_usb
    if [ $stop_runs -eq 1 ]; then 
        echo "[WARNING] Run data not saved to usb."
        rm "$fullpath/$1"
        return
    fi

    if [ ! -d "$usb/$output_folder" ]; then 
        sudo mkdir "$usb/$output_folder"
	sudo cp $dir/config/config.ini $usb/$output_folder/
    fi

    fnum=`echo $1 | awk -F'.' '{print $1}' | awk -F'_' '{print $2}'`
    echo "fnum=$fnum"
    cd $fullpath

    if [ $fnum -lt 2 ]; then 
        gzip -c Run${run_num}_${fnum}.txt > $usb/$output_folder/Run${run_num}_${fnum}.txt.gz
    fi 
    nice ${dir}/DiRPi lossycompress $run_num $fnum
    sudo mv Run${run_num}_${fnum}.drpw $usb/$output_folder/Run${run_num}_${fnum}.drpw
    nice ${dir}/DiRPi savepulses $run_num $fnum
    sudo mv Run${run_num}_${fnum}.drp $usb/$output_folder/Run${run_num}_${fnum}.drp

#sudo nice mv Run${run_num}_${fnum}.drpw $usb/$output_folder/Run${run_num}_${fnum}.drpw
    #start of changes that ruin analysis, commented for now
    # Turn off the pulse-only files to make sure we keep up with data taking
    #nice ${dir}/DiRPi savepulses $run_num $fnum
    #sudo nice mv Run${run_num}_${fnum}.drp $usb/$output_folder/Run${run_num}_${fnum}.drp
    #grep "^T_" Run${run_num}_${fnum}.txt > $usb/$output_folder/Run${run_num}_${fnum}.livedead
    cd -
    rm "$fullpath/$1"
} 

compress()
{
    if [ $(ls $1/ | grep -c txt) -lt 3 ]; then 
        # while waiting for new files sleep to save cpu time
        sleep 3
    fi
    if [ $(ls $1/ | grep -c txt) -gt 2 ]; then 

        for FILE in $(cd $1 && ls -trp *.txt | grep -v '/$' | head -n +1)
            do  
                moveTo_usb $FILE 
            done 
    fi
}

if [ ! -d $fullpath ]; then 
    exit
fi 

while [ ! -f ".stop" ]; do
    compress $fullpath
done

for FILE in $(cd $fullpath && ls -trp *.txt | grep -v '/$' | head --lines -0)
    do  
        moveTo_usb $FILE 
    done 

cp run.log "$USB_DIR/$output_folder/"
> run.log
rm -rf $fullpath
