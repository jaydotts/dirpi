
// Methods to control components that are less time-sensitive, 
// such as digipot, DACs, and ADC. 

// May include classes for all components as a matter of consistency, 
// but will define important values locally on startup to preserve efficiency.

#include "components.h"
#include "pigpiod_if2.h"
// Need to manage the libraries that are included here 

#include <iostream> 
using namespace std; 
#define I2CBUS 1

// I2C Devices: 
////////////////////////////////////// Bias DACs 
BiasDAC::BiasDAC( int chan){ 
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
    cout<<"The chan "<<channel<<" bias byte is "<<readbyte<<endl;
};

void BiasDAC::SetVoltage(int voltage){
    if (voltage < 0) voltage = 0; 
    if (voltage > 255) voltage = 255; 
    BiasVoltage = voltage; 
};

////////////////////////////////////// Threshold DACs
ThrDAC::ThrDAC(const int chan){
    channel = chan; 

    if (channel == 1)
        addr = 0x60; 
    else if (channel == 2)
        addr = 0x61;
    
    DACfd = wiringPiI2CSetup(addr);
};

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
    //if (persist) {
     //   wiringPiI2CWrite(DACfd, WRITEDACEEPROM);
    //} else {
    //wiringPiI2CWrite(DACfd, addr);
    //}

    // Send our data using the register parameter as our first data byte
    // this ensures the data stream is as the MCP4725 expects
    wiringPiI2CWriteReg8(DACfd, data[0], data[1]);
};

////////////////////////////////////// DIGIPOT
DIGIPOT::DIGIPOT(){
    addr = 0x2c; 
    fd = wiringPiI2CSetup(addr); 
    if (fd == -1) cerr<<"Unable to setup device at "<<addr<<endl;
};

void DIGIPOT::SetVoltage(int chan, double voltage){
    // Set DC offset (voltage) in mV to 
    // channel 1 or 2 (chan) input 
    // by adjusting the digipot wiper

    //TODO: buiild in value checking for max/min voltages
    if(voltage >= 3300.0) voltage = 3299.0; // prevent NANing

    if (chan == 1){
        // Theoretical resistance of wiper determined by voltage divider equation
        double R_ideal = 75000.0*(1/((3300.0/voltage)-1)); 
        if (R_ideal > RMax) R_ideal = RMax; 

        cout<<"Requested Resistance: "<<R_ideal<<endl;
        // determine value of wiper "steps"

        unsigned N = round(((R_ideal-75.0)*256.0)/RMax);
        cout<<"N:"<<N<<endl;
        
        
        //unsigned int write_byte = ((N >> 8 & 0x01));
        unsigned int wiper_reg = 0x00;

        // first, tell pot to write next byte to register 0x00: 
        uint8_t write_cmd = ((N >> 8 & 0x01) | 0x00); // this is sent to the wiper register: says "enter write mode"
        wiringPiI2CWriteReg8(fd, wiper_reg, write_cmd);
        unsigned int write_byte = (N & 0xFF);
        int result = wiringPiI2CWriteReg8(fd,wiper_reg,write_byte);

        RCH1 = RMax*N/256.0;

        cout<<"Wiper resistance setting:"<<RCH1<<"/50000"<<endl;
        // Calculate offset accurate to precision of the wiper 
        int true_offset = (RCH1/(RCH1+75000.0))*3300.0;
        cout<<"CH1 offset: "<<true_offset<<"mV"<<endl;
    };

    if (chan == 2){
          // Theoretical resistance of wiper determined by voltage divider equation
        double R_ideal = 75000.0*(1/((3300.0/voltage)-1)); 
        if (R_ideal > RMax) R_ideal = RMax; 

        cout<<"Requested Resistance: "<<R_ideal<<endl;
        // determine value of wiper "steps"

        unsigned N = round(((R_ideal-75.0)*256.0)/RMax);
        cout<<"N:"<<N<<endl;
        
        
        //unsigned int write_byte = ((N >> 8 & 0x01));
        unsigned int wiper_reg = 0x01;

        // first, tell pot to write next byte to register 0x00: 
        uint8_t write_cmd = ((N >> 8 & 0x01) | 0x00); // this is sent to the wiper register: says "enter write mode"

        wiringPiI2CWriteReg8(fd, 0x01, write_cmd);
        
        unsigned int write_byte = (N & 0xFF);
        //int result = wiringPiI2CWriteReg8(fd,wiper_reg,write_byte);

        RCH2 = RMax*N/256.0;

        cout<<"Wiper resistance setting:"<<RCH2<<"/50000"<<endl;
        // Calculate offset accurate to precision of the wiper 
        int true_offset = (RCH2/(RCH2+75000.0))*3300.0;
        cout<<"CH2 offset: "<<true_offset<<"mV"<<endl;
};
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
DIGIO::DIGIO( int chan){
    channel = chan; 
    DIGIOfd = wiringPiI2CSetup(addr);
};

////////////////////////////////////// IO
IO::IO(){
    addr = 0x21; 
};

void IO::SetClock(int speed){
    // set En20 or En40 high 
    // (not both) to select the clock. 
};

// Other: 

////////////////////////////////////// MUX
MUX::MUX(int chan){ 
    channel = chan; 
};

////////////////////////////////////// MUX
SRAM::SRAM(){
};

void SRAM::restart(){
    // Prepare SRAM for next trigger 
    cout<<"SRAM Restart happens here"<<endl;
};