# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 
DIR = $(shell pwd)

all: runner manager

compiler: 
	$(CC) $(CFLAGS ) src/*.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner: main
	$(DIR)/main config/config.ini $(RUN) 100

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	$(DIR)/checkFileSize.sh $(RUN)

plotter: 
	python3 gui.py

soft-reset:
	make -j3 runner manager plotter RUN=$(RUN)

cleanup: 
	./checkFileSize $(RUN)
	@touch ".stop"
	@sleep 2
	@rm ".stop"

test: 
	$(CC) $(CFLAGS ) src/*.cpp unit_test/src/*.cpp -o test -lwiringPi

monitor: 
	$(DIR)/main
