#!/bin/bash

dir=`pwd`
compression_point=1000
abs_max=100000000
USB_DIRECTORY="$dir/Dummy"

fullpath="$dir/Dummy"

rawdata_size=$(du -I '*.gz' $fullpath | awk '{ print $1}' | tail -1) # size in bytes
folder_size=$(du $fullpath | awk '{ print $1}' | tail -1) # size in bytes

compress()
{

    echo "Txt files are above compression point of: $compression_point bytes"

    if [ "$folder_size" -gt "$abs_max" ]; then 
        echo "Max size reached!"
        # only save the pulses from the oldest files 
        # remove them
        exit 
    fi

    if [ $(ls $1/ | grep -c txt) -gt 10 ]; then 
        echo "Compressing: "
        mkdir "$1/Data_$chunks_compressed"
        for FILE in $(cd $1 && ls -tp *.txt | grep -v '/$' | tail -n +2 )
            do  
                mv $1/$FILE $1/Data_$chunks_compressed/
            done 

        zip -jr $1/Data_$chunks_compressed.zip $1/Data_$chunks_compressed/ 
        rm -rf $1/Data_$chunks_compressed/

    # move the files onto external drive if it exists
        if [ -d $USB_DIRECTORY ]; then 
            # move compressed .gz to external drive
            echo "Moving somewhere else..."
        fi 

        ((chunks_compressed++))
    else 
        echo "Not enough files for compression" 
    fi
}

echo "Size of $fullpath is $rawdata_size bytes"

chunks_compressed=0
while [ true ]; do

    fullpath="$dir/Dummy"

    rawdata_size=$(du -I '*.gz' $fullpath | awk '{ print $1}' | tail -1) # size in bytes
    folder_size=$(du $fullpath | awk '{ print $1}' | tail -1) # size in bytes

    echo "Size of $fullpath is $rawdata_size bytes"
    #echo "Chunks compressed: $chunks_compressed"
    if [ "$rawdata_size" -gt "$compression_point" ]; then 
        compress $fullpath
    else
        echo "looks good"
    fi
    sleep 1

#    if [ "$chunks_compressed" -gt 2 ]; then 
#        exit 
#    fi
done