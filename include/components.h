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
        int channel; 
        unsigned addr; 
        unsigned DACfd; 
        int BiasVoltage; 

    public: 
        BiasDAC(int chan); 
        void SetVoltage(int voltage); 
        void PrintBias();

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
        ThrDAC(int chan, int voltage, int persist); 
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
        DIGIPOT(int chan, int N);
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
        
};

class DIGIO{
    //PCA9554APW,118
    private: 
        // add Pi handle + wPi addr
         int channel; 
         unsigned addr; 
         unsigned fd; 
    
    public:
        DIGIO(int chan);
        void setConfigReg(unsigned int write_byte);
        void setPolarityReg(unsigned int write_byte);
        void setIOState(unsigned int write_byte);
        unsigned int readIOState(); 
        void writePin(int pin);
        void readPin(int pin);
};


class GPIO{
    //TCA6408ARGTR
    //IO expander used to select clock, trigger point, 
    //and address depth 
    private: 
         unsigned addr; 
         unsigned fd;
         // bits that set clckspeed
         unsigned CLCK_20; 
         unsigned CLCK_40;
         // bits to set memory depth
         // high = deep, low = shallow
         unsigned MEM_FULL;
         unsigned MEM_PART; 
         // bits to set trigger point
         unsigned TRPA; 
         unsigned TRPB; 

         // settings to pass to component 
         unsigned CLCK_SET; 
         unsigned MEM_SET; 
         unsigned TRP_SET; 

    
    public: 
        GPIO(); 
        void set(); 
        unsigned setClock(int speed_MHz); 
        unsigned setTriggerPoint(int setting); 
        unsigned setMemDepth(int setting); 
        void setConfigReg(unsigned int write_byte);
        void setPolarityReg(unsigned int write_byte);
        void setIOState(unsigned int write_byte);
        void writePin(int pin);
        void readPin(int pin);
};

class DIGI_TEMP{
    // MCP9808
    // digital temperature sensor
    private:
        unsigned addr; 
        unsigned fd; 
        const unsigned temp_reg = 0x05; 
    
    public: 
        DIGI_TEMP();
        float get_temp(); 

};

// Non-I2C Devices: 
class MUX{
    private:
         int channel; 
    
    public: 
        MUX( int chan); 
        ~MUX(); 
};

#endif