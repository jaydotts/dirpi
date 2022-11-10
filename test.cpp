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

bool testDigiPot = false; 
bool testDataRW = true; 

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
  StartSampling();
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

int main(){

setupPins();

if(testDigiPot){
    DIGIPOT digi = DIGIPOT();
    digi.setInputBias(2,150);
}

if(testDataRW){
    digitalWrite(DAQHalt,0);
    digitalWrite(SFTTRG,0);

    DebugTests(); // Varying debugging tests
}

return 0;
}
