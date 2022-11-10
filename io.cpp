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

void ToggleSlowClock() // Toggle the slow clock down then up
{
 // Repeat twice to avoid rare errors.
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,0);
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

int IsTriggered() {
  for (int i=0; i<100; i++)
   if (ReadPin(OEbar) == 1) return 0;
  return 1; 
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
}

void StartSampling()
{
  eventNum++;
  for (int i=0; i<10; i++) ToggleSlowClock();
  digitalWrite(SLWCLK,1);
  digitalWrite(SLWCLK,1);
  ResetCounters();
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
}

void SoftwareTrigger(int nhigh)
{
  for(int i=0; i<nhigh; i++) 
   digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

void SoftwareTrigger() {
  SoftwareTrigger(3);
}