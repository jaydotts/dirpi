
// sets startup configuration 
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
#include "Adafruit_ADS1015.h"
#include "components.cpp"
//#include "PCA9554.hpp"
using namespace std;

#define CONFIG_FILE_PATH "./config/digi.config"

// Define useful names for the GPIO pins and the wiringPi interface to them.
// The wiringPi numbering is described with the "gpio readall" command for RPis with wiringPi installed.
// I copy its output for the Pi3 below, and edit in columns with the photon counter uses.
// +-----+-----+------------+---Pi 3---+------------+------+---------+-----+-----+
// | BCM | wPi | PhotonCntr | Physical | PhotonCntr | wPi | BCM |
// +-----+-----+------------+----++----+------------+-----+-----+
// |     |     |  N/C       |  1 || 2  |   5V       |     |     |
// |   2 |   8 |  SDA       |  3 || 4  |   5V       |     |     |
// |   3 |   9 |  SCL       |  5 || 6  |   GND      |     |     |
// |   4 |   7 |  SFTTRG    |  7 || 8  |   TX       | 15  | 14  |
// |     |     |  GND       |  9 || 10 |   RX       | 16  | 15  |
// |  17 |   0 |  TrgExtEn  | 11 || 12 |   PSCL     | 1   | 18  |
// |  27 |   2 |  Trg1En    | 13 || 14 |   GND      |     |     |
// |  22 |   3 |  Trg2Cnt   | 15 || 16 |   Trg2En   | 4   | 23  |
// |     |     |  N/C       | 17 || 18 |   Trg1Cnt  | 5   | 24  |
// |  10 |  12 |  SPI_MOSI  | 19 || 20 |   GND      |     |     |
// |   9 |  13 |  SPI_MISO  | 21 || 22 |   MR       | 6   | 25  |
// |  11 |  14 |  SPI_CLK   | 23 || 24 |   SPI_CE0  | 10  | 8   |
// |     |     |  GND       | 25 || 26 |   SPI_CE1  | 11  | 7   |
// |   0 |  30 |  ISDA      | 27 || 28 |   ISCL     | 31  | 1   |
// |   5 |  21 |  MX1Out    | 29 || 30 |   GND      |     |     |
// |   6 |  22 |  MX2       | 31 || 32 |   MX2Out   | 26  | 12  |
// |  13 |  23 |  Calib     | 33 || 34 |   GND      |     |     |
// |  19 |  24 |  MX1       | 35 || 36 |   MX0      | 27  | 16  |
// |  26 |  25 |  DAQHalt   | 37 || 38 |   OEbar    | 28  | 20  |
// |     |     |  GND       | 39 || 40 |   SLWCLK   | 29  | 21  |
// +-----+-----+------------+---Pi 3---+------------+------+----+
// | BCM | wPi | PhotonCntr | Physical | PhotonCntr | wPi | BCM |
// +-----+-----+------------+----++----+------------+-----+-----+
//
// The ExtraN pins are not used by the board, but they are available for extra purposes, such as self pulsing during tests.

// Global variables that keep track of the total number of bits read and how many read errors occurred.
// All bits are read twice to check for mismatches indicating an error. 
int nBitsRead = 0;
int nReadErrors = 0;

int debug = 0;
bool DoErrorChecks = true;

// Global variables that speed reading.
int addressDepth = 4096; // 12 bits used in the SRAM
int dataCH1, dataCH2; // The bytes most recently read for each channel
int dataBits[2][8];
int dataBlock[2][4096]; // Results of all data reads

int trg1CntAddr, trg2CntAddr; // Address when Trg1Cnt and Trg2Cnt go high.

int eventNum = 0;

int saveWaveforms = 1000; // Prescale for saving waveforms.  1=all
int printScalers = 100; // Prescale for calculating/displaying scalers

int trgCnt[2][2]; // Results of all Trg1Cnt and Trg2Cnt reads. First number is channel, second number is current (0) and previous (1) values
double scalars[2];
bool checkScalars[2]; 
uint16_t nSLWCLK = 0;

chrono::high_resolution_clock::time_point t0[2];

int MX0 = 27;      // OUTPUT. Lowest bit of the multiplexer code.
int MX1 = 24;      // OUTPUT. Middle bit of the multiplexer code.
int MX2 = 22;      // OUTPUT. Highest bit of the multiplexer code.
int MX1Out = 21;   // INPUT. Data bit out of multiplexer for CH1.
int MX2Out = 26;   // INPUT. Data bit out of multiplexer for CH2.
int SLWCLK = 29;   // OUTPUT. Set high when runtning fast clock, and toggle to slowly clock addresses for readout.
int OEbar = 28;    // INPUT. Goes low when ready for readout.
int DAQHalt = 25;  // OUTPUT. Hold high to stay in readout mode.
int Calib = 23;    // OUTPUT. Pulled high to fire a calibration pulse.
int MR = 6;        // OUTPUT. Pulse high to reset counters before starting a new event.
int Trg1Cnt = 5;   // INPUT. Goes high when the trigger counter for CH1 overflows.
int Trg2Cnt = 3;   // INPUT. Goes high when the trigger counter for CH2 overflows.
int SFTTRG = 7;    // OUTPUT. Pulse high to force a software trigger through TrgExt.
int TrgExtEn = 0;  // OUTPUT. Set high to enable external triggers and software triggers.
int Trg1En = 2;    // OUTPUT. Set high to enable CH1 triggers.
int Trg2En = 4;    // OUTPUT. Set high to enable CH2 triggers.
int PSCL = 1;      // OUTPUT (PWM). Used to prescale the number of CH1 OR CH2 triggers accepted.
int PSCLduty = 1;      // OUTPUT (PWM). Used to prescale the number of CH1 OR CH2 triggers accepted.

int PotValCh1 = 0;
int PotValCh2 = 0;
int DACValCh1 = 0; 
int DACValCh2 = 0;


// parameters to load from config 
//bool extrg; 
bool trg1; 
bool trg2; 
bool sftrg;
bool extrg;
int nEvents;
int clckspeed; 
int events_per_file;
float timeout; 
std::string fname_prefix; 

bool LoadRunConfiguration(const char* configFile){

  bool pass = true; 

  ifstream cfile(configFile, ios::out);
    if (cfile.is_open()){ //checking whether the file is open
      string line;
      while(getline(cfile, line)){ //read data from file object and put it into string.

          if( line.empty() || line[0] == '#' ){continue;}

            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            std::cout << name << " " << value << '\n';

      if (name.compare("nEvents ")==0 ){nEvents = stoi(value);} 
      if (name.compare("events_per_file ")==0){events_per_file = stoi(value);}
      if (name.compare("TrgCh1 ")==0 ){trg1 = (bool)stoi(value);}
      if (name.compare("TrgCh2 ")==0 ){trg2 = (bool)stoi(value);}
      if (name.compare("sftrg ")==0 ){sftrg = (bool)stoi(value);}
      if (name.compare("extrg ")==0 ){extrg = (bool)stoi(value);} 
      if (name.compare("timeout ")==0){timeout = stof(value);}
      if (name.compare("fname ")==0){
        cout<<"File name: "<<value<<endl; 
        fname_prefix = value;}

      if (name.compare("clock ")==0 ){
        int speed = stoi(value);
        if ((speed == 20)||(speed == 40)){clckspeed = speed;}
        else pass = false; 
        }

      // parse config for component settings 
      if (name.compare("DACValCh1 ")==0 ){DACValCh1 = stoi(value);} 
      if (name.compare("DACValCh2 ")==0 ){DACValCh2 = stoi(value);} 
      if (name.compare("PotValCh1 ")==0 ){PotValCh1 = stoi(value);} 
      if (name.compare("PotValCh2 ")==0 ){PotValCh2 = stoi(value);} 
  
      }
      cfile.close();

      cout<<pass<<endl;
    }
}

// Set up I2C components + determine that they exist 
void setupComponents(){
  cout<<"Initializing components..."<<endl;
  
  // Setup pots for DC pedestal setting 
  DIGIPOT CH1_Ped = DIGIPOT();
  DIGIPOT CH2_Ped = DIGIPOT();

  // Setup threshold DACs
  ThrDAC ThrDAC1 = ThrDAC(1); 
  ThrDAC ThrDAC2 = ThrDAC(2); 
  
  // Throw a warning if the pedestal is above the trigger threshold? 

  // Setup IO expander chip
  IO GPIO1 = IO(); 

  cout<<"POT1: "<<PotValCh1<<endl;
  CH1_Ped.SetWiper(1, PotValCh1);
  CH2_Ped.SetWiper(2, PotValCh2); 
  ThrDAC1.SetThr(DACValCh1,0); 
  ThrDAC2.SetThr(DACValCh2,0); 

  cout<<"Setting clock speed to "<<clckspeed<<endl;
    IO clock_IO = IO(); 
    clock_IO.setClock(clckspeed);
  
}

void setupPins(){
  const char* configFile = "Config.txt";
  wiringPiSetup(); //Setup GPIO Pins, etc. to use wiringPi and wiringPiI2C interface
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
  // pinMode(PSCL,OUTPUT
  pinMode(PSCL,OUTPUT);
}

