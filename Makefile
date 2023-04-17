# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 

all: runner manager plotter

compiler: 
	$(CC) $(CFLAGS ) /src/*.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner: main
	/home/dirpi4/digi_refactor/main config/digi.config

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	/home/dirpi4/digi_refactor/checkFileSize.sh

plotter: 
	python3 plotter.py
