#!/bin/bash

RUN=$(tail -n 1 runlist.txt)
ID=$(head -n 1 metadata/ID.txt)
TARGET=128.111.19.32
CONFIG_FOLDER="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$ID/config"
CONFIG_PATH="$CONFIG_FOLDER/*ini"
SCHEDULE_PATH="/home/dirpi$ID/dirpi/config_templates/schedule.json"
DIRPI_DIR="/home/dirpi$ID/dirpi"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
CMSX_PATH="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$ID"

clean_sd () {
    local minimum_space=10000000000
    local size=$(
        du -bs $DIRPI_DIR | awk '{print $1}'
    )
    local space=$(
        df -B 1 --output=avail "$DIRPI_DIR" | tail -n 1
    )
    if [ $space -lt $minimum_space ]; then 
        echo "WARNING: MORE THAN HALF OF SD STORAGE EXCEEDED" 
        echo "Details:" 
        echo $(du -h --max-depth=1)
    fi 

    echo "Cleaning dirpi directory..."

    # List directories that start with "Run*" and are older than 24 hours based on file creation date
    local old_runs=$(find -name "Run*" -type d -mtime +0)
    local deleted_size=$(du -bs $old_runs)

    if [ -z "$deleted_size" ]; then
        printf "Deleting the following:\n\n[SIZE]  [DIRECTORY]\n$deleted_size\n"
        rm -rf $old_runs
    fi 

    # clean out run list and global run logs 
    sed -i -e :a -e '$q;N;10,$D;ba' $DIRPI_DIR/runlist.txt
    sed -i -e :a -e '$q;N;10,$D;ba' $DIRPI_DIR/globalRuns.log
    echo "Directory is clean. $size bytes available"

    }

parachute () {
    if [ -f "config_templates/emergency_pull.ini" ]; then
        cd $DIRPI_DIR
        sudo cp /etc/ssh/ssh_config_git /etc/ssh/ssh_config
        git fetch --all
        git reset --hard origin/auto_run
        rm "config_templates/emergency_pull.ini"
        sudo cp /etc/ssh/ssh_config_rsync /etc/ssh/ssh_config
    fi 
}

# returns true if ping to TARGET succeeds
check_connection () {
  ping -c 1 $TARGET > /dev/null
  rc=$?
  if [ $rc -eq 0 ]; then
    connected=1
  else
    connected=0
  fi
}

# copies data from USB to cms server
copy_data () {
  N=0
  echo "Copying data to cms" 

  while [ $N -lt $RUN ]; do
    #echo $N
    N=$((N+1))
    if [ -e "$USB_DIR/Run$N/" ]; then
      url="http://cms2.physics.ucsb.edu/cgi-bin/NextDiRPiRun?RUN=$N&ID=$ID"
      echo $url
      next=$(curl -s $url)
      nextRun=$next #${next:64:4}
    #  echo "$ID $N  $nextRun \n" >> globalRuns.log 
      echo "Copying DiRPi run $N to cmsX as $nextRun"
      rsync -r -z -c --remove-source-files "$USB_DIR/Run$N/" "$CMSX_PATH/Run$nextRun"
      if [ $? ] ; then
        echo "Runs moved to cmsX successfully. Deleting..."
        rmdir "$USB_DIR/Run$N/"
      fi
      echo "$ID $N  $nextRun" >> globalRuns.log 
    fi
  done
}

# fetches fresh configuration files from cms, stores them as templates
fetch_configs () {
  scp $CONFIG_PATH config_templates/ 
  scp "$CONFIG_FOLDER/schedule.json" config_templates/
}

# updates configuration schedule

update_configs(){
  #cp $SCHEDULE_PATH metadata/prev_schedule.txt
  local index_file="config/_status.ini"
  local index=$(awk -F "=" '/index/ {print $2}' $index_file | tr -d ' ')

  # read the schedule 
  local config_arr=()
  filename="config/schedule.txt"
  if [ ! -f $index_file ]; then 
      touch $index_file
      echo "index=0" > $index_file; 
  fi
 # load config file list into array
  while IFS= read -r line
  do
    config_arr+=("$line")
  done < "$filename"

  # return if it is empty
  if [ ! -n "${#config_arr[@]}" ]; then 
    echo "schedule is empty. Exiting."
    return
  fi
  current_file=${config_arr[$index]}
  echo "CURRENT FILE: $current_file"
  next_index=$((index+1))
  next_file=${config_arr[$next_index]}
  echo "NEXT FILE: $next_file"

  # copy file at current index
  if [ -f "config/$current_file" ]; then 
    cp "config/$current_file" "config/config.ini"
  else
    echo "WARNING: $current_file not in config folder."
  fi 

  if [ ! -n "$next_file" ];then 
    echo "NOTE: ${config_arr[$index]} is last file in schedule"
    next_file=${config_arr[0]}
    next_index=0
  fi 
  # advance the index
  echo "index=$next_index" > "config/_status.ini"
}


upsert_configs () {
  echo "Upserting.."
  scp $SCHEDULE_PATH $CONFIG_FOLDER/json_response.json
}

# main execution 
check_connection 
if [ $connected -eq 1 ]; then
  echo "Connection to tau.physics.ucsb.edu active"
  parachute
  copy_data
  fetch_configs
  update_configs
  

#  echo "Processes:"
#  top -n 1 -b | head -15
  
else
  echo "Connection to tau.physics.ucsb.edu is down"
  update_configs
fi

# cleanup  
clean_sd || true

echo "Run cycle complete. Waiting for next run..."
