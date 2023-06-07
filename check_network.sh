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
  updated_json=$(jq '' $SCHEDULE_PATH)

  local objects=$(jq -r '
                .frequency | 
                keys[] as $k | 
                "\($k), \(.[$k])"' $SCHEDULE_PATH)

  local index=$(jq --arg key $curr_file '.frequency[] |
                select(.template == $key) | 
                .order' $SCHEDULE_PATH)

  echo "File: $curr_file"
  echo "Count: $curr_cnt"
  echo "Current index: $index"

  echo "checking if key exists"
  if [[ -n $index ]]; then
    echo "key exists"
    local max_cnt=$(jq --arg key $curr_file '.frequency[] |
                  select(.template == $key) | 
                  .repeats' $SCHEDULE_PATH)
    if [ $max_cnt -gt $curr_cnt ]; then 
      echo "advancing counter..."
      # repeat config, increment counter, no change
      ((curr_cnt++))
      updated_json=$(jq --arg key $curr_file \
                      --argjson value $curr_cnt '.status[$key]=$value' \
                      $SCHEDULE_PATH)
    else 
      echo "Moving to next config file"
      # go to the next one in order
      ((index++))
      local length=$(jq '.frequency | length' $SCHEDULE_PATH)
      echo "next index $index"
      echo "length $length"
      if [ $index -eq $length ]; then 
      echo "end of list" 
        # end of list, return to top
        local init=$(jq '.frequency[] |
                      select(.order == 0) | .template' $SCHEDULE_PATH)
        echo "Returning to $init"
        updated_json=$(jq --argjson key $init \
                        '.status={ ($key):1 }' \
                        $SCHEDULE_PATH)
      else 
        # move to next in order
        local next_file=$(jq --argjson order $index \
                        '.frequency[] |
                        select(.order == $order) | 
                        .template' $SCHEDULE_PATH)
        echo "New file: $next_file"
        updated_json=$(jq --argjson key $next_file \
                      '.status={ ($key):1 }' \
                      $SCHEDULE_PATH)
      fi 
    fi
  else 
    echo "key $curr_file does not exist. Resetting."
    local init=$(jq '.frequency[] |
              select(.order == 0) | .template' $SCHEDULE_PATH)
    updated_json=$(jq --argjson key $init \
          '.status={ ($key):1 }' \
          $SCHEDULE_PATH)
    echo $updated_json > tmp && mv tmp $SCHEDULE_PATH
  fi
}

update_configs () { 
  update_schedule
  local new_configs=$(echo $updated_json | jq '.status | keys_unsorted[0]' | tr -d '"')
  local target=$HOME/dirpi/config_templates/$new_configs
  if [ -f "$HOME/dirpi/config_templates/$new_configs" ]; then
    echo "file found. copying to config.ini.." 
    cp $target "$HOME/dirpi/config/config.ini"
    echo "done. overwriting json" 
    echo $updated_json > tmp && mv tmp $SCHEDULE_PATH
  else 
    echo "Config file not in templates."
    echo $updated_json > tmp && mv tmp $SCHEDULE_PATH
  fi
}


upsert_configs () {
  echo "Upserting.."
}

# main execution 
if [ connected ]; then
  echo "Connection to tau.physics.ucsb.edu active"
  #copy_data
  #fetch_configs
  update_configs
  upsert_configs
else
  echo "Connection to tau.physics.ucsb.edu is down"
fi

