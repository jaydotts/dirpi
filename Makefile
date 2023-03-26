# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 

all: compiler runner manager 

compiler: 
	$(CC) $(CFLAGS ) main.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner: main
	/home/dirpi0/ucsb-digitizer/main config/digi.config

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	/home/dirpi0/ucsb-digitizer/checkFileSize.sh
