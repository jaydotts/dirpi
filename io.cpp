// Handles data I/O operations such as SRAM read/write, 
// sampling start/stop, and trigger handling.

#include "setup.cpp"

///////////////////////////////////// Basic I/O

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
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
}

void ResetCounters() // Reset the address counters
{
 if (debug) {cout << "Reseting the address counters.\n"; cout.flush();}
 //cout<<"Resetting Address Counters"<<endl;
 // Toggle high then low.
 // Repeat to avoid rare failures.
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

void StartSampling(bool extrg, bool trg1, bool trg2)
{
  eventNum++;
  ToggleSlowClock();
  digitalWrite(SLWCLK,1);
  digitalWrite(SLWCLK,1);

  // Disable all triggers 
  digitalWrite(TrgExtEn, 0); 
  digitalWrite(TrgExtEn, 0); 
  digitalWrite(TrgExtEn, 0); 

  digitalWrite(Trg1En, 0);
  digitalWrite(Trg1En, 0);
  digitalWrite(Trg1En, 0);

  digitalWrite(Trg2En, 0);
  digitalWrite(Trg2En, 0);
  digitalWrite(Trg2En, 0);

  // Enable sampling (then wait to clear out stale data)
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);
  digitalWrite(DAQHalt,0);

  // Reset counters 
  ResetCounters();
  ResetCounters(); 
  ResetCounters();

  // Reset all MX bits back to 0
  digitalWrite(MX0,0);
  digitalWrite(MX1,0);
  digitalWrite(MX2,0);
  
  // Wait ~ 210 uS for 20MHz clock)
  delayMicroseconds(1);

  // Enable triggers 
  if (trg1) {
    digitalWrite(Trg1En, 1);
    digitalWrite(Trg1En, 1);
    digitalWrite(Trg1En, 1);
  }

  if (trg2) {
    digitalWrite(Trg2En, 1);
    digitalWrite(Trg2En, 1);
    digitalWrite(Trg2En, 1);
  }

  if(extrg){
    digitalWrite(TrgExtEn, 1); 
    digitalWrite(TrgExtEn, 1); 
    digitalWrite(TrgExtEn, 1);  
  }
}

void SoftwareTrigger(int nhigh)
{
  for(int i=0; i<nhigh; i++) 
   digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

void SoftwareTrigger() {
  SoftwareTrigger(3);
  delayMicroseconds(1); 
  SoftwareTrigger(3);
  delayMicroseconds(1); 
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

void ReReadData()
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

void readSRAMData(){
    for (int i=0; i<addressDepth; i++) {
      ToggleSlowClock();
      ReadData();
      dataBlock[0][i] = dataCH1;
      dataBlock[1][i] = dataCH2;
    }
    //debug
    ReReadData();
}

// helper for writeSRAMData
bool isfile(const char *fileName){
    std::ifstream infile(fileName);
    return infile.good();
}

void WriteSRAMData(int eventNum, const char *fname){

    // Make buffers for waveform data
    FILE *OutputFile;
    char data_buffer[50];
    sprintf(data_buffer, fname);
    
    if(!isfile(data_buffer)){
        OutputFile = fopen(data_buffer , "w ");
    }
    else{
        OutputFile = fopen(data_buffer , "a");
    } 

   // if (eventNum%N == 0 || eventNum<nEvents){
    for (int i=0; i<4096; i++) {
       fprintf(OutputFile, "DATA: %i  %i %i %i\n", eventNum, i, dataBlock[0][i],dataBlock[1][i]);
     //plot waveform
    //}
  };

  fclose(OutputFile);
  
}

void ToggleCalibPulse(){
  cout<<"Sending pulse"<<endl;
  digitalWrite(Calib, 1); 
  digitalWrite(Calib, 1); 
  digitalWrite(Calib, 1); 
  delayMicroseconds(1); 
  digitalWrite(Calib, 0); 
  digitalWrite(Calib, 0); 
}
