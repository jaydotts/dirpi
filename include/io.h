#include <string>
#ifndef IO_H
#define IO_H

/*
Helper functions for quick I/O operations 
*/

void SetMUXCode(int val); 

int ReadPin(int iPin); 

void ToggleSlowClock(); 

void reset_MUX(); 

std::string getTime(); 

bool isfile (const std::string& name); 

bool ispath (const char * path); 

void writeToFile(const char * fname, std::string input);

class Timer{
    private: 
        double start_time; 
        double stop_time; 
    public: 
        Timer(); 
        Timer(float time); 
        void start(); 
        void stop(); 
        void clear(); 
        double get_duration();
        double get_time();
};

#endif