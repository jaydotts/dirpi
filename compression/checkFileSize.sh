#!/bin/bash

dir=`pwd`
run_num=$1
output_folder="Run$run_num"
fullpath="$dir/$output_folder"

moveTo_usb(){

    existing_files=0

    usb=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
    #usb="/home/struble/dirpi/tmpout"
    if [ ! -d "$usb" ]; then 
        echo "NO USB INSERTED - DATA NOT SAVED" 
        rm "$fullpath/$1"
        return
    fi 

    if [ ! -d "$usb/$output_folder" ]; then 
        sudo mkdir "$usb/$output_folder"
    fi

    # do this so files are not overwritten if there is a numbering mismatch
	while [ -f $usb/$output_folder/$output_folder"_"$existing_files".txt.gz" ]; do
            ((existing_files++))
    done

    fnum=`echo $1 | awk -F'.' '{print $1}' | awk -F'_' '{print $2}'`
    echo "fnum=$fnum"
    cd $fullpath
    nice ${dir}/DiRPi lossycompress $run_num $fnum
    nice ${dir}/DiRPi savepulses $run_num $fnum
    sudo mv Run${run_num}_${fnum}.drpw $usb/$output_folder/Run${run_num}_${fnum}.drpw
    sudo mv Run${run_num}_${fnum}.drp $usb/$output_folder/Run${run_num}_${fnum}.drp
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

chunks_compressed=0
if [ ! -d $fullpath ]; then 
    exit
fi 

while [ ! -f ".stop" ]; do
    compress $fullpath
done

for FILE in $(cd $fullpath && ls -trp *.txt | grep -v '/$' | head)
    do  
        moveTo_usb $FILE 
    done 

rm -rf $fullpath
