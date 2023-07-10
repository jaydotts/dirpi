#!/bin/bash
#
# Contains storage management protocols to be executed when network
# connection is available 
#
#
#
source "$HOME/dirpi/utils/_bash_utils.sh"
RUN=$(tail -n 1 $HOME/dirpi/runlist.txt)
ID=$(head -n 1 $HOME/dirpi/metadata/ID.txt)
TARGET="128.111.19.32"
CONFIG_FOLDER="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$ID/config"
CONFIG_PATH="$CONFIG_FOLDER/*ini"
SCHEDULE_PATH="$HOME/dirpi/config_templates/schedule.json"
DIRPI_DIR="$HOME/dirpi"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)

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
        echo "Attempting to pull" >> config_templates/pull_status.txt
        git fetch --all
        git reset --hard origin/v3 &> config_templates/pull_status.txt
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
  # make sure nodes are setup 
  # source $HOME/dirpi/utils/node_manager.sh; setup_nodes
  local node1_dir=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node1_dir)
  local node1_id=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node1_id)
  local node2_dir=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node2_dir)
  local node2_id=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node2_id)
  local node3_dir=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node3_dir)
  local node3_id=$(. "$HOME/dirpi/utils/node_manager.sh"; echo $node3_id)
  
  echo "NODE 1 DIR: $node1_dir"
  echo "NODE 2 DIR: $node2_dir"
  echo "NODE 3 DIR: $node3_dir"
  
  if [ -d "$node1_dir" ]; then
    copy_data $node1_dir $node1_id
  fi 
  if [ -d "$node2_dir" ]; then 
    copy_data $node2_dir $node2_id
  fi 
  if [ -d "$node3_dir" ]; then 
    copy_data $node3_dir $node3_id
  fi 
}

# copies data from USB to cms server
copy_data(){
  if [[ ! -n "$1" || ! -n "$2" ]]; then 
    echo "ERROR: source directory and ID must be passed. "
    return 
  fi 
  local SOURCE_DIR=$1
  local ID=$2
  local CMSX_DIR="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$ID"


  for folder in $SOURCE_DIR/Run*; do

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
        rmdir "$SOURCE_DIR/Run$N/"
        # add run to global run log
        echo "$ID $N  $next_run" >> globalRuns.log 
      fi 
    fi
  done
}

fetch_node_configs() {
  local node1_id=$(cat "$HOME/dirpi/nodes/node1/ID.txt")
  local node2_id=$(cat "$HOME/dirpi/nodes/node2/ID.txt")
  local node3_id=$(cat "$HOME/dirpi/nodes/node3/ID.txt")
  
  local node=0
  for id in $node1_id $node2_id $node3_id
  do 
    ((node++))
    echo "fetching DPID $id configs"
    local CONFIG_FOLDER="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi$id/config/*"
    scp -r $CONFIG_FOLDER "$HOME/dirpi/nodes/node$node/configs"
  done 
}

# fetches fresh configuration files from cms, stores them as templates
fetch_new_configs () {
  
  fetch_node_configs 
  
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
  if [ ! -f ".on_network" ]; then 
    touch ".on_network"
    echo "Connection to tau.physics.ucsb.edu active"
    echo "Stopping DAQ."
    $HOME/dirpi/dirpi.sh stop
    parachute
    fetch_node_configs
    copy_data $USB_DIR $ID
    fetch_new_configs
  #  upsert_configs
    send_node_data
    echo "Processes:"
    top -n 1 -b | head -15
    rm ".on_network"
  fi 
else
  echo "Connection to tau.physics.ucsb.edu is down"
fi

# cleanup  
clean_sd || true
rm ".on_network" || true
