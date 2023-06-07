#!/bin/bash

RUN=$(tail -n 1 runlist.txt)
ID=$(head -n 1 metadata/ID.txt)
TARGET=128.111.19.32
CONFIG_PATH="pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/config/*ini"
SCHEDULE_PATH="$HOME/dirpi/config_templates/schedule.json"

# returns true if ping to TARGET succeeds
connected () {
  ping -c 1 $TARGET
  rc=$?
  if [ $rc -eq 0 ]; then
    return 1
  else
    return 0
  fi
}

# copies data from USB to cms server
copy_data () {
  N=200
  echo "Copying data to cms" 

  while [ $N -lt $RUN ]; do
    echo $N
    N=$((N+1))
    if [ -e "/media/dirpi4/4E79-9470/Run$N/" ]; then
      url="http://cms2.physics.ucsb.edu/cgi-bin/NextDiRPiRun?RUN=$N&ID=$ID"
      echo $url
      next=$(curl -s $url)
      echo  $next
      nextRun=$next #${next:64:4}
    #  echo "$ID $N  $nextRun \n" >> globalRuns.log 
      echo "copying DiRPi run $N to cmsX as $nextRun"
      rsync -r -z -c --remove-source-files "/media/dirpi4/4E79-9470/Run$N/" "pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/Run$nextRun"
      if [ !$ans ] ; then
        rm -r "/media/dirpi4/4E79-9470/Run$N/"
        echo "deleted"
      fi
      echo "$ID $N  $nextRun" >> globalRuns.log 
    fi
  done
}

# fetches fresh configuration files from cms, stores them as templates
fetch_configs () {
  scp $CONFIG_PATH config_templates/ 
}

# updates configuration schedule
update_schedule () {
  local curr_file=$(jq -r '.status | keys_unsorted[]' "$SCHEDULE_PATH")
  local curr_cnt=$(jq -r '.status."'$curr_file'"' "$SCHEDULE_PATH")
  local next_file=$curr_file
  local objects=$(jq -r '
                .frequency | 
                keys[] as $k | 
                "\($k), \(.[$k])"' $SCHEDULE_PATH)

  local index=$(jq --arg key $curr_file '.frequency[] |
                select(.template == $key) | 
                .order' $SCHEDULE_PATH)

  echo "File: $curr_file"
  echo "Count: $curr_cnt"
  echo $index

  if [ $curr_cnt -gt 0 ]; then 
    echo "checking if key exists"
    if [ -n $index ]; then
      echo "key exists"
      local max_cnt=$(jq --arg key $curr_file '.frequency[] |
                    select(.template == $key) | 
                    .repeats' $SCHEDULE_PATH)
      echo $max_cnt
      if [ $max_cnt -eq $curr_cnt ]; then 
        # repeat config, increment counter, no change
        ((curr_cnt++))
        updated_json=$(jq --arg key $curr_file \
                        --argjson value $curr_cnt '.status.key=$value' \
                        $SCHEDULE_PATH)
        echo "updated"
      else 
        # go to the next one 
        local next=$order+1
        local length=$(jq '.frequency | length' $SCHEDULE_PATH)
        if [ $next > $length ]; then 
        # end of list, return to top
        local init=$(jq '.frequency[] |
                      select(.order == 0) | .template' $SCHEDULE_PATH)
        updated_json=$(jq --argjson key $init \
                        '.status={ ($key):0 }' \
                        $SCHEDULE_PATH)
        echo $updated_json
        # move to next in order
        fi 
      fi
    fi
  fi
}

# main execution 
if [ connected ]; then
  echo "Connection to tau.physics.ucsb.edu active"
  #copy_data
  #fetch_configs
  update_schedule
else
  echo "Connection to tau.physics.ucsb.edu is down"
fi

