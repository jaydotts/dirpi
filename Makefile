# Makefile
CC = g++
CFLAGS = -O 


all: test main
	$(CC) $(CFLAGS) test.cpp main.cpp

test: 
	$(CC) $(CFLAGS ) test.cpp -o test -lwiringPi
	sudo ./test 

main: 
	$(CC) $(CFLAGS ) main.cpp -o main -lwiringPi

clean: 
	rm test || true
	rm main || true 
	rm outputfile.txt || true