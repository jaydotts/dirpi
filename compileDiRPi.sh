#!/bin/sh
#
#set OPT = "-O3 -ffast-math -g0 "
#set OPT = "-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4"
g++ -Wno-deprecated -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -o DiRPi.o -c DiRPi.cc
echo "Linking DiRPi..."
g++ -Wno-deprecated -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 DiRPi.o -o DiRPi
