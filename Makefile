# Makefile
# use make -j2 to run last two simultaneously
CC = g++
CFLAGS = -O 

all: compiler runner manager 

compiler: 
	$(CC) $(CFLAGS ) test.cpp -o run

# make sure to change this to the code path on the device!
runner: run
	/Users/jhoward/Research/test_stuff/run

# make sure to change this to the code path on the device! 
manager: checkFileSize.sh
	/Users/jhoward/Research/test_stuff/checkFileSize.sh
