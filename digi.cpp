// Read the 2-channel digitizer
// David Stuart, May 1, 2022
// This uses the wiringPi library for fast GPIO access. To compile use
// g++ -O digi.cpp -o digi -lwiringPi

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
//#include "PCA9554.hpp"
using namespace std;

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
// |   5 |  21 |  Intrpt    | 29 || 30 |   GND      |     |     |
// |   6 |  22 |  MX1Out    | 31 || 32 |   MX2Out   | 26  | 12  |
// |  13 |  23 |  MX2       | 33 || 34 |   GND      |     |     |
// |  19 |  24 |  MX1       | 35 || 36 |   MX0      | 27  | 16  |
// |  26 |  25 |  DAQHalt   | 37 || 38 |   OEbar    | 28  | 20  |
// |     |     |  GND       | 39 || 40 |   SLWCLK   | 29  | 21  |
// +-----+-----+------------+---Pi 3---+------------+------+----+
// | BCM | wPi | PhotonCntr | Physical | PhotonCntr | wPi | BCM |
// +-----+-----+------------+----++----+------------+-----+-----+
//
// The ExtraN pins are not used by the board, but they are available for extra purposes, such as self pulsing during tests.
//std::map config< std::string, double>
//config["Threshold1"] = 0;
//config["Threshold2"] = 0;
//config["PSCL"] = 0;
//config["nEvents"] = 0;

int MX0 = 27;      // OUTPUT. Lowest bit of the multiplexer code.
int MX1 = 24;      // OUTPUT. Middle bit of the multiplexer code.
int MX2 = 23;      // OUTPUT. Highest bit of the multiplexer code.
int MX1Out = 22;   // INPUT. Data bit out of multiplexer for CH1.
int MX2Out = 26;   // INPUT. Data bit out of multiplexer for CH2.
int SLWCLK = 29;   // OUTPUT. Set high when runtning fast clock, and toggle to slowly clock addresses for readout.
int OEbar = 28;    // INPUT. Goes low when ready for readout.
int DAQHalt = 25;  // OUTPUT. Hold high to stay in readout mode.
int Intrpt = 21;   // INPUT. Pulled high to get attention from the RPi.
int MR = 6;        // OUTPUT. Pulse high to reset counters before starting a new event.
int Trg1Cnt = 5;   // INPUT. Goes high when the trigger counter for CH1 overflows.
int Trg2Cnt = 3;   // INPUT. Goes high when the trigger counter for CH2 overflows.
int SFTTRG = 7;    // OUTPUT. Pulse high to force a software trigger through TrgExt.
int TrgExtEn = 0;  // OUTPUT. Set high to enable external triggers and software triggers.
int Trg1En = 2;    // OUTPUT. Set high to enable CH1 triggers.
int Trg2En = 4;    // OUTPUT. Set high to enable CH2 triggers.
int PSCL = 1;      // OUTPUT (PWM). Used to prescale the number of CH1 OR CH2 triggers accepted.
int PSCLduty = 1;      // OUTPUT (PWM). Used to prescale the number of CH1 OR CH2 triggers accepted.

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
int nEvents = 1000; //number of events
int printScalers = 100; // Prescale for calculating/displaying scalers

int trgCnt[2][2]; // Results of all Trg1Cnt and Trg2Cnt reads. First number is channel, second number is current (0) and previous (1) values
double scalars[2];
bool checkScalars[2];
uint16_t nSLWCLK = 0;

chrono::high_resolution_clock::time_point t0[2];

bool is_file_exist(const char *filename)
{ 
 std::ifstream infile(filename);
 return infile.good();
}

// Define addresses for read/write of I2C components 
#define WRITEDAC 0x40
#define WRITEDACEEPROM 0x60 //0x50?
#define Thr1DACAddr 0x60
#define Thr2DACAddr 0x61
#define Bias1DACAddr 0x2c
#define Bias2DACAddr 0x2d
#define ADCaddr 0x48
#define EPPROMAddr 0x50
int Thr1DACfd; // I2C handle for DAC communication
int Thr2DACfd; // I2C handle for DAC communication
int Bias1DACfd; // I2C handle for DAC communication
int Bias2DACfd; // I2C handle for DAC communication
int EEPROMfd;   // 
int ADCfd;
#define ADCREG 0x00   //C0 
#define ADCCONF 0x01   //C0 

void printV( std::vector<double> const &a){
  for (int i{0};i<a.size();i++){
    cout<< a.at(i)<< ' ';
  }
}

void SetBias(int chan, int voltageadc, int persist) {
  // Set the bias voltage in volts. persist=1 writes to flash.
  //i2c communication with CAT5171 potentiometer 
  int DACfd;
  if (chan == 1)
    DACfd = Bias1DACfd;
  else if (chan == 2)
    DACfd = Bias2DACfd;
  else
    return;
  cout<<DACfd<<endl;
 // DACfd = DACfd*2; //add a zero to indicate writing to cat
  cout<<DACfd<<endl;
  // 2 byte array to hold 12bit data chunks
  int DACval =voltageadc;
  if (DACval < 0) DACval = 0;
  if (DACval > 255) DACval = 255;
  // limit check voltage
  //voltage = (voltage > 4095) ? 4095 : voltage;

  // CAT5171 expects 8bit data stream in two bytes (2nd & 3rd of transmission)
  //int WRITE = (64) & 0xFF; //01000000; //command to write to cat5171
  //int data = (DACval) & 0xFF; // 8-bit wiper position (effectively sets bias)
  int data = DACval;
  //int address;
  //if (chan==1) address = 44*2;
  //if (chan==2) address = 45*2;
  //wiringPiI2CWrite(DACfd, (address)&&0xFF);
 // sleep(1);
  //wiringPiI2CWrite(DACfd, WRITE);
  //sleep(1);
  //int x = 0b01000000;
  //int x = 0b01000000;
  //wiringPiI2CWrite(DACfd, x);
  // 1st byte is the register
  /*
  if (persist) {
    wiringPiI2CWrite(DACfd, WRITEDACEEPROM);
  } else {
    wiringPiI2CWrite(DACfd, WRITEDAC);
  }
  */
  // Send our data using the register parameter as our first data byte
  // this ensures the data stream is as the MCP4725 expects
  //wiringPiI2CWriteReg8(DACfd, 8, data);
  if (chan == 1){
	wiringPiI2CWriteReg8(DACfd, 10, data);
	int readbyte1 = wiringPiI2CReadReg8(DACfd, 10);
	cout<<"THe chan 1 bias byte is "<< readbyte1<<endl;}
  else if (chan == 2){
	wiringPiI2CWriteReg8(DACfd, 10, data);
	int readbyte2 = wiringPiI2CReadReg8(DACfd, 10);
	cout<<"THe chan 2 bias byte is "<< readbyte2<<endl;}
}

uint16_t ReadADC(uint8_t ch){
	  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= 0x0000;

  // Set single-ended input channel
  switch (ch)
  {
    case (0):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
      break;
    case (1):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
      break;
    case (2):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
      break;
    case (3):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
      break;
  }
   config |= ADS1015_REG_CONFIG_OS_SINGLE ;

	wiringPiI2CWriteReg16(ADCfd, ADS1015_REG_POINTER_CONFIG,(config>>8)|(config<<8)); 
	usleep(16000);
	wiringPiI2CWrite(ADCfd, ADCREG);
	uint16_t reading = wiringPiI2CReadReg16(ADCfd, ADCREG);
	  
	 //reading = (reading>>8)|(reading<<8); 
	 
	 //uint8_t reading2 = wiringPiI2CRead(ADCaddr);
	return reading;
}

void SetTriggerThreshold(int chan, int voltage, int persist) {
  // Set the trigger threshold voltage in millivolts. persist=1 writes to flash.
  int DACfd;
  if (chan == 1)
    DACfd = Thr1DACfd;
  else if (chan == 2)
    DACfd = Thr2DACfd;
  else
    return;
  // 2 byte array to hold 12bit data chunks
  int data[2];

  int DACval = (4095*voltage)/3300;
  if (DACval < 0) DACval = 0;
  if (DACval > 4095) DACval = 4095;
  // limit check voltage
  //voltage = (voltage > 4095) ? 4095 : voltage;

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

void readEEPROM(){
	int DACfd;
//	DACfd = EEPROMfd
//	wiringPiI2CWrite(DACfd, WRITEDACEEPROM);

//     data = wirinPiI2CRead();
    // while data !="":
       // do stuff
	//start condition, dev select, W bit up 
	
}

void TestTrgThr(int chan) // Test the setting of the trigger threshold
{
  // Vary the trigger threshold so it can be monitored on the scope
  for (int voltage=0; voltage<3301; voltage++) { // Loop over voltage
    cout << "Trigger threshold = "<<voltage<<" mV\n";
    SetTriggerThreshold(chan, voltage, 0);
    delayMicroseconds(10000);
  } // Loop over voltage    
}

void StartEvent() // Start a new event
{
 if (debug) {cout << "Starting new event.\n"; cout.flush();}
 // Repeat twice to avoid rare errors.
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(DAQHalt,0);
 digitalWrite(DAQHalt,0);
}

void ToggleSlowClock() // Toggle the slow clock down then up
{
 // Repeat twice to avoid rare errors.
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,0);
}

void ResetCounters() // Reset the address counters
{
 if (debug) {cout << "Reseting the address counters.\n"; cout.flush();}
 // Toggle high then low.
 // Repeat to avoid rare failures.
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,0);
 digitalWrite(MR,0);
 digitalWrite(MR,0);
}

int ReadPin(int iPin) // Read a wPi pin with error checking
{
 int i1;
 i1 = digitalRead(iPin);
 if (DoErrorChecks) { // Check for read errors?
  int i2, i3;
  i2 = digitalRead(iPin);
  i3 = i2; // Only do two reads for the first test. Do triplicate checking afterward if these don't match.
  int iRead = 0;
  nBitsRead++;
  while ((i1 != i2 || i1 != i3 || i2 != i3) && iRead<1000) {
   nReadErrors++; // Keep track of global read errors.
   i1 = digitalRead(iPin);
   i2 = digitalRead(iPin);
   i3 = digitalRead(iPin);
   nBitsRead++;
   iRead++;
  }
  if (iRead >= 1000) {cerr << "Bit mismatch after 1000 reads.\n"; cout.flush();}
 }
 return i1;
}

int IsTriggered() {
  for (int i=0; i<100; i++)
   if (ReadPin(OEbar) == 1) return 0;
  return 1; 
}

void SetMUXCode(int val) // Set the multiplexer code
{
 if (val<0 || val>7) {cerr << "Invalid MUX code\n"; cerr.flush();}
 int MXCODEA = 0;
 int MXCODEB = 0;
 int MXCODEC = 0;
 // Crude hack to set MUX code
 MXCODEA = val%2;
 if (val>3) 
  MXCODEB = (val-4)/2;
 else
  MXCODEB = val/2;
 MXCODEC = val/4;
 digitalWrite(MX0,MXCODEA);
 digitalWrite(MX1,MXCODEB);
 digitalWrite(MX2,MXCODEC);
}

void ReadData() // Read the 16 data bits into the global variable array
{
  int bitsMX1[8];
  int bitsMX2[8];
  for (int iMUX = 0; iMUX<8; iMUX++) {
   SetMUXCode(iMUX);
   bitsMX1[iMUX] = ReadPin(MX1Out);
   bitsMX2[iMUX] = ReadPin(MX2Out);
  }
  // Fix the bit scrammbling done to ease trace routing
  dataBits[0][0] = bitsMX1[3];
  dataBits[0][1] = bitsMX1[0];
  dataBits[0][2] = bitsMX1[1];
  dataBits[0][3] = bitsMX1[2];
  dataBits[0][4] = bitsMX1[4];
  dataBits[0][5] = bitsMX1[6];
  dataBits[0][6] = bitsMX1[7];
  dataBits[0][7] = bitsMX1[5];
  dataBits[1][0] = bitsMX2[3];
  dataBits[1][1] = bitsMX2[0];
  dataBits[1][2] = bitsMX2[1];
  dataBits[1][3] = bitsMX2[2];
  dataBits[1][4] = bitsMX2[5];
  dataBits[1][5] = bitsMX2[7];
  dataBits[1][6] = bitsMX2[6];
  dataBits[1][7] = bitsMX2[4];
  
  // Extract the ADC values from the bits
  dataCH1 = 0;
  dataCH2 = 0;
  int datascale = 1; // scale by 3300 /255 in readout
  for (int i=0; i<8; i++) {
   dataCH1 += dataBits[0][i]*datascale;
   dataCH2 += dataBits[1][i]*datascale;
   datascale *= 2;
  }
  //cout<<dataCH1<<" "<<dataCH2<<endl;
}

void ReReadAllData()
{
  trg1CntAddr=-1;
  trg2CntAddr=-1;

  // Reread the data to see if there are any differences
 for (int i=0; i<addressDepth; i++) {
  ToggleSlowClock();
  ReadData();
  if (dataBlock[0][i] != dataCH1)
    cout << "mismatch: "<<i<<" "<<dataBlock[0][i]<<" "<<dataCH1<<"\n";
  if (dataBlock[1][i] != dataCH2)
    cout << "mismatch: "<<i<<" "<<dataBlock[1][i]<<" "<<dataCH2<<"\n";
  if (trg1CntAddr < 0 && ReadPin(Trg1Cnt)==1) trg1CntAddr=i;
  if (trg2CntAddr < 0 && ReadPin(Trg2Cnt)==1) trg2CntAddr=i;
 }
}

double CalcScalar(int nSLWCLK, int channel) {
	auto t0_ch = t0[channel];
    auto t1 = std::chrono::high_resolution_clock::now();
    double nTrigs = double(pow(2,14) - nSLWCLK); 
    //double tDiff = (t1.time_since_epoch().count()-t0_ch.time_since_epoch().count());
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0_ch);
    double tDiff = duration.count();
    cout<<tDiff<<" microseconds"<<std::endl;
	return 1000000*nTrigs/tDiff;
}

int ReadScalars() {
	//t0 = std::chrono::high_resolution_clock::now();
	if (checkScalars[0] == 0 && checkScalars[1] == 0) return 1; // return 1 if both were checked
	
	nSLWCLK += 1;
	
	// last read bits become "previous" bits
	trgCnt[0][1] = trgCnt[0][0];
	trgCnt[1][1] = trgCnt[1][0];
	//read new bits
	trgCnt[0][0] = ReadPin(Trg1Cnt);
	trgCnt[1][0] = ReadPin(Trg2Cnt);
	
	for (int ch = 0; ch < 2; ch++) {
		if (checkScalars[ch]) {
			if (trgCnt[ch][1] == 1 && trgCnt[ch][0] == 0) {
				scalars[ch] = CalcScalar(nSLWCLK, ch);
				cout << "DEBUG ==== " << nSLWCLK << std::endl;
				cout << "DEBUG ==== " << scalars[ch] << " Hz" << std::endl;
				checkScalars[ch] = 0;
				t0[ch] = std::chrono::high_resolution_clock::now();
			}
		}
	}
	return 0;
}

void ReadAllData(bool scl = 0)
{
	 if (scl) {
		 nSLWCLK = 0;
		 checkScalars[0] = 1;
		 checkScalars[1] = 1;
	 }
	 int foo = 0;
	 for (int i=0; i<addressDepth; i++) {
		  ToggleSlowClock();
		  ReadData();
		  dataBlock[0][i] = dataCH1;
		  dataBlock[1][i] = dataCH2;
		  if (scl && foo == 0) foo = ReadScalars();
	 }
	 if (scl) {
		 for (double i = addressDepth; i < pow(2,14); i++) {
			if (foo) break; 
			foo = ReadScalars();
		 }
	 }
}

void DumpData()
{
 for (int i=0; i<addressDepth; i++) {
   cout << "DATA:"<< eventNum << " " << i << " " << dataBlock[0][i] << " " <<  dataBlock[1][i] <<"\n";
   delayMicroseconds(1000);
 }
}


std::vector<double> GetArea(int dataBlock[][4096], int ch, int thr, int minWidth =1, int nQuiet = 5){
  std::vector<double> sums; 
  double sum{0};
  double interval = 31.25; // time between samples in ns
  bool isQuiet=true, isTrig=false, isDone =false;
  int width = 0;
  int imax = 0;
  double max = 0;
  //loop over waveform entries
  for(int i= nQuiet+1;i<4096; i++){
    if (isTrig ==false){
      width =0.; sum=0.; max = 0; imax =0; isQuiet=true;
      for ( int j =0; j<nQuiet;j++){
        if (dataBlock[ch][i-j-1]>thr){isQuiet=false;} 
      }
    }
    if (isQuiet && dataBlock[ch][i]>thr){
      isTrig = true;
      sum+=double(dataBlock[ch][i])*interval;
      width+=1;
 //     sums.push_back(sum);     
     // cout <<i << ' ' << width<<' ';
      if (dataBlock[ch][i]>max){ max = dataBlock[ch][i]; imax = i;}
    }else{
      isTrig=false;
      //cout<<sum<<endl;
      if (width>minWidth){ sums.push_back(sum);   sums.push_back(imax); //cout<<"ch: "<<ch<<" "<<"width "<<width <<endl; 
      }
    }
  }  
 // printV(sums); cout<<endl;
  return sums;
}


void WriteData()
{
  FILE *OutputFile, *OutputFileW;
  char data_buffer[50], data_bufferW[50];
  sprintf(data_buffer, "outputfile.txt");
  if(!is_file_exist(data_buffer))
  {
     OutputFile = fopen(data_buffer , "w");
  }
  else
  {
    OutputFile = fopen(data_buffer , "a");
  } 
  
  sprintf(data_bufferW, "outputfileWfm.txt");
  if(!is_file_exist(data_bufferW))
  {
     OutputFileW = fopen(data_bufferW , "w");
  }
  else
  {
    OutputFileW = fopen(data_bufferW , "a");
  } 
  //  fprintf(OutputFile, "%10d %04d:%02d:%02d:%02d:%02d:%02d\n",
  //      tv.tv_sec-1600042894,
  //	  tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  //  for (int i=0; i<addressDepth; i++) {
  //   fprintf(OutputFile, "%x ",dataBlock[i]);
  //   if (i%32 == 0) fprintf(OutputFile,"\n");
  //  }
  //fprintf(OutputFile, "%d %d %d %d",
//	  ievt, addressTrig, AddressReadTime, A17Val1);
  int max1=0;
  int max2=0;
  int threshold = 10;// int(float(float(256.)/float(3000.) )*50.);
  // Write waveform // or get area from waveform

  int iMax1 = 0;
  int iMax2 = 0;
  for (int i=0; i<addressDepth; i++) {
    if (dataBlock[0][i] > max1) {
      iMax1 = i;
      max1 = dataBlock[0][i];
    }
    if (dataBlock[1][i] > max2) {
      max2 = dataBlock[1][i];
      iMax2 = i;
    }
  }
  int scale = (3300/255);
  if (eventNum%saveWaveforms == 0 || eventNum<10){
    for (int i=0; i<4096; i++) {
       fprintf(OutputFileW, "DATA: %i  %i %i %i\n", eventNum, i, dataBlock[0][i]*scale,dataBlock[1][i]*scale);//"%x ",dataBlock[i]); //write to separate file
     //plot waveform
    }
  }
  
  fclose(OutputFileW);
  // Write max values for each event
  fprintf(OutputFile, "ANT: %i %i %i %i %i\n", eventNum, max1, max2, iMax1, iMax2);
  
  std::vector<double> area1 = GetArea(dataBlock,0,threshold );
  std::vector<double> area2 = GetArea(dataBlock,1,threshold );
  fprintf(OutputFile,  "AREA1: ");
  for (const auto &e :area1) {  fprintf(OutputFile,  "%f ",e);}
  fprintf(OutputFile,  "\n");
  fprintf(OutputFile,  "AREA2: ");
  for (const auto &e :area2)   fprintf(OutputFile,  "%f ",e);    
  fprintf(OutputFile,  "\n");
  //fprintf(OutputFile, "AREA: %i CH1:  %g  CH2: %g \n", eventNum, area1, area2);
  
  if (eventNum%printScalers == 0) { 
	  fprintf(OutputFile, "SCL1: %f\n", scalars[0]); // write to file every event?
	  fprintf(OutputFile, "SCL2: %f\n", scalars[1]); // 
	  cout << "Scalars: " << scalars[0] << " " << scalars[1] << std::endl;
  }
  
  auto now = std::chrono::high_resolution_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  fprintf(OutputFile, "TIME: %d\n", timestamp); 
  
  fclose(OutputFile);
  
  if (eventNum%10 == 0 ){
    cout<< "\rEvent "<<eventNum<<" Max1="<<max1<<" Max2="<<max2<<" iMax1="<<iMax1<<" iMax2="<<iMax2<<"\n"; 
    cout<<" Area1= "; printV(area1); cout<<endl; 
    cout<<" Area2= "; printV(area2); cout<<endl; 
    cout.flush();
  }
}



void StartSampling()
{
  // start filling SRAM with ADC outputs 
  eventNum++;
  for (int i=0; i<100; i++) ToggleSlowClock();
  digitalWrite(SLWCLK,1);
  digitalWrite(SLWCLK,1);
  ResetCounters();
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
}

void SoftwareTrigger()
{
  digitalWrite(SFTTRG,1);
  //digitalWrite(SFTTRG,1);
  //digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

void TestSLWCLK()
{
  int val;
  for (int i=0; i<5; i++) {
    StartSampling();
    val = ReadPin(OEbar);
    cout << "Started sampling for a new event, OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    cout << "\n";
    val = ReadPin(OEbar);
    cout << "OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    SoftwareTrigger();
    val = ReadPin(OEbar);
    cout << "Trig with SLWCLK high, OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    digitalWrite(DAQHalt,1);
  }
}

void CheckForSRAMErrors(int nEvent)
{  
  // Repeatedly read the same event and check for discrepancies
  if (nEvent<=0) return;
  cout << "Checking for SRAM errors...\n";
  for(int i=0; i<2000; i++) {
    cout << "\rEvent "<<i<<" ";cout.flush();
    StartSampling();
    delayMicroseconds(20);
    SoftwareTrigger();
    while (ReadPin(OEbar)==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    ReadAllData();
    ReReadAllData(); // Check for errors
  }
}

void ReadRandomEvents(int nEvent)
{
  // Read randomly triggered events and dump them to the screen.
  if (nEvent <= 0) return;  
  for(int i=0; i<nEvent; i++) {
    StartSampling();
    delayMicroseconds(150); // This is a bit more than one full sample at 32MHz
    SoftwareTrigger();
    while (IsTriggered()==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    ReadAllData();
    WriteData();
  }
}

void ReadEvents(int nEvent)
{
  // Read randomly triggered events and dump them to the screen.
  if (nEvent <= 0) return;  
  for(int i=0; i<nEvent; i++) {
    StartSampling();
    delayMicroseconds(1000);
    while (IsTriggered()==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    ReadAllData();
    DumpData();
  }
}

void ReadSpeedTest(int nEvent)
{
  // Repeatedly read memory and record the time
  if (nEvent <= 0) return;  
  system("date");
  for(int i=0; i<nEvent; i++) {
    ReadAllData();
  }
  system("date");
}

void BitBangSpeedTest(int nEvent)
{
  // Repeatedly read and write bits to test speed
  if (nEvent <= 0) return;  
  cout << "Write test...\n"; cout.flush();
  system("date");
  for(int i=0; i<nEvent; i++) 
    digitalWrite(MX0,1);
  system("date");

  cout << "Read test...\n"; cout.flush();
  system("date");
  for(int i=0; i<nEvent; i++)
    ReadPin(MX1Out);
  system("date");
  cout << "Direct read test...\n"; cout.flush();
  system("date");
  for(int i=0; i<nEvent; i++)
    digitalRead(MX1Out);
  system("date");
}

void TestTrigLatch(int nEvents) {
  digitalWrite(TrgExtEn,0);
  digitalWrite(Trg1En,1);
  digitalWrite(Trg2En,1);
  digitalWrite(PSCL,1);
  digitalWrite(DAQHalt,0);
  digitalWrite(SFTTRG,0);
  DoErrorChecks = false;
  int i{0};
  while (i<nEvents) {
    StartSampling();
    //  SoftwareTrigger();
    while (IsTriggered()==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    //cout << "Event OEbar="<<ReadPin(OEbar)<<"\n";
    ReadAllData();
    //DumpData();
    WriteData();
    //ToggleSlowClock();
    i++;
    digitalWrite(PSCL,0);
    if (i%10 == 0) {
      cout << "\rEvent "<<i<<" of "<<nEvents; cout.flush();
      digitalWrite(PSCL,1);
    }
  }
}

void TakeCosmicData(int nEvents) {
  int i{0};
  while (i<nEvents) {
    StartSampling();
    delayMicroseconds(100);
    while (IsTriggered()==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    ReadAllData();
    WriteData();
    i++;
    /*
    digitalWrite(PSCL,0);
    if (i%10 == 0)
      digitalWrite(PSCL,1); // Allow single channel triggers every 10th event
    */
  }
}


void TakeData(int nEvents, bool isRandom=false) {// Better than ReadRandomEvents
// takes a set of events 
  int i{0};
  //random DAQ
  // Read randomly triggered events and dump them to the screen.
  
  if (nEvents <= 0) return;
  
  auto t_now = std::chrono::high_resolution_clock::now(); // record time at beginning of data collection
  t0[0] = t_now;
  t0[1] = t_now;
  bool scl;
  
  while (i<nEvents) {
	StartSampling();//clear DAQHalt to enable sampling again 
    delayMicroseconds(150); 
    if (isRandom)     SoftwareTrigger();
    
    while (IsTriggered()==0) {
      delayMicroseconds(2);
    }
    
    // hold DAQHalt high to stop sampling 
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
	
	
	scl = (i%printScalers==0) ? 1 : 0;
    ReadAllData(scl);//read SRAM 
    WriteData();
    //if (i%10==0) {
	ifstream p("stop");// if "stop" file exists -> stop sampling
	if(p.good()) { remove("stop"); exit(0); }
	//}
    i++;    
  }  
}

void setConfig(const char* configFile){
  FILE *OutputFile, *OutputFileW;
  char data_buffer[50], data_bufferW[50];
  sprintf(data_buffer, "outputfile.txt");
  if(!is_file_exist(data_buffer))
  {
     OutputFile = fopen(data_buffer , "w");
  }
  else
  {
    OutputFile = fopen(data_buffer , "a");
  } 

  ifstream cfile(configFile, ios::out);
    if (cfile.is_open()){ //checking whether the file is open
      string line;
      while(getline(cfile, line)){ //read data from file object and put it into string.
         cout << line << "\n"; //print the data of the string
         if( line.empty() || line[0] == '#' )
            {
                continue;
            }
            auto delimiterPos = line.find("=");
	        auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            std::cout << name << " " << value << '\n';
            fprintf(OutputFile,"%s %s\n", name.c_str(), value.c_str());
	    if (name.compare("Threshold1 ")==0 )SetTriggerThreshold(1, stoi(value), 0); // Threshold1 = 300
	    if (name.compare("Threshold2 ")==0)SetTriggerThreshold(2, stoi(value), 0);
	    // if (name.compare("PSCL ")==0 ) digitalWrite(PSCL,stoi(value)); //prescale, 0 for AND/1 for OR, unnecessary since adding PSCLduty
	    if (name.compare("saveWaveforms ")==0)saveWaveforms = stoi(value);
	    if (name.compare("bias1 ")==0)  SetBias(1, stoi(value), 0);
	    if (name.compare("bias2 ")==0)  SetBias(2, stoi(value), 0);
	    if (name.compare("PSCLduty ")==0) PSCLduty = int(stoi(value)*1024/100);
	    if (name.compare("nEvents ")==0 ){nEvents = stoi(value); cout<<"There are "<<nEvents<<" events in the config"<<endl;} 
	    if (name.compare("printScalers ")==0)printScalers = stoi(value);
      }
      cfile.close();
   }
   fclose(OutputFile);
}

int main(int argc, char* argv[]){
  bool configOnly{0};
  if (argc ==2){istringstream(argv[1])>>configOnly;}
  const char* configFile = "Config.txt";
  wiringPiSetup(); //Setup GPIO Pins, etc. to use wiringPi and wiringPiI2C interface
  pinMode(MX0,OUTPUT);
  pinMode(MX1,OUTPUT);
  pinMode(MX2,OUTPUT);
  pinMode(MX1Out,INPUT);
  pinMode(MX2Out,INPUT);
  pinMode(SLWCLK,OUTPUT);
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
  pinMode(PSCL,PWM_OUTPUT);
  
  // Set up I2C
  Thr1DACfd = wiringPiI2CSetup(Thr1DACAddr);
  cout<< "The address is "<< wiringPiI2CSetup(Thr1DACAddr) << endl;
  cout<< wiringPiI2CSetup(Thr2DACAddr) << endl;
  Thr2DACfd = wiringPiI2CSetup(Thr2DACAddr);
  Bias1DACfd = wiringPiI2CSetup(Bias1DACAddr);
  Bias2DACfd = wiringPiI2CSetup(Bias2DACAddr);
  ADCfd = wiringPiI2CSetup(ADCaddr);
 // EEPROMfd = wiringPiI2CSetup(EEPROMAddr);

/////////////////////////////////// MAKE CHANGES HERE //////////////////////////////////
 
 // Enable software/external triggers 
  digitalWrite(TrgExtEn,1);
  // Enable ch1 and ch2 triggers 
  digitalWrite(Trg1En,1);
  digitalWrite(Trg2En,1);
  
  /*
   * use these to set PWM.
   * pwmSetRange(n) sets the number of steps in each cycle to be n
   * pwmWrite(pin, m) sets the number of "ON" steps to be m
  */
  pinMode(PSCL,PWM_OUTPUT);
  pwmSetMode(PWM_MODE_MS);
  pwmSetRange(1024);
  
  
  digitalWrite(DAQHalt,0);
  digitalWrite(SFTTRG,0);

  DoErrorChecks = true;
  //int nEvents = 20000;
  setConfig(configFile);

  // digitalWrite(PSCL,1); // force OR
  pwmWrite(PSCL, PSCLduty);
  cout<<"PWM duty cycle: "<<int(100*PSCLduty/1024)<<"%"<<endl;
  
  cout<<"Reading ADC AIN0: "<<ReadADC(0)<<endl;
  cout<<"Reading ADC AIN1: "<<ReadADC(1)<<endl;
  cout<<"Reading ADC AIN2: "<<ReadADC(2)<<endl;
  cout<<"Reading ADC AIN3: "<<ReadADC(3)<<endl;
  if(configOnly){ return 0;}
  TakeData(nEvents, true);//TakeCosmicData(10000); // Set the number of events here// true for random triggering, false for normal triggering  
    
  // ReadRandomEvents(10000);
  // TestTrigLatch(1000); // Specialized tests for probing trigger latch
  
  exit(0);

  SetTriggerThreshold(1,1000,0);
  SetTriggerThreshold(2,1000,0);
  // ReadEvents(1000);
  
  //TestTrgThr(2);

  DoErrorChecks = true;
  CheckForSRAMErrors(0);

  DoErrorChecks = true;
  ReadSpeedTest(1000);

  DoErrorChecks = true;
  BitBangSpeedTest(100000000);

  //ReadRandomEvents(1);
  
  cout << nReadErrors<<" bit errors out of "<<nBitsRead<<" = "<<(100.*nReadErrors)/nBitsRead<<"%\n";

  return 0;

}
