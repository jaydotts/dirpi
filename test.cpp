

// The essential digi functions are: 
// - Set thresholds 
// - Trigger 
// - Record data 
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

// Set up all the pins 
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

#define WRITEDAC 0x40
//#define WRITEDACEEPROM 0x50 //0x50?
#define Thr1DACAddr 0x60
#define Thr2DACAddr 0x61
#define Bias1DACAddr 0x4c
#define Bias2DACAddr 0x4d
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


void SetupWires(){
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
  cout<< wiringPiI2CSetup(Thr2DACAddr) << endl;
  Thr2DACfd = wiringPiI2CSetup(Thr2DACAddr);
  Bias1DACfd = wiringPiI2CSetup(Bias1DACAddr);
  Bias2DACfd = wiringPiI2CSetup(Bias2DACAddr);
  ADCfd = wiringPiI2CSetup(ADCaddr);
}


// Read a wPi pin with error checking
int ReadPin(int iPin) {
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

// Clear address counters 
void ADRCounterClear()
{
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

// Activate software trigger: manually pulls ExtTrig high 
void SoftwareTrigger(){
  digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

// Read the OEBar pin 
int IsTriggered() {
  for (int i=0; i<100; i++)
   if (ReadPin(OEbar) == 1) return 0;
  return 1; 
}

//The address counters are advanced by the 
// AddrCLK signal which is equal to CLK during 
// sampling but equal to SLWCLK* during readout. 
// (The sampling vs readout mode is controlled by OE*). 
// This allows the address to be advanced at the full CLK 
// speed during sampling but then be directly controlled 
// by the RPi using SLWCLK during readout. To accomplish this, 
// the RPi will hold SLWCLK high during sampling and then toggle 
// it hi-low-hi to advance AddrCLK during readout.

void ToggleSlowClock() // Toggle the slow clock down then up
{
 // Repeat twice to avoid rare errors.
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,0);
}

// Set DAQHalt low to activate/enable sample collection
void StartSampling(){
  for (int i=0; i<100; i++) ToggleSlowClock();
  digitalWrite(SLWCLK,1);
  digitalWrite(SLWCLK,1);
  ADRCounterClear(); // clear the address counter 
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
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

// select low, middle, and high bits of the mux data
// corresponding to ADC bit of interest 
void MUXSelectBits(int bit){
  if (bit<0 || bit > 7) {cerr <<bit<< " is an invalid MUX bit\n"; cerr.flush();}

  int MXCODEA = 0;
  int MXCODEB = 0;
  int MXCODEC = 0;

  // Crude hack to set MUX bits.
  // pull bits off of SRAM using their
  // mapped locations in the multiplexers 
  
  MXCODEA = bit%2;
  if (bit>3) 
    MXCODEB = (bit-4)/2; 
  else
    MXCODEB = bit/2;
  MXCODEC = bit/4;

  // write MX0, MX1, MX2 with the bits pulled off the multiplexer 
  // and written to their appropriate pins
  digitalWrite(MX0,MXCODEA);
  digitalWrite(MX1,MXCODEB);
  digitalWrite(MX2,MXCODEC);

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
  cout<<"DACfd:"<<DACfd<<endl;
 // DACfd = DACfd*2; //add a zero to indicate writing to cat
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

// read both of the 8-bit multiplexers 
void MUXRead(){
  // set up the 8 bits for the muxs
  int bitsMX1[8];
  int bitsMX2[8];

  for (int MUXbit = 0; MUXbit<8; MUXbit++){
    // select input bits on MUX corresponding to 
    // appropriate SRAM bit 
    MUXSelectBits(MUXbit); 

    // Load data bit of multiplexer (now holding the proper bits) from CH1 and CH2
    bitsMX1[MUXbit] = ReadPin(MX1Out);
    bitsMX2[MUXbit] = ReadPin(MX2Out);
    //cout<<"MX1Out:"<<bitsMX1[MUXbit]<<endl;
    //cout<<"MX2Out:"<<bitsMX2[MUXbit]<<endl;

  }

  // Fix the SRAM->MUX bit scrammbling done to ease trace routing
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

  // Extract the actual ADC values (voltage readings) from the bits
  dataCH1 = 0;
  dataCH2 = 0;

  // write data values to dataCH1 and dataCH2 bit by bit, advancing through them one at a time

  int binscale = 1; // binary scaling factor for each bit
  for (int i=0; i<8; i++) {
    // append value of scaled bit to dataCH1 and dataCH2 as 8-bit binary numbers
   dataCH1 += dataBits[0][i]*binscale;
   dataCH2 += dataBits[1][i]*binscale;
   binscale *= 2; 
  }
}

// Read 8 bits of data into SRAM 
void SRAMRead(){

  // i: number of bins (samples) cycled through. = 4096
  for (int i=0; i<addressDepth; i++){
    ToggleSlowClock(); // toggle slow clock hi-lo, so that it will advance adress forward on a high input
    MUXRead(); // read the 16 bits from the multiplexer for this event 
    // load location in the SRAM data block with this byte of data into databits 
    dataBlock[0][i] = dataCH1;
		dataBlock[1][i] = dataCH2;
  }

}

// output data from SRAM to the screen 
void DumpData(double scale)
{

  for (int i=0; i<addressDepth; i++) {
    cout << "DATA:"<< eventNum << " " << i << " " << dataBlock[0][i]*scale << " " <<  dataBlock[1][i]*scale <<"\n";
    delayMicroseconds(1000);
 }
}

bool is_file_exist(const char *filename)
{ 
 std::ifstream infile(filename);
 return infile.good();
}

void printV( std::vector<double> const &a){
  for (int i{0};i<a.size();i++){
    cout<< a.at(i)<< ' ';
  }
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
  
  if (eventNum%saveWaveforms == 0 || eventNum<100){
    for (int i=0; i<4096; i++) {
      int CH1reading = dataBlock[0][i]; 
      int CH2reading = dataBlock[1][i];

       fprintf(OutputFileW, "DATA: %i  %i %i %i\n", eventNum, i, CH1reading, CH2reading);//"%x ",dataBlock[i]); //write to separate file
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

void TestSoftwareTrigger(int nEvents, bool isRandom = false){

  if (nEvents==0) return; 

  // record time at beginning of data collection
  auto t_now = std::chrono::high_resolution_clock::now(); 


  t0[0] = t_now;
  t0[1] = t_now;

  // wait for events 
  int i = 0; 

  while(i<nEvents){
  
    StartSampling(); // reset counters + pull DAQHalt low 

    delayMicroseconds(150); 

    SoftwareTrigger();

    // infinite loop - waits for a trigger 
    while (IsTriggered() == 0){
      delayMicroseconds(2);
    }

    // when triggered, stop sampling by holding DAQHalt Hi
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    //cout<< "DAQHalt is :"<< (digitalRead(DAQHalt) ? "high" : "low") << endl;

    // Read SRAM into dataBlock
    SRAMRead();

    // Write data to file 
    WriteData();
    eventNum++;
    i++;

    if (1000%nEvents) cout<<i<<" triggers occurred"<<endl;
  }

  cout<<"Data collection completed"<<endl;
}

void SRAMTestReadWrite(int nEvents){

  // Set DAQHalt low to enable Write mode on SRAM
  StartSampling();

  delayMicroseconds(150);

  int bitsMX1[8];
  int bitsMX2[8];

  // Clear each bit manually
  cout<<"clearing bits"<<endl;
  for (int MUXbit = 0; MUXbit<8; MUXbit++){
    MUXSelectBits(MUXbit);
    digitalWrite(MX1Out,0);
    digitalWrite(MX2Out,0);

    cout<<digitalRead(MX1Out)<<endl;;
    cout<<digitalRead(MX2Out)<<endl;
  }

    delayMicroseconds(2);  

    cout<<"Activating SRAM read mode"<<endl; 

    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);

    SRAMRead();

    WriteData();

}

int main(int argc, char* argv[]){
  bool configOnly{0};
  if (argc ==2){istringstream(argv[1])>>configOnly;}
  const char* configFile = "Config.txt";

  SetupWires(); 
 
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
  
  
  digitalWrite(DAQHalt,0); // sampling enabled
  digitalWrite(SFTTRG,0); // software trigger off 

  DoErrorChecks = true;
  //int nEvents = 20000;
  //setConfig(configFile);

  // digitalWrite(PSCL,1); // force OR
  pwmWrite(PSCL, PSCLduty);
  cout<<"PWM duty cycle: "<<int(100*PSCLduty/1024)<<"%"<<endl;

  cout<<"Testing software trigger"<<endl; 
  TestSoftwareTrigger(1000,true);
  //SRAMTestReadWrite(1000);
  return 0;

}