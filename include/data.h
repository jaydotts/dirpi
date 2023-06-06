#include "setup.h"
#include "components.h"
#ifndef DATA_H
#define DATA_H
/*
Manages the read/write of data using 
helper functions from the IO class
*/

class Data{
    private: 
        int dataCH1, dataCH2; 
        int dataBits[2][8]; 
        int ** dataBlock;
        int memory_depth; 
        DIGI_TEMP tmp_sensor; 
        int trg1_count; 
        int trg2_count; 

    public:  
        Data(const int address_depth); 
        ~Data();
        int eventNum = 0; 
        void read_bytes(); 
        void Read();
        void Write(const char *fname); 
        void CountTriggers(int nclocks); 
        std::string PrintConfigs(); 
};

#endif