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

// helper function for trigger rate measurement
double CalcScalar(int nSLWCLK, int channel, double tDiff) {
    double nTrigs = double(pow(2,14) - nSLWCLK); 
    cout<<tDiff<<" microseconds"<<std::endl;
	return 1000000*nTrigs/tDiff;
}

// returns false if the count is full 
bool Trg1CntOut(){
    return (ReadPin(Trg1Cnt)==0|| 
            ReadPin(Trg1Cnt)==0||
            ReadPin(Trg1Cnt)==0||
            ReadPin(Trg1Cnt)==0);
    }

void testTrigRate(){

    bool trg1 = false; 
    bool trg2 = true; 
    //int addressDepth = 4096; 

    digitalWrite(PSCL, 1);

    // start time measurement 
    // read the value in Trg1CntOut
    // then count the number of clock cycles 
    // needed for Trg1CntOut to go high 
    cout<<"Starting sampling"<<endl;

    int ntests = 1000; 
    for(int i=0; i!=ntests;i++){

        // Reset counters 
        ResetCounters();
        ResetCounters(); 
        ResetCounters();
        cout<<"Counters reset"<<endl;

        // Enable sampling (then wait to clear out stale data)
        if (trg1) {
            digitalWrite(Trg1En, 0);
            digitalWrite(Trg1En, 0);
            digitalWrite(Trg1En, 0);
            digitalWrite(Trg1En, 1);
            digitalWrite(Trg1En, 1);
            digitalWrite(Trg1En, 1);
            }

        if (trg2) {
            digitalWrite(Trg2En, 0);
            digitalWrite(Trg2En, 0);
            digitalWrite(Trg2En, 0);
            digitalWrite(Trg2En, 1);
            digitalWrite(Trg2En, 1);
            digitalWrite(Trg2En, 1);
            }

        digitalWrite(DAQHalt,0);
        digitalWrite(DAQHalt,0);
        digitalWrite(DAQHalt,0);

        auto t0 = std::chrono::high_resolution_clock::now(); 
        while(ReadPin(Trg2Cnt)==0);
        auto t1 = std::chrono::high_resolution_clock::now();  
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0);
        double tDiff = duration.count();
        WriteDouble(double(1024/tDiff),"TrigRateCH2.csv");
        delayMicroseconds(100); 
    }
    /*
        while (ReadPin(OEbar) == 1);
    digitalWrite(DAQHalt,1);
    auto t1 = std::chrono::high_resolution_clock::now();    

    // READ SRAM 
    ResetCounters();
    int N1 = 0; // count until Trg1Cnt goes high
    while(ReadPin(Trg1Cnt)==0){
        ToggleSlowClock();
        N1++; 
    }

    cout<<"Trg1Cnt Memory Depth: "<<N1<<endl;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0);
    double tDiff = duration.count();

    double nTrigs =  double(pow(2,11) - N1);  
    cout<<nTrigs<<" in "<<tDiff<<" microseconds"<<std::endl;
	cout<<nTrigs/tDiff<<"kHz"<<endl;
    */


   //ResetCounters(); // takes 3 uS 
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
    BiasDAC DAC2 = BiasDAC(2); 

    DAC1.SetVoltage(220);
    DAC2.SetVoltage(1); 
    delayMicroseconds(10000);

    cout<<"Starting test samples"<<endl;

    digitalWrite(PSCL, 1); 

    int event = 0; 
    while(event < nEvents){
        cout<<event<<endl;
        StartSampling(false, false, true); 
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
    IO test_IO = IO(); 
    test_IO.setClock(20);
}

void testDIGIO(){

    DIGIO digio_test(2); 

    int ntests = 1000;
    double times[ntests]; 
    int nErrors = 0;

    for(int i = 0; i!=ntests; i++){

        unsigned int test_byte = 0b1<<(i%8); 

        digio_test.setIOState(test_byte);
        auto t0 = std::chrono::high_resolution_clock::now(); 
        unsigned int result = digio_test.readIOState(); 
        auto t1 = std::chrono::high_resolution_clock::now();  
        
        if(test_byte!=result){nErrors++;}
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0);
        double tDiff = duration.count();
        times[i] = tDiff; 
        WriteDouble(tDiff,"WriteTimes_1000.csv");
        digio_test.setIOState(0b0);
    }

    int sum = 0;

    for (auto& n : times){
        sum += n;
    }
    float average = sum/ntests;

    cout<<"Average read time: "<<average<<" microseconds"<<endl;
    cout<<nErrors<<" errors in "<<ntests<<" reads"<<endl;
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
                cout<<"Running IO Test"<<endl;
                testIO();
            }

            else if (arg == "Pulse"){
                setupComponents();
                cout<<"Running Pulse Test"<<endl;
                testCalibPulse(true); 
            }

            else if (arg == "TrgRate"){
                setupComponents(); 
                cout<<"Running trigger rate test"<<endl;
                testTrigRate(); 
            }

            else if (arg == "DIGIO"){
                setupComponents(); 
                cout<<"Running GPIO Expander Test"<<endl;
                testDIGIO(); 
            }

            else{
                cout<<"ERROR: "<<arg<<" is not a test option \n"<<endl;
            }

        }
    return 0;
}
