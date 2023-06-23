#!/bin/bash
#
# Contains storage management protocols to be executed when network
# connection is available 
#
#
#
source "$HOME/dirpi/utils/_bash_utils.sh"
RUN=$(tail -n 1 runlist.txt)
ID=$(head -n 1 metadata/ID.txt)
TARGET=128.111.19.32
CONFIG_FOLDER="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi1/config"
CONFIG_PATH="$CONFIG_FOLDER/*ini"
SCHEDULE_PATH="$HOME/dirpi/config_templates/schedule.json"
DIRPI_DIR="$HOME/dirpi"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
CMSX_DIR="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$ID"

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

request_global_run(){
  local N=$1
  local ID=$2

  local url="http://cms2.physics.ucsb.edu/cgi-bin/NextDiRPiRun?RUN=$N&ID=$ID"
  # call curl with -f flag to get errors for missing/404/etc. 
  local next=$(curl -sf $url)
  local status=$?
  # skip if status > 0 (indicates error)
  if [ $status -gt 0 ] ; then
    echo ""
    return 
  fi
  echo $next
}

# calls send_node_data from node manager
send_node_data(){
  node1_dir=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node1_dir)
  node2_dir=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node2_dir)
  if [ -d "$node1_dir" ]; then
    copy_data $node1_dir
  fi 
  if [ -d "$node2_dir" ]; then 
    copy_data $node2_dir
  fi 
}

# copies data from USB to cms server
copy_data(){
  local SOURCE_DIR=$1
  for folder in "$SOURCE_DIR"/Run*; do

    folder_name="$(basename "$folder")"
    N="${folder_name#Run}"

    if [ "$(ls -A $SOURCE_DIR/$folder_name/)" ]; then 
      echo "Attempting to transfer run $N from $SOURCE_DIR/$folder_name"
      next_run=$(request_global_run $N $ID)
      echo "next run: $next_run"
      if [ "$next_run" ]; then 
        # check that next run is an integer
        re_int='^[0-9]+$'
        if ! [[ $next_run =~ $re_int ]] ; then
          echo "skipping $N: got non-integer value for next run: $next_run"
          continue
        fi
        
        # check that next run is greater than zero
        if ! [ $next_run -gt 0 ] ; then
          echo "skipping $N: got non-positive next_run: $next_run"
          continue
        fi
        
        # success - transfer the run files
        echo "Copying DiRPi run $N to cmsX as $next_run"
        sudo cp /etc/ssh/ssh_config_rsync /etc/ssh/ssh_config
        rsync -r -z -c --remove-source-files "$SOURCE_DIR/Run$N/" "$CMSX_DIR/Run$next_run"
        # add run to global run log
        echo "$ID $N  $next_run" >> globalRuns.log 
      fi 
    fi
  done
}

# fetches fresh configuration files from cms, stores them as templates
fetch_new_configs () {
  scp $CONFIG_PATH config_templates/ 
  scp "$CONFIG_FOLDER/schedule.json" config_templates/
}

upsert_configs () {
  echo "Upserting.."
  scp $SCHEDULE_PATH $CONFIG_FOLDER/json_response.json
}

############################################## main execution block
check_connection 
if [ $connected -eq 1 ]; then
  echo "Connection to tau.physics.ucsb.edu active"
  echo "Stopping DAQ."
  ./dirpi.sh stop
  run_with_timeout 300 copy_data $USB_DIR
  #fetch_new_configs
  #upsert_configs
  parachute
  run_with_timeout 600 send_node_data
  echo "Processes:"
  top -n 1 -b | head -15
else
  echo "Connection to tau.physics.ucsb.edu is down"
fi

# cleanup  
clean_sd || true