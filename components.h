
// header file containg classes for each I2C component 

#ifndef COMPONENT_H 
#define COMPONENT_H

#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <vector>

using namespace std;
 
class BiasDAC{
    private: 
        const int chan; 
        const int addr; 
        const int DACfd; 
        int BiasVoltage; 

    public: 
        BiasDAC(const int chan); 
        void SetVoltage(int voltage); 
        
        ~BiasADC();

}

class ThrDac{
    private: 
        const int chan; 
        const int addr; 
        const int DACfd; 
        int ThrVoltage; 

    public: 
        ThrDAC(const int chan); 
        void SetThr(int voltage); 
        ~ThrADC();
}

class DIGIPOT{

}

class I2CADC{

}
