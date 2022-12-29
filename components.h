
// header file containg classes for each I2C component 

#ifndef COMPONENT_H 
#define COMPONENT_H
#endif 

#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <vector>

using namespace std;
 
// I2C Devices: 
class BiasDAC{
    //DAC5571IDBVR
    private: 
        int channel; 
        unsigned addr; 
        unsigned DACfd; 
        int BiasVoltage; 

    public: 
        BiasDAC( int chan); 
        void SetVoltage(int voltage); 
        void PrintBias();
        ~BiasDAC();

};

class ThrDAC{
    //MCP4725A0T-E/CH

    // This component has its own EEPROM (?)
    private: 
        int channel; 
        unsigned addr; 
        unsigned DACfd; 
        int ThrVoltage; 

    public: 
        ThrDAC( int chan); 
        void SetThr(int voltage, int persist); 
};

class DIGIPOT{
    //MCP4451-503E/ST
    private:
         unsigned addr;
         unsigned fd; 
         double RMax = 50000.0; // Max resistance of DIGIPOT in Ohms 

        // DC offsets of each channel in mV 
        double offsetCH1 = 0; 
        double offsetCH2 = 0;

        // Resistance values for each channel 
        // (in order to set DC offset)
        int RCH1 = 0; 
        int RCH2 = 0; 
    
    public: 
        DIGIPOT(); 
        void SetWiper(int chan, int N); 
        void setInputBias(int chan, double voltage); 
        int test();

};

class I2CADC{
    //ADS1115IDGSR
    private:
         unsigned addr;
    
    public: 
        I2CADC(); 
        ~I2CADC(); 
};

class EEPROM{
    //M24C64-RMN6TP
    private:
        // add wPi addr
         unsigned addr;
    
    public: 
        EEPROM(); 
        ~EEPROM(); 
};

class DIGIO{
    //PCA9554APW,118
    private: 
        // add Pi handle + wPi addr
         int channel; 
         unsigned addr; 
         unsigned DIGIOfd; 
    
    public:
        DIGIO( int chan);
        ~DIGIO();
};


class IO{
    //TCA6408ARGTR
    //IO expander used to select clock
    private: 
         unsigned addr; 
    
    public: 
        IO(); 
        void SetClock(int speed); 
        ~IO(); 
};

// Non-I2C Devices: 
class MUX{
    private:
         int channel; 
    
    public: 
        MUX( int chan); 
        ~MUX(); 
};

class SRAM{
    //CY7C1021DV33-10ZSXIT
    private: 
        void restart();
    
    public: 
        SRAM();
        ~SRAM();
};

class CLOCK{        

};