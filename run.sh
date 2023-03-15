usage="$(basename "$0") [-h] [-c CONFIG_FILE] [-f OUTPUT_FOLDER]
Run XRD digitizer data collection using config file located at CONFIG_FILE
Run data is placed in OUTPUT_FOLDER and dynamically compressed/moved when the output 
folder reaches a critical size. 

    -h  show this help text
    -f  path to data output folder  
    -c  path to configuration file 
"
options=':f:c:'
while getopts $options option; do
  case "$option" in
        h) echo "$usage"; exit;;
        f) output_folder=${OPTARG};;
        c) config_file=${OPTARG};;
        :) printf "missing argument for -%s\n" "$OPTARG" >&2; echo "$usage" >&2; exit 1;;
        \?) printf "illegal option: -%s\n" "$OPTARG" >&2; echo "$usage" >&2; exit 1;;
    esac
done

if [ ! "$output_folder" ] || [ ! "$config_file" ]; then
  echo "arguments -f and -c must be provided"
  echo "$usage" >&2; exit 1
fi

dir=`pwd`

echo "OUTPUT_FOLDER: $output_folder";
echo "CONFIG: $config_file";

fpath="$dir/$output_folder"

if [ -d "$output_folder" ]; then
    echo "WARNING: Folder $output_folder already exists - data may be overwritten!"
    while true; do
    read -p "Proceed? [y/n]" yn
    case $yn in
        [Yy]* ) make install; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

else 
    echo "Creating directory $output_folder at $fpath";
    mkdir $fpath;
fi


################# FUNCTIONS 
check_size()
{
    echo $1
    mfs=$(du $1 | awk '{ print $1}' | tail -1) # size in bytes
    max=10000
    dir=`pwd`

    echo "Checking size of $1 : $mfs"

    if [ "$mfs" -gt "$max" ]; then # check that file is not above critical size 

        echo "Unloading"
        count=0

        while [ -f "$1/overflow_$count.zip" ]; do
            ((count++))
        done 

        echo $count
        mkdir "$1/overflow_$count";

        # list all data in destination directory, move all but current file into overflow
        (cd $1 && ls -tp *.txt | grep -v '/$' | tail -n +1 |  xargs  -i  mv  '{}'  "$1/overflow_$count")
        zip -jr "$1/overflow_$count.zip" "$1/overflow_$count"
        rm -rf "$1/overflow_$count"

    fi
}

export -f check_size 
watch_file(){
    watch -n 5 -x bash -c "check_size $1"
}

#python3 plot.py

#################### PROGRAM EXECUTION 
make clean 
make main

./main $config_file $output_folder & 
watch_file $fpath 


echo 'done'