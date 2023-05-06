# Makefile
# use make -j3 to run all sources simultaneously. 

CC = g++
CFLAGS = -O 

all: runner manager plotter

compiler: 
	$(CC) $(CFLAGS ) src/*.cpp -o main -lwiringPi

# make sure to change this to the code path on the device!
runner: main
	/home/dirpi4/dirpi/main config/digi.config $(RUN)

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	/home/dirpi4/dirpi/checkFileSize.sh $(RUN)

plotter: 
	python3 gui.py

soft-reset:
	make -j runner manager RUN=$(RUN)

cleanup: 
	./checkFileSize $(RUN)
	@touch ".stop"
	@sleep 2
	@rm ".stop"
