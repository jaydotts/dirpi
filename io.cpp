
// Dedicated IO processes, data collection, etc.

#include "setup.cpp"

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

