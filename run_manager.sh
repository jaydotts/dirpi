#!/bin/bash

PIDFILE="/home/dirpi4/dirpi/run.pid"
STORAGE_THRESHOLD=90

create_pidfile () {
  echo $$ > "$PIDFILE"
}

remove_pidfile () {
  [ -f "$PIDFILE" ] && rm "$PIDFILE"
}

previous_instance_active () {
  local prevpid
  if [ -f "$PIDFILE" ]; then
    prevpid=$(cat "$PIDFILE")
    kill -0 $prevpid 
  else 
    false
  fi
}

start_DAQ () {
  date +"PID: $$ Action started at %H:%M:%S"
  ./testFile.sh
  date +"PID: $$ Action finished at %H:%M:%S"
}

check_network(){
  RUN = $(tail -n 1 runlist.txt)
  ID=$(tail -n 1 ID.txt)
  ping -c 1 128.111.19.32
  rc=$?
  if [[ rc -eq 0 ]] ; then
    echo 'connection to tau.physics.ucsb.edu active'
    next=$(curl -d ID="2" -d Run="$runNumber"  http://cms2.physics.ucsb.edu/cgi-bin/NextRun)
    nextRun=${next:64:4}
    echo 'The next run number is '$nextRun
    #write to log of IP address, dirpiRun, globalRun
    N=0
    while [ $N -lt $RUN ]
    do
      echo $N
      N=$((N+1))
      if [ -e /media/diripi4/media/dirpi4/4E79-9470/Run$N/ ] ; then
      	next=$(curl -d ID="2" -d Run="$runNumber"  http://cms2.physics.ucsb.edu/cgi-bin/NextRun)
      	nextRun=${next:64:4}
      	rsync -r -z -c --remove-source-files /media/dirpi4/4E79-9470/Run$N/ pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/Run$nexRun
        "$ID $N  $nextRun" >> globalRuns.log 
      fi
    done
  else
    echo'connection to tau.physics.ucsb.edu down'
  fi
}

#don't actually need these, rsync can handle this for us with --remove-source-files
#check_storage(){
#   used=$(df -h $1 | awk '{print $5}' | tail -1 )
#   used_pct=`echo ${used:0:-1}` 
#   free_pct=$((100-$used_pct))
#   if [$free_pct <= STORAGE_THRESHOLD]; then
#   # rm /media/dirpi4/4E79-9470/Run*/*.drpw probably a bit aggressive
#}

#remove_old_runs(){
 #  RUN = $(tail -n 1 runlist.txt)
 #  N =0
 #  while $N < $RUN-215
 #  do 
 #    rsync -r -z -c --remove-source-files /media/dirpi4/4E79-9470/Run$N/ pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/Run$N
  # done
  #for runs on Dirpi older than current run
   #check against log for global run number
   #check for changing file size for global runNumber
   #no change->delete 
   #change ->keep
#}

if previous_instance_active
then 
  date +'PID: $$ Previous instance is still active at %H:%M:%S, aborting ... '
else 
trap remove_pidfile EXIT
create_pidfile
start_DAQ
remove_pidfile
check_network
fi
