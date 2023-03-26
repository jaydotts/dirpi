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

# then pass args to the makefile using make $output_folder