#!/bin/bash

dir=`pwd`
compression_point=$(echo '10^3' | bc)
abs_max=$(echo '10^11' | bc) 
output_folder="OUTPUTS"

echo $abs_max
rawdata_size=$(du --exclude='*.gz' $fullpath | awk '{ print $1}' | tail -1) # size in bytes
folder_size=$(du $fullpath | awk '{ print $1}' | tail -1) # size in bytes

moveTo_usb(){
    usb=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
    if [ ! -d "$usb" ]; then 
        echo "NO USB INSERTED - DATA NOT SAVED" 
        rm $1 
        return
    fi 

    echo "Copying to usb" 
    if [ ! -d "$usb/$output_folder" ]; then 
        mkdir "$usb/$output_folder"
    fi
	
	while [ -f "$usb/$output_folder/Data_$chunks_compressed.zip" ]; do
            ((chunks_compressed++))
    done

    mv $1 "$usb/$output_folder/Data_$chunks_compressed.zip"
} 

compress()
{

    echo "Txt files are above compression point of: $compression_point bytes"

    if [ "$folder_size" -gt "$abs_max" ]; then 
        echo "Max size reached!"
        # only save the pulses from the oldest files 
        # remove them
        return
    fi

    if [ $(ls $1/ | grep -c txt) -gt 10 ]; then 
        echo "Compressing: "

        # make sure not to overwrite existing zip files 
        while [ -f "$1/Data_$chunks_compressed.zip" ]; do 
            ((chunks_compressed++))
        done 

        mkdir "$1/Data_$chunks_compressed"

        for FILE in $(cd $1 && ls -tp *.txt | grep -v '/$' | tail -n +2 )
            do  
                mv $1/$FILE $1/Data_$chunks_compressed/
            done 

        zip -jr $1/Data_$chunks_compressed.zip $1/Data_$chunks_compressed/ 
        rm -rf $1/Data_$chunks_compressed/

    # move the files onto external drive if it exists

        moveTo_usb $1/Data_$chunks_compressed.zip
        ((chunks_compressed++))
    else 
        echo "Not enough files for compression" 
    fi
}

chunks_compressed=0
while [ true ]; do

    fullpath="$dir/$output_folder"
    echo $chunks_compressed
    rawdata_size=$(du --exclude='*.zip' $fullpath | awk '{ print $1}' | tail -1) # size in bytes
    folder_size=$(du $fullpath | awk '{ print $1}' | tail -1) # size in bytes

    echo "Size of $fullpath is $rawdata_size bytes"

    if [ "$rawdata_size" -gt "$compression_point" ]; then 
        compress $fullpath
    else
        echo "looks good"
    fi
    sleep 1

done
