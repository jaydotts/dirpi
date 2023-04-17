#ifndef SRAM_H
#define SRAM_H

/*
Manage the SRAM, including 
start/stop of sampling, memory 
depth, etc. 
*/

class SRAM{
    private: 
        int memory_depth = 4096; 
    public: 
        void enable_sampling();
};

#endif 