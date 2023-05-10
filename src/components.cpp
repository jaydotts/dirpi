
//#include "setup2.h"
#include "../include/components.h"
#include "../include/setup.h"
#include <iostream> 
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <iomanip>
#include <sys/time.h>
#include <vector>
#include <chrono>
using namespace std; 
#define I2CBUS 1

// I2C Devices: 
////////////////////////////////////// Bias DACs 
BiasDAC::BiasDAC(int chan){ 
    // Sets amplitude of injected pulse 

    channel = chan; 

    if (channel == 1)
        addr = 0x4c; 
    else if (channel == 2)
        addr = 0x4d;
    
    DACfd = wiringPiI2CSetup(addr);
};

void BiasDAC::PrintBias(){
    int readbyte = wiringPiI2CReadReg8(DACfd, 10);
    int voltage = readbyte*(3300/4095);
    cout<<"CH"<<channel<<" pulse amplitude setting: "<<voltage<<" mV"<<endl;
};

void BiasDAC::SetVoltage(int voltage){

    int data[2]; 

    int DACval = (4095*voltage)/3300;
    if (DACval < 0) DACval = 0;
    if (DACval > 4095) DACval = 4095;

    // MCP4725 expects a 12bit data stream in two bytes (2nd & 3rd of transmission)
    data[0] = (DACval >> 8) & 0xFF; // [0 0 0 0 D11 D10 D9 D8] (first bits are modes for our use 0 is fine)
    data[1] = DACval; // [D7 D6 D5 D4 D3 D2 D1 D0]
    int result = wiringPiI2CWriteReg8(DACfd, data[0], data[1]);

 cout<<"Setting bias to "<<voltage<<"mV for channel "<<channel<<" "<< (result ? "FAILED" : "Succeeded")<<endl;

};

////////////////////////////////////// Threshold DACs
ThrDAC::ThrDAC(const int chan, int voltage, int persist){
    channel = chan; 

    if (channel == 1)
        addr = 0x60; 
    else if (channel == 2)
        addr = 0x61;
    
    DACfd = wiringPiI2CSetup(addr);

    SetThr(voltage,persist); 
};

void ThrDAC::SetThr(int voltage, int persist){
    // 2-byte array to hold 12-bit chunks 
    int data[2];
    int DACval = (4095*voltage)/3300;
    if (DACval < 0) DACval = 0;
    if (DACval > 4095) DACval = 4095;

    cout<<"Setting DAC"<<channel<<" to "<<DACval<<endl;
    //wiringPiI2CWrite(DACfd, 0x40);// WRITE register

    // MCP4725 expects a 12bit data stream in two bytes (2nd & 3rd of transmission)
    data[0] = (DACval >> 8) & 0xFF; // [0 0 0 0 D11 D10 D9 D8] (first bits are modes for our use 0 is fine)
    data[1] = DACval; // [D7 D6 D5 D4 D3 D2 D1 D0]
    int result = wiringPiI2CWriteReg8(DACfd, data[0], data[1]);
    cout<<"Setting trig threshold to "<<voltage<<"mV for channel "<<channel<<" "<< (result ? "FAILED" : "Succeeded")<<endl;
};


////////////////////////////////////// DIGIPOT
DIGIPOT::DIGIPOT(){
    // construct with floating wiper
    addr = 0x2c; 
    fd = wiringPiI2CSetup(addr); 
    if (fd == -1) cerr<<"Unable to setup device at "<<addr<<endl;
};

DIGIPOT::DIGIPOT(int chan, int N){
    // set wiper value in constructor
    addr = 0x2c; 
    fd = wiringPiI2CSetup(addr); 
    if (fd == -1) cerr<<"Unable to setup device at "<<addr<<endl;

    SetWiper(chan,N); 
};

// Write N (0-255) to wiper on digipot channel 
void DIGIPOT::SetWiper(int chan, int N){

    if(N>=256) N=255;
    if(N<=0) N=0;

    unsigned int wiper_reg; 
    // Select wiper memory register based on desired channel
    switch(chan){
        case 1: 
            wiper_reg = 0x00;
            break;
        case 2: 
            wiper_reg = 0x10;
            break;
        case 3:
            wiper_reg = 0x60;
            break;
        case 4: 
            wiper_reg = 0x70;
            break;
    } 

    // Write N (0-255) to wiper memory. I 
    unsigned int write_byte = (N & 0xFF);
    int result = wiringPiI2CWriteReg8(fd,wiper_reg,write_byte);

    cout<<"Writing N="<<N<<" to channel "<<chan<<" pot "<< (result ? "\033[1;31mFAILED\033[0m" : "Succeeded")<<endl;
};

void DIGIPOT::setInputBias(int chan, double voltage){

    if(voltage >= 860){
        voltage = 860; 
        SetWiper(chan,0);
        return;
    };
    if(voltage <= 0){
        voltage = 0; 
        SetWiper(chan,255);
        return;
    }
    // calculate resistance to achieve desired offset 

    int N = round((16*(2003*3300 - 5003*voltage))/(125*(3300-voltage))); 
    cout<<N<<endl;
    SetWiper(chan, N); 
};

////////////////////////////////////// ADC
I2CADC::I2CADC(){
    addr = 0x48; 
};

////////////////////////////////////// EEPROM
EEPROM::EEPROM(){
    addr = 0x50; 
};

////////////////////////////////////// DIGIO
DIGIO::DIGIO(int chan){

    channel = chan; 

    if (chan == 1)
        addr = 0x38; 
    else if (chan == 2)
        addr = 0x39; 

    fd = wiringPiI2CSetup(addr);
    if (fd == -1) cerr<<"Unable to setup device at "<<addr<<endl;
};

void DIGIO::setConfigReg(unsigned int write_byte){
    int result = wiringPiI2CWriteReg8(fd,3,write_byte);
};

void DIGIO::setIOState(unsigned int write_byte){
    int result = wiringPiI2CWriteReg8(fd,1,write_byte);
};

unsigned int DIGIO::readIOState(){
    return wiringPiI2CReadReg8(fd,1); 
}

void DIGIO::setPolarityReg(unsigned int write_byte){
    int result = wiringPiI2CWriteReg8(fd,2,write_byte);
};

////////////////////////////////////// GPIO EXPANDER
GPIO::GPIO(){
    addr = 0x21; 
    CLCK_20 = 0x02;
    CLCK_40 = 0x01;
    MEM_FULL = 0x04; 
    MEM_PART = 0x00; 
    TRPA = 0x08; 
    TRPB = 0x10; 
    fd = wiringPiI2CSetup(addr); 
    if (fd == -1) cerr<<"Unable to setup device at "<<addr<<endl;
};

void GPIO::setConfigReg(unsigned int write_byte){
    int result = wiringPiI2CWriteReg8(fd,3,write_byte);
    cout<<result<<endl;
};

void GPIO::setPolarityReg(unsigned int write_byte){
    cout<<"Flipping polarity"<<endl;
    int result = wiringPiI2CWriteReg8(fd,2,write_byte);
    cout<<result<<endl;
};

void GPIO::setIOState(unsigned int write_byte){
    cout<<"Setting GPIO state"<<endl;
    int result = wiringPiI2CWriteReg8(fd,1,write_byte);
    cout<<result<<endl;
}

void GPIO::setClock(int speed_MHz){

    // In order to write to pins, 
    // first set configuration to WRITE mode, 
    // then send write byte into reg. 1, then 
    // set all pins back to READ mode. 

    if(speed_MHz == 20){
        CLCK_SET = CLCK_20; 
    }
    else if(speed_MHz == 40){ 
        CLCK_SET = CLCK_40; 
    }
    else{cout<<"Invalid clock speed selected"<<endl;}

};

void GPIO::setTriggerPoint(int setting){
    /*
    Sets trigger point between 1 and 2 
    */

    switch(setting){
        case 1: 
            TRP_SET = TRPA; 
            cout<<"Set trigger point to A"<<endl;
            break;
        case 2: 
            TRP_SET = TRPB; 
            cout<<"Set trigger point to B"<<endl;
            break;
    }
}

void GPIO::setMemDepth(int setting){
    if( setting >= 4096){
        MEM_SET = MEM_FULL; 
        cout << "Using full depth" << endl; 
    }
    else{
        MEM_SET = MEM_PART; 
        cout << "Using partial depth" << endl; 
    }
}

void GPIO::set(){
    setClock(config->clckspeed); 
    setTriggerPoint(config->trigger_pos); 
    setMemDepth(config->memory_depth); 

    unsigned io_setting = {
        CLCK_SET | 
        MEM_SET  | 
        TRP_SET
    };
    setConfigReg(0x1F); 
    setIOState(io_setting); 
    setConfigReg(0x00); 
}

void GPIO::readPin(int pin){
    int output; 
    output = wiringPiI2CReadReg8(addr,pin);
    cout<<"Pin "<<pin<<"output: "<<output<<endl;
};

// Other: 

////////////////////////////////////// MUX
MUX::MUX(int chan){ 
    channel = chan; 
};

