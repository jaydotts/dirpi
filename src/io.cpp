#include "../include/io.h"
#include "../include/setup.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
using namespace std; 

int ReadPin(int iPin) // Read a wPi pin with error checking
{
 int i1;
 i1 = digitalRead(iPin);
  int i2, i3;
  i2 = digitalRead(iPin);
  i3 = i2; // Only do two reads for the first test. Do triplicate checking afterward if these don't match.
  int iRead = 0;
  while ((i1 != i2 || i1 != i3 || i2 != i3) && iRead<1000) {
   i1 = digitalRead(iPin);
   i2 = digitalRead(iPin);
   i3 = digitalRead(iPin);
   iRead++;
  }
  if (iRead >= 1000) {cerr << "Bit mismatch after 1000 reads.\n"; cout.flush();}
 return i1;
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

void ToggleSlowClock() // Toggle the slow clock down then up
{
    // Repeat twice to avoid rare errors.
    digitalWrite(SLWCLK,0);
    digitalWrite(SLWCLK,0);
    digitalWrite(SLWCLK,0);
    digitalWrite(SLWCLK,1);
    digitalWrite(SLWCLK,1);
    digitalWrite(SLWCLK,1);
}

void reset_MUX(){ 
    // Reset all MX bits back to 0
    digitalWrite(MX0,0);
    digitalWrite(MX1,0);
    digitalWrite(MX2,0);
}