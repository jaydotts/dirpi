# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 
DIR = $(shell pwd)

all: runner manager plotter

compiler: 
	$(CC) $(CFLAGS ) src/*.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner: main
	$(DIR)/main config/config.ini $(RUN)

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	$(DIR)/checkFileSize.sh $(RUN)

plotter: 
	python3 gui.py

soft-reset:
	make -j2 runner manager RUN=$(RUN)

cleanup: 
	./checkFileSize $(RUN)
	@touch ".stop"
	@sleep 2
	@rm ".stop"

test: 
	$(CC) $(CFLAGS ) unit_test/src/*.cpp -o test