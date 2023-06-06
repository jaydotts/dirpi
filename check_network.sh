#!/bin/bash

RUN=$(tail -n 1 runlist.txt)
ID=$(head -n 1 ID.txt)
ping -c 1 128.111.19.32
rc=$?

if [ $rc -eq 0 ]; then
  echo "Connection to tau.physics.ucsb.edu active"

  N=200
  while [ $N -lt $RUN ]; do
    echo $N
    N=$((N+1))
    if [ -e "/media/dirpi4/4E79-9470/Run$N/" ]; then
      url="http://cms2.physics.ucsb.edu/cgi-bin/NextDiRPiRun?RUN=$N&ID=$ID"
      echo $url
      next=$(curl -s $url)
      echo  $next
      nextRun=$next #${next:64:4}
   #   echo "foo\n" >> globalRuns.log
    #  echo "$ID $N  $nextRun \n" >> globalRuns.log 
      echo "copying DiRPi run $N to cmsX as $nextRun"
      rsync -r -z -c --remove-source-files "/media/dirpi4/4E79-9470/Run$N/" "pmfreeman@tau.physics.ucsb.edu:/net/cms26/cms26r0/pmfreeman/XRD/DiRPi_v3/dirpi4/Run$nextRun"
      rm -r "/media/dirpi4/4E79-9470/Run$N/"
      echo "$ID $N  $nextRun" >> globalRuns.log 
    fi
  done
else
  echo "Connection to tau.physics.ucsb.edu down"
fi
