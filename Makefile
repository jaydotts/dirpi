# Makefile

CC = g++
CFLAGS = -O 
DIR = $(shell pwd)
FILES_PER_RUN = $(shell source utils/_bash_utils.sh && parse_config "config/config.ini" "files_per_run" 100)
SHELL := /bin/bash
COMPSN_OPTS = -O3 -ffast-math -g0 
PLOT_BUFFER = plot_buffer.txt

#all: runner manager

compiler:
	$(CC) $(CFLAGS ) src/*.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner:
	$(DIR)/main config/config.ini $(RUN) $(FILES_PER_RUN)

manager: 
	./utils/run_compression.sh $(RUN)

plotter: 
	python3 gui.py $(PLOT_BUFFER)

soft-reset:
	make -j3 runner manager plotter RUN=$(RUN)

monitor: 
	$(DIR)/main

DAQ:
	taskset -c 0 $(DIR)/main config/config.ini $(RUN) $(FILES_PER_RUN) & taskset -c 1 ./utils/run_compression.sh $(RUN)

compression: compression/DiRPi
	g++ -Wno-deprecated $(OPT)  -o compression/DiRPi.o -c compression/DiRPi.cc
	g++ -Wno-deprecated $(OPT) compression/DiRPi.o -o compression/DiRPi

clean: 
	rm main || true
	rm compression/DiRPi || true
	rm compression/DiRPi.o || true
