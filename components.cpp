
// Methods to control components that are less time-sensitive, 
// such as digipot, DACs, and ADC. 

// May include classes for all components as a matter of consistency, 
// but will define important values locally on startup to preserve efficiency.

#include "components.h"
// Need to manage the libraries that are included here 

#include <iostream> 
using namespace std; 

// I2C Devices: 

////////////////////////////////////// Bias DACs 
BiasDAC::BiasDAC(const int chan){ 
    channel = chan; 

    if (channel == 1)
        addr = 0x4c; 
    else if (channel == 2)
        addr = 0x4d;
    
    DACfd = wiringPiI2CSetup(addr);
}

void BiasDAC::PrintBias(){
    int readbyte = wiringPiI2CReadReg8(DACfd, 10);
    cout<<"The chan "<<chan<<" bias byte is "<<readbyte<<endl;
}

void BiasDAC::SetVoltage(int voltage){
    if (voltage < 0) voltage = 0; 
    if (voltage > 255) voltage = 255; 
    BiasVoltage = voltage; 
}

////////////////////////////////////// Threshold DACs
ThrDac::ThrDAC(const int chan){
    channel = chan; 

    if (channel == 1)
        addr = 0x60; 
    else if (channel == 2)
        addr = 0x61;
    
    DACfd = wiringPiI2CSetup(addr);
}

void ThrDAC::SetThr(int voltage, int persist){
    // 2-byte array to hold 12-bit chunks 
    int data[2];
    int DACval = (4095*voltage)/3300;
    if (DACval < 0) DACval = 0;
    if (DACval > 4095) DACval = 4095;

    // MCP4725 expects a 12bit data stream in two bytes (2nd & 3rd of transmission)
    data[0] = (DACval >> 8) & 0xFF; // [0 0 0 0 D11 D10 D9 D8] (first bits are modes for our use 0 is fine)
    data[1] = DACval; // [D7 D6 D5 D4 D3 D2 D1 D0]

      // 1st byte is the register
    if (persist) {
        wiringPiI2CWrite(DACfd, WRITEDACEEPROM);
    } else {
        wiringPiI2CWrite(DACfd, WRITEDAC);
    }

    // Send our data using the register parameter as our first data byte
    // this ensures the data stream is as the MCP4725 expects
    wiringPiI2CWriteReg8(DACfd, data[0], data[1]);
}

////////////////////////////////////// DIGIPOT
DIGIPOT::DIGIPOT(){
    addr = 0x2c; 
}

////////////////////////////////////// ADC
I2CADC::I2CADC(){
    addr = 0x48; 
}

////////////////////////////////////// MUX
MUX::MUX(const int chan){ 
    channel = chan; 
}

////////////////////////////////////// EEPROM
EEPROM::EEPROM(){
    addr = 0x50; 
}

////////////////////////////////////// DIGIO
DIGIO::DIGIO(const int chan){
    channel = chan; 
}

////////////////////////////////////// IO
IO::IO(){
    addr = 0x21; 
}

void IO::SetClock(int speed){
    // set En20 or En40 high 
    // (not both) to select the clock. 
}


// Non-I2C Devices: 