#include "../include/setup.h"
#include "../include/components.h"
#include "../include/io.h"
#include <iostream>
#include <fstream>
#include <wiringPi.h>
#include <wiringPiI2C.h>

using namespace std; 

const int MX0=27;      
const int MX1=24;      
const int MX2=22;      
const int MX1Out=21;   
const int MX2Out=26;   
const int SLWCLK=29;   
const int OEbar=28;    
const int DAQHalt=25;  
const int Calib=23;    
const int MR=6;        
const int Trg1Cnt=5;   
const int Trg2Cnt=3;   
const int SFTTRG=7;    
const int TrgExtEn=0;  
const int Trg1En=2;    
const int Trg2En=4;
const int PSCL = 1;
/* 
Define parameters to be loaded 
from the config file. Initialize 
with defaults in case of a read 
problem. 
*/

bool trg1 = false; 
bool trg2 = false; 
bool sftrg = false;
bool extrg = true;  /* Enable extrg by default */

int PotValCh1 = 0;
int PotValCh2 = 0;
int PotValCh3 = 0;
int PotValCh4 = 0;
int DACValCh1 = 0; 
int DACValCh2 = 0;
int clckspeed = 20;  
int PSCLduty = 1; 
int events_perFile = 1; 
bool record_data = false; 
int run_num; 
std::string fname_prefix;
std::string output_folder;

void setupPins(){
    /* Setup pins to use WiringPi library and I2C */ 
    
    pinMode(MX0,OUTPUT);
    pinMode(MX1,OUTPUT);
    pinMode(MX2,OUTPUT);
    pinMode(MX1Out,INPUT);
    pinMode(MX2Out,INPUT);
    pinMode(SLWCLK,OUTPUT);
    pinMode(Calib,OUTPUT);
    pinMode(OEbar,INPUT);
    pullUpDnControl(OEbar,PUD_UP);
    pinMode(MR,OUTPUT);
    pinMode(DAQHalt,OUTPUT);
    pinMode(Trg1Cnt,INPUT);
    pinMode(Trg2Cnt,INPUT);
    pinMode(SFTTRG,OUTPUT);
    pinMode(TrgExtEn,OUTPUT);
    pinMode(Trg1En,OUTPUT);
    pinMode(Trg2En,OUTPUT);
    pinMode(PSCL,OUTPUT);

    // set channel prescale
    digitalWrite(PSCL,PSCLduty);

}

bool setupComponents(){
    /*
    Create component objects with initialized 
    values 
    */
    DIGIPOT CH1_POT = DIGIPOT(1, PotValCh1);
    DIGIPOT CH2_POT = DIGIPOT(2, PotValCh2); 
    DIGIPOT SIPBIAS_1 = DIGIPOT(3, PotValCh3);
    DIGIPOT SIPBIAS_2 = DIGIPOT(4, PotValCh4); 

    ThrDAC ThrDAC1 = ThrDAC(1, DACValCh1, 0); 
    ThrDAC ThrDAC2 = ThrDAC(2, DACValCh2, 0); 

    // GPIO lines used to set clock speed
    GPIO IO_1 = GPIO(); 
    IO_1.setClock(clckspeed); 
    //IO_1.setTriggerPoint(1); 

    return true;
}

void load_configs(const char * CONFIG_FILE_PATH){
    /* 
    Loads variables with configs from config file. 
    If required variables are not filled, it returns false. 
    */
    ifstream cfile(CONFIG_FILE_PATH, ios::out);
    if (cfile.is_open()){ //checking whether the file is open
        string line;
        while(getline(cfile, line)){ //read data from file object and put it into string.

            if( line.empty() || line[0] == '#' ){continue;}

            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);

            if (name.compare("TrgCh1 ")==0 ){trg1 = (bool)stoi(value);
                std::cout << name << " " << trg1 << '\n';}
            if (name.compare("TrgCh2 ")==0 ){trg2 = (bool)stoi(value);
                std::cout << name << " " << trg2 << '\n';}
            if (name.compare("sftrg ")==0 ){sftrg = (bool)stoi(value);
                std::cout << name << " " << sftrg << '\n';}
            if (name.compare("extrg ")==0 ){extrg = (bool)stoi(value);
                std::cout << name << " " << extrg << '\n';} 
            if (name.compare("Prescale ")==0 ){PSCLduty = stoi(value);
                std::cout << name << " " << PSCLduty << '\n';} 
            if (name.compare("events_per_file ")==0){
            events_perFile = stoi(value);
                std::cout << name << " " << events_perFile << '\n';}
            if (name.compare("clock ")==0 ){
                int speed = stoi(value);
                if ((speed == 20)||(speed == 40)){clckspeed = speed;
                    std::cout << name << " " << speed << '\n';}
            }

            // parse config for component settings 
            if (name.compare("DACValCh1 ")==0 ){DACValCh1 = stoi(value);} 
            if (name.compare("DACValCh2 ")==0 ){DACValCh2 = stoi(value);} 
            if (name.compare("PotValCh1 ")==0 ){PotValCh1 = stoi(value);} 
            if (name.compare("PotValCh2 ")==0 ){PotValCh2 = stoi(value);} 
            if (name.compare("PotValCh3 ")==0 ){PotValCh3 = stoi(value);} 
            if (name.compare("PotValCh4 ")==0 ){PotValCh4 = stoi(value);} 
        }

    cfile.close();
    }
}

bool setupFiles(int run_num){
    /*
    Sets up file naming conventions and does directory checks 
    */

    fname_prefix = "Run"+std::to_string(run_num);
    output_folder = "Run"+std::to_string(run_num);

    if(ispath(output_folder.c_str())){
        record_data=true; 
    }
    return true; 
}

bool initialize(const char * CONFIG_FILE_PATH){

    /// load variables from configuration file 
    load_configs(CONFIG_FILE_PATH); 

    /// Setup pins to use WiringPi library and I2C
    wiringPiSetup();
    setupPins();
    
    /// Create component objects & initialize them 
    setupComponents(); 
    setupFiles(run_num);

    return true; 
}
