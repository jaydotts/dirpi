
// header file containg classes for each I2C component 

#ifndef COMPONENT_H 
#define COMPONENT_H

#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <vector>

using namespace std;
 
// I2C Devices: 
class BiasDAC{
    //DAC5571IDBVR
    private: 
        const int channel; 
        const unsigned addr; 
        const unsigned DACfd; 
        int BiasVoltage; 

    public: 
        BiasDAC(const int chan); 
        void SetVoltage(int voltage); 
        ~BiasDAC();

}

class ThrDAC{
    //MCP4725A0T-E/CH

    // This component has its own EEPROM (?)
    private: 
        const int channel; 
        const unsigned addr; 
        const unsigned DACfd; 
        int ThrVoltage; 

    public: 
        ThrDAC(const int chan); 
        void SetThr(int voltage); 
        ~ThrDAC();
}

class DIGIPOT{
    //MCP4451-503E/ST
    private:
        const unsigned addr;
        int OffsetVoltage; // set a default val? 
    
    public: 
        DIGIPOT(); 
        void SetVoltage(); 
        ~DIGIPOT(); 
}

class I2CADC{
    //ADS1115IDGSR
    private:
        const unsigned addr;
    
    public: 
        I2CADC(); 
        ~I2CADC(); 
}

class EEPROM{
    //M24C64-RMN6TP
    private:
        // add wPi addr
        const unsigned addr;
    
    public: 
        EEPROM(); 
        ~EEPROM(); 
}

class DIGIO{
    //PCA9554APW,118
    private: 
        // add Pi handle + wPi addr
        const int channel; 
        const unsigned addr; 
        const unsigned DIGIOfd; 
    
    public:
        DIGIO(const int chan);
        ~DIGIO();
}

class IO{
    //TCA6408ARGTR
    //IO expander used to select clock
    private: 
        const unsigned addr; 
    
    public: 
        IO(); 
        void SetClock(int speed); 
        ~IO(); 
}

// Non-I2C Devices: 
class MUX{
    private:
        const int channel; 
    
    public: 
        MUX(const int chan); 
        ~MUX(); 
}

class SRAM{
    //CY7C1021DV33-10ZSXIT
    public: 
        restart();
}