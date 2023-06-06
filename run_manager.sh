# Depracated -- Use dirpi.sh

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
