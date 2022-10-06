
// Dedicated script for rapid read/write data 
// and management of memory devices 

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


////// Basic I/O 
int ReadPin(int iPin){ // Read a wPi pin with error checking
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

void ReadAllData(bool scl = 0){
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

void ReReadAllData(){
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

void WriteData(){
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
  
  if (eventNum%saveWaveforms == 0 || eventNum<10){
    for (int i=0; i<4096; i++) {
       fprintf(OutputFileW, "DATA: %i  %i %i %i\n", eventNum, i, dataBlock[0][i],dataBlock[1][i]);//"%x ",dataBlock[i]); //write to separate file
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

void DumpData(){
 for (int i=0; i<addressDepth; i++) {
   cout << "DATA:"<< eventNum << " " << i << " " << dataBlock[0][i] << " " <<  dataBlock[1][i] <<"\n";
   delayMicroseconds(1000);
 }
}

////// MUX
void SetMUXCode(int val){ // Set the multiplexer code
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

void ReadData(){ // Read the 16 data bits into the global variable array

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

///// ADC 
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

////// EEPROM 
void readEEPROM(){
	int DACfd;
//	DACfd = EEPROMfd
//	wiringPiI2CWrite(DACfd, WRITEDACEEPROM);

//     data = wirinPiI2CRead();
    // while data !="":
       // do stuff
	//start condition, dev select, W bit up 
	
}
