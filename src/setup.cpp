#include "../include/setup.h"
#include "../include/components.h"
#include "../include/io.h"
#include "../include/INIReader.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
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

int run_num; 
int max_files = (2^31); 
std::string fname_prefix;
std::string output_folder;
Configuration* config; 

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
    digitalWrite(PSCL,config->PSCLduty);
}

bool setupComponents(){
    /*
    Create component objects with initialized 
    values 
    */
    DIGIPOT CH1_POT = DIGIPOT(1, config->PotValCh1);
    DIGIPOT CH2_POT = DIGIPOT(2, config->PotValCh2); 
    DIGIPOT CH3_POT = DIGIPOT(3, config->PotValCh3);
    DIGIPOT CH4_POT = DIGIPOT(4, config->PotValCh4); 

    ThrDAC ThrDAC1 = ThrDAC(1, config->DACValCh1, 0); 
    ThrDAC ThrDAC2 = ThrDAC(2, config->DACValCh2, 0); 

    // GPIO lines used to set clock speed
    GPIO IO_1 = GPIO(); 
    IO_1.set(); 
    return true;
}

Configuration* load_configs(std::string CONFIG_FILE_PATH){
 
    INIReader reader(CONFIG_FILE_PATH); 

    if (reader.ParseError() < 0) {
        std::cout << "Can't load " << CONFIG_FILE_PATH<< "\n";
    }
    
    Configuration* conf = new Configuration{
        .trg1 = reader.GetBoolean("trigger","TrgCh1",0),
        .trg2 = reader.GetBoolean("trigger","TrgCh2",0),
        .sftrg = reader.GetBoolean("trigger","sftrg",0),
        .extrg = reader.GetBoolean("trigger","extrg",1),
        .PotValCh1 = reader.GetInteger("components","PotValCh1",255),
        .PotValCh2 = reader.GetInteger("components","PotValCh2",255),
        .PotValCh3 = reader.GetInteger("components","PotValCh3",255),
        .PotValCh4 = reader.GetInteger("components","PotValCh4",255),
        .DACValCh1 = reader.GetInteger("trigger","DACValCh1",100),
        .DACValCh2 = reader.GetInteger("trigger","DACValCh2",100),
        .trigger_pos = reader.GetInteger("trigger","Position",1),
        .memory_depth = reader.GetInteger("data","memory_depth",4096),
        .clckspeed = reader.GetInteger("components","clckspeed",20),
        .PSCLduty = reader.GetInteger("trigger","Prescale",1),
        .events_perFile = reader.GetInteger("data","events_per_file",100),
        .record_data = reader.GetBoolean("data","record_data",0)
    };

    return conf; 
}

bool setupFiles(int run_num){
    /*
    Sets up file naming conventions and does directory checks 
    */

    fname_prefix = "Run"+std::to_string(run_num);
    output_folder = "Run"+std::to_string(run_num);

    if(ispath(output_folder.c_str())){
        config->record_data=true; 
    }
    return true; 
}

bool initialize(std::string CONFIG_FILE_PATH){

    config = load_configs(CONFIG_FILE_PATH);

    /// Setup pins to use WiringPi library and I2C
    wiringPiSetup();
    setupPins();
    
    /// Create component objects & initialize them 
    setupComponents(); 
    setupFiles(run_num);

    return true; 
}
