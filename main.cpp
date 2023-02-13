
#include "io.cpp"
using namespace std;


// begin data recording 
void takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData){

  // extrg - whether or not to enable external triggers 
  // sftrg - whether or not to trigger using software trigger 
  // trg1 and trg2 - enable triggering on either of these channels 

  const char* output_fname = "outputfile.txt";

  // enable trigger based on passed parameter 
  // todo: set trigEns low 

  while(eventNum < nEvents){
    cout<<eventNum<<endl;

    // Toggle SLWCLCK high, Reset counters, Deassert DAQHalt
    // to enable sampling mode
    StartSampling(extrg,trg1,trg2); 
    delayMicroseconds(200);
    
    if (sftrg){
      cout<<"Triggering"<<endl;
      delayMicroseconds(500); 
      SoftwareTrigger();
    }

    // sit and wait for a trigger to come along 
    while (ReadPin(OEbar) == 1);
    
    // hold DAQHalt high to stop data recording when trigger comes
    // (dead time starts now) ====================================== **
    // TODO: Record dead time 
    digitalWrite(DAQHalt,1);

    // load SRAM data into global integer array, dataBlock
    readSRAMData(); 

    // dump the event data into the writeFile
    WriteSRAMData(eventNum-1,output_fname);
    // (end dead time) ============================================= **
    }
}

int main(){
  
  LoadRunConfiguration("config/digi.config");

  setupPins();
  setupComponents();
  digitalWrite(PSCL,1);

  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  takeData(false, false, true, false, nEvents, false, false);

  return 0;

}
