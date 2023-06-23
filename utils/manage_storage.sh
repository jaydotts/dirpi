#!/bin/bash
#
# Contains storage management protocols to be executed with and without internet. 
#
#
#
RUN=$(tail -n 1 runlist.txt)
ID=$(head -n 1 metadata/ID.txt)
TARGET=128.111.19.32
CONFIG_FOLDER="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/config"
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

# copies data from USB to cms server
copy_data () {
  N=0
  echo "Copying data to cms" 

  while [ $N -lt $RUN ]; do
    #echo $N
    N=$((N+1))

    # only proceed if run directory exists, not just main usb directory
    # if [ -e "$USB_DIR" ]; then
    if [ -e "$USB_DIR/Run$N/" ]; then
      
      # if the run directory isn't empty, try to transfer run files
      if [ "$(ls -A $USB_DIR/Run$N/)" ]; then
        echo "attempting to transfer run $N from $USB_DIR/Run$N/"
        
        # request a global run number for this run
        url="http://cms2.physics.ucsb.edu/cgi-bin/NextDiRPiRun?RUN=$N&ID=$ID"
        echo $url
        # call curl with -f flag to get errors for missing/404/etc. 
        next=$(curl -sf $url)
        status=$?

        # skip if status > 0 (indicates error)
        if [ $status -gt 0 ] ; then
          echo "skipping $N: curl returned nonzero status: $status"
          continue
        fi
        
        # extract nextRun from curl result
        nextRun=$next #${next:64:4}
        #echo "$ID $N  $nextRun \n" >> globalRuns.log 
        
        # check that nextRun is an integer
        re_int='^[0-9]+$'
        if ! [[ $nextRun =~ $re_int ]] ; then
          echo "skipping $N: got non-integer value for nextRun: $nextRun"
          continue
        fi
        
        # check that nextRun is greater than zero
        if ! [ $nextRun -gt 0 ] ; then
          echo "skipping $N: got non-positive nextRun: $nextRun"
          continue
        fi
        
        # success - transfer the run files
        echo "Copying DiRPi run $N to cmsX as $nextRun"
        sudo cp /etc/ssh/ssh_config_rsync /etc/ssh/ssh_config
        rsync -r -z -c --remove-source-files "$USB_DIR/Run$N/" "$CMSX_DIR/Run$nextRun"

        # add run to global run log
        echo "$ID $N  $nextRun" >> globalRuns.log 
        
      fi

      # if the directory is empty now, remove it
      if [ "$(ls -A $USB_DIR/Run$N/)" ]; then
        echo "directory $USB_DIR/Run$N/ not empty - transfer may have failed"
      else
        echo "directory $USB_DIR/Run$N/ is empty; deleting it"
        rmdir "$USB_DIR/Run$N/"
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

# usage: timeout <n_seconds> <function_name>
timeout() {
	local cmd_pid sleep_pid retval
	(shift; "$@") &   # shift out sleep value and run rest as command in background job
	cmd_pid=$!
	(sleep "$1"; kill "$cmd_pid" 2>/dev/null) &
	sleep_pid=$!
	wait "$cmd_pid"
	retval=$?
	kill "$sleep_pid" 2>/dev/null
	return "$retval"
}

############################################## main execution block
check_connection 
if [ $connected -eq 1 ]; then
  echo "Connection to tau.physics.ucsb.edu active"
  timeout 10 copy_data
  fetch_new_configs
  upsert_configs
  parachute

  echo "Processes:"
  top -n 1 -b | head -15
  
else
  echo "Connection to tau.physics.ucsb.edu is down"
fi

# cleanup  
clean_sd || true