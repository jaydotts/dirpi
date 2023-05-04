# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 

all: runner manager plotter

compiler: 
	$(CC) $(CFLAGS ) src/*.cpp -o main -lwiringPi

runner: main
	$(DIR)/main config/digi.config $(RUN)

manager: checkFileSize.sh
	$(DIR)/checkFileSize.sh $(RUN)

plotter: 
	python3 gui.py

soft-reset:
	make -j runner manager plotter RUN=$(RUN) DIR=$(DIR)

cleanup: 
	./checkFileSize $(RUN)
	@touch ".stop"
	@sleep 2
	@rm ".stop"
