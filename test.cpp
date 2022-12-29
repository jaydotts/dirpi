// quick tests 

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
#include "io.cpp"
using namespace std;


// Enable / Disable test modes here 
bool testDigiPot = true; 
bool testDataRW = false; 
bool testDAC = true; 
bool testSampling = false;
bool testTrgCount = false;


void DebugTests()
{
  ResetCounters();
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,1);// Makes WE* high and OE* low
  digitalWrite(MX0,0);
  digitalWrite(MX1,0);
  digitalWrite(MX2,0);
  digitalWrite(TrgExtEn,1);
  //StartSampling();
  delayMicroseconds(500);
  SoftwareTrigger(3);
  SoftwareTrigger(3);
  SoftwareTrigger(3);
  //delayMicroseconds(1);

  digitalWrite(DAQHalt,1);// Makes WE* high and OE* low

  for (int i=0; i<addressDepth; i++) {
          ToggleSlowClock();
          ReadData();
          dataBlock[0][i] = dataCH1;
          dataBlock[1][i] = dataCH2;
		  if ((i+3)%10==0) cout <<i<<" "<<dataCH1 << " " << dataCH2<<"\n";
		  //cout << i << " " << dataCH1 << " " << dataCH2<<"\n";
         }
}

void TestSLWCLK()
{
int val;
for (int i=0; i<5; i++) {
    //StartSampling();
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

void testTriggerCount(int nTriggers, bool trg1, bool trg2){

    digitalWrite(PSCL,0);
    digitalWrite(PSCL,0);
    digitalWrite(PSCL,0);

    if (trg1) {
        digitalWrite(Trg1En, 1);
        digitalWrite(Trg1En, 1);
        digitalWrite(Trg1En, 1);
        cout<<"Trig1 Enabled"<<endl;
    }

    if (trg2) {
        digitalWrite(Trg2En, 1);
        digitalWrite(Trg2En, 1);
        digitalWrite(Trg2En, 1);
        cout<<"Trig2 Enabled"<<endl;
    }

    int count = 0;

    auto t1 = std::chrono::high_resolution_clock::now();
    //StartSampling(); 
    //cout<<ReadPin(Trg1Cnt)<<endl;
    while(ReadPin(Trg1Cnt) == 0);
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> fp_us = t2 - t1;
 
    // integral duration: requires duration_cast
    //auto int_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
    cout <<"2048 trigger primitives in " << fp_us.count() << " us"<<endl;
    
    double trig_rate = 2048 / fp_us.count(); 

    cout<<"Trigger frequency: "<<trig_rate*1000<<" kHz"<<endl;
}

void testTriggerDetect(int nTriggers, bool trg1, bool trg2){
    
    digitalWrite(TrgExtEn, 1); 

    if (trg1) {
        digitalWrite(Trg1En, 1);
        digitalWrite(Trg1En, 1);
        digitalWrite(Trg1En, 1);
        cout<<"Trig1 Enabled"<<endl;
    }

    if (trg2) {
        digitalWrite(Trg2En, 1);
        digitalWrite(Trg2En, 1);
        digitalWrite(Trg2En, 1);
        cout<<"Trig2 Enabled"<<endl;
    }
    int count = 0;

    auto t1 = std::chrono::high_resolution_clock::now();
    //StartSampling();
    while(count <= nTriggers){
        while (ReadPin(OEbar) == 1);
        count++;
        //StartSampling();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
 
    // floating-point duration: no duration_cast needed
    std::chrono::duration<double, std::micro> fp_us = t2 - t1;
 
    // integral duration: requires duration_cast
    //auto int_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
 
    cout << nTriggers<< " triggers in " << fp_us.count() << " us"<<endl;

    double trig_rate = nTriggers / fp_us.count(); 

    cout<<"Trigger frequency: "<<trig_rate*1000<<" kHz"<<endl;

    
    }

// test code to bypass the trigger and just read data out of the 
// SRAM at one snapshot of time while all triggers 
// are disabled
void testForcedSampling(int nEvents){

    const char* output_fname = "outputfile.txt";

    digitalWrite(Trg2En, 0);
    digitalWrite(Trg2En, 0);
    digitalWrite(Trg2En, 0);
    cout<<"Trig2 Disabled"<<endl;

    digitalWrite(Trg1En, 0);
    digitalWrite(Trg1En, 0);
    digitalWrite(Trg1En, 0);
    cout<<"Trig1 Disabled"<<endl;

    int i =0;
    while(i<nEvents){
        ResetCounters();
        digitalWrite(DAQHalt,0);
        digitalWrite(DAQHalt,0);
        digitalWrite(DAQHalt,1);
        digitalWrite(MX0,0);
        digitalWrite(MX1,0);
        digitalWrite(MX2,0);

        //digitalWrite(TrgExtEn,1);
        //StartSampling();
        // test what happens when SFTTRG is called before readout
        SoftwareTrigger();

        // sample for 500 uS 
        delayMicroseconds(500);

        digitalWrite(DAQHalt,1);// Makes WE* high and OE* low   

        for (int i=0; i<addressDepth; i++) {
            ToggleSlowClock();
            ReadData();
            dataBlock[0][i] = dataCH1;
            dataBlock[1][i] = dataCH2;
            }
        
        WriteSRAMData(i,output_fname);
        cout<<"Event "<<i<<" recorded"<<endl;
        i++;
        ResetCounters();
    }
}

int main(){

    setupPins();

    if(testDigiPot){
        cout<<"Testing DigiPot"<<endl;
        DIGIPOT digi = DIGIPOT();
        digi.SetWiper(1,255);
        digi.SetWiper(2,255);
    }

    if(testDAC){

        cout<<"Setting DACs"<<endl;
        ThrDAC DAC1 = ThrDAC(1); 
        ThrDAC DAC2 = ThrDAC(2); 

        DAC1.SetThr(1000,0); 
        DAC2.SetThr(1000,0);

    }

    if(testSampling){
        //testForcedSampling(2);
    }

    if(testTrgCount){
        testTriggerDetect(1000, true, false);
    }

    if(testDataRW){
        cout<<"Testing ADC data read/write"<<endl;
        digitalWrite(DAQHalt,0);
        digitalWrite(SFTTRG,0);

        DebugTests(); // Varying debugging tests
    }

    return 0;
}
