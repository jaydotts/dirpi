#include "../include/data.h"
#include "../include/io.h"
#include "../include/setup.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>


Data::Data(const int mem_depth){
    memory_depth = mem_depth; 
    dataBlock = new int*[2]; // dynamic array (size 2) of pointers to int
    dataBlock[0] = new int[memory_depth];
    dataBlock[1] = new int[memory_depth];
    DIGI_TEMP tmp_sensor = DIGI_TEMP(); 
    trg1_count = 0; 
    trg2_count = 0; 
}

void Data::read_bytes(){
    /*
    Read data byte from both channels 
    of SRAM 
    */
    int bitsMX1[8];
    int bitsMX2[8]; 
    for (int iMUX = 0; iMUX<8; iMUX++) {
    SetMUXCode(iMUX);
    bitsMX1[iMUX] = ReadPin(MX1Out);
    bitsMX2[iMUX] = ReadPin(MX2Out);
    }
    // Fix the bit scrammbling done to ease trace routing
    dataBits[0][0] = bitsMX1[3];
    dataBits[0][1] = bitsMX1[0];
    dataBits[0][2] = bitsMX1[1];
    dataBits[0][3] = bitsMX1[2];
    dataBits[0][4] = bitsMX1[4];
    dataBits[0][5] = bitsMX1[6];
    dataBits[0][6] = bitsMX1[7];
    dataBits[0][7] = bitsMX1[5];
    dataBits[1][0] = bitsMX2[3];
    dataBits[1][1] = bitsMX2[0];
    dataBits[1][2] = bitsMX2[1];
    dataBits[1][3] = bitsMX2[2];
    dataBits[1][4] = bitsMX2[5];
    dataBits[1][5] = bitsMX2[7];
    dataBits[1][6] = bitsMX2[6];
    dataBits[1][7] = bitsMX2[4];

    // Extract the ADC values from the bits
    dataCH1 = 0;
    dataCH2 = 0;
    int datascale = 1; // scale by 3300 /255 in readout
    for (int i=0; i<8; i++) {
        dataCH1 += dataBits[0][i]*datascale;
        dataCH2 += dataBits[1][i]*datascale;
        datascale *= 2;
    }
}

void Data::Read(){
    for (int i=0; i<memory_depth; i++){
        ToggleSlowClock(); 
        CountTriggers(i); 
        read_bytes(); 
        dataBlock[0][i] = dataCH1;
        dataBlock[1][i] = dataCH2;
    }
}

void Data::Write(const char *fname, int64_t tLive, int64_t tDead, int64_t evtTime){
    // Make buffers for waveform data

    FILE *OutputFile;
    FILE *plot_output; 
    char data_buffer[50];
    char plot_buffer[50]; 
    sprintf(plot_buffer, "plot_buffer.txt");
    if(config->record_data){sprintf(data_buffer, fname);}

    plot_output = fopen(plot_buffer,"w");
 
    if(!isfile(data_buffer) && config->record_data){
        OutputFile = fopen(data_buffer , "w ");
    }
    else if(config->record_data){
        OutputFile = fopen(data_buffer , "a");
    } 
    
    if(config->record_data){
//        int64_t t0;
//        sscanf(getTimeus().c_str(), "%llu", &t0);
        fprintf(OutputFile,(
//            "TIME " + std::to_string(t0)+"\n"+
            "TIME " + std::to_string(evtTime/1000)+"\n"+
            "T_LIVE " + std::to_string(eventNum) + " " + std::to_string(tLive)+"\n"+
            "T_DEAD " + std::to_string(eventNum) + " " + std::to_string(tDead)+"\n"+
            "TEMP " + std::to_string(tmp_sensor.get_temp()) + "\n" +
            "Trg1Cnt " + std::to_string(trg1_count) + "\n" +
            "Trg2Cnt " + std::to_string(trg2_count)+ "\n").c_str());}
        
    for (int i=0; i<memory_depth; i++) {
        if (config->record_data){
            fprintf(OutputFile, "DATA: %i  %i %i %i\n", eventNum, i, 
                    dataBlock[0][i],dataBlock[1][i]);}

        fprintf(plot_output, "%i %i %i\n", i, 
                dataBlock[0][i],dataBlock[1][i]);
    }

    if(config->record_data){fclose(OutputFile);}
    fclose(plot_output);
}

void Data::CountTriggers(int nclocks){
    if (ReadPin(Trg1Cnt)){
        trg1_count = (2048 - nclocks); 
    }
    if (ReadPin(Trg2Cnt)){
        trg2_count = (2048 - nclocks); 
    }
}

std::string Data::PrintConfigs(){
return std::string(
    "TrgCh1 " + std::to_string(config->trg1) + "\n" +
    "TrgCh2 " + std::to_string(config->trg2) + "\n" +
    "sftrg " + std::to_string(config->sftrg) + "\n" +
    "extrg " + std::to_string(config->extrg) + "\n" +
    "Prescale " + std::to_string(config->PSCLduty) + "\n" +
    "Position " + std::to_string(config->trigger_pos) + "\n" +
    "PotValCh1 " + std::to_string(config->PotValCh1) + "\n" +
    "PotValCh2 " + std::to_string(config->PotValCh2) + "\n" +
    "PotValCh3 " + std::to_string(config->PotValCh3) + "\n" + 
    "PotValCh4 " + std::to_string(config->PotValCh4) + "\n" +
    "DACValCh1 " + std::to_string(config->DACValCh1) + "\n" +
    "DACValCh2 " + std::to_string(config->DACValCh2) + "\n" +
    "clckspeed " + std::to_string(config->clckspeed) + "\n");}

Data::~Data(){
delete[] dataBlock[0];
delete[] dataBlock[1];
delete[] dataBlock;
}
