#!/bin/sh
#
set OPT = "-O3 -ffast-math -g0 "
g++ -Wno-deprecated $OPT  -o DiRPi.o -c DiRPi.cc
echo "Linking DiRPi..."
g++ -Wno-deprecated $OPT DiRPi.o -o DiRPi
