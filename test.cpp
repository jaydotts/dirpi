// test functions 

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

bool testDigiPot(bool chan1 = true, bool chan2 = true){
    cout<<"Sweeping wipers from 0-255"<<endl;
    DIGIPOT digi = DIGIPOT();
    
    for(int i = 0; i<255; i++){
        digi.SetWiper(1,i);
        digi.SetWiper(2,i);
        delayMicroseconds(200);
    }
    return true;
};

bool testCalibPulse(bool ch1, int nSamples = 100){

    bool PULSE_OK = false; 
    const char* output_fname = "outputfile.txt";
    
    // initialize the component 
    BiasDAC DAC1 = BiasDAC(1); 
    //BiasDAC DAC2 = BiasDAC(2); 

    DAC1.SetVoltage(220);
    delayMicroseconds(10000);

    cout<<"Starting test samples"<<endl;

    digitalWrite(PSCL, 1); 

    int event = 0; 
    while(event < nEvents){
        cout<<event<<endl;
        StartSampling(false, true, false); 
        delayMicroseconds(200); 

        ToggleCalibPulse(); 

        while (ReadPin(OEbar) == 1);

        digitalWrite(DAQHalt,1);

        // load SRAM data into global integer array, dataBlock
        readSRAMData(); 

        // dump the event data into the writeFile
        WriteSRAMData(event,output_fname);

        event++;
    }

    return PULSE_OK; 
}

bool testIO(){
    IO GPIO = IO(); 
    GPIO.setClock(20);
}

int main(int argc, char **argv){

    setupPins();

    // Replace this with a test run config file
    LoadRunConfiguration("config/test_config.config");

    for (int i = 1; i < argc; ++i){

        string arg = argv[i];
        cout<<arg<<": \n"<<endl;

            if (arg == "digipot"){
                cout<<"Running Test of Digital Potentiometer"<<endl;
                testDigiPot();
            }

            else if (arg == "slowclock"){
                cout<<"Running Test of SlowClock"<<endl;
                TestSLWCLK();
            }

            else if (arg == "GPIO"){
                cout<<"Running GPIO Test"<<endl;
                testIO();
            }

            else if (arg == "Pulse"){
                setupComponents();
                cout<<"Running Pulse Test"<<endl;
                testCalibPulse(true); 
            }

            else{
                cout<<"ERROR: "<<arg<<" is not a test option \n"<<endl;
            }

        }
    return 0;
}
