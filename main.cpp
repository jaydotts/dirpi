#include "io.cpp"
using namespace std;


// begin data recording 
void takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData, const char* output_fname = "outputfile.txt"){

  // extrg - whether or not to enable external triggers 
  // sftrg - whether or not to trigger using software trigger 
  // trg1 and trg2 - enable triggering on either of these channels 

  // enable trigger based on passed parameter 
  // todo: set trigEns low 

  while(eventNum < nEvents){
    cout<<eventNum<<endl;

    // Toggle SLWCLCK high, Reset counters, Deassert DAQHalt
    // to enable sampling mode
    StartSampling(extrg,trg1,trg2); 
    
    if (sftrg){
      cout<<"Triggering"<<endl;
      delayMicroseconds(100); 
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

void CosmicCatcher(bool extrg, bool sftrg, bool trg1, bool trg2, int nFiles, int nEvents){

  for(int i = 0; i<nFiles; i++){
    const char* fname = ("Cosmics_"+std::to_string(i)+".txt").c_str(); 
    takeData(extrg, sftrg, trg1, trg2, nEvents, false, false, fname);
  };
}

int main(){
  
  LoadRunConfiguration("config/digi.config");

  setupPins();
  setupComponents();
  digitalWrite(PSCL,1);

  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  //takeData(true, true, false, false, nEvents, false, false);

  CosmicCatcher(true, true, false, false, nEvents, 10);

  return 0;

}