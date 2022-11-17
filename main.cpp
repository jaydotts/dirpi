
#include "io.cpp"
using namespace std;


// begin data recording 
void takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData){

  // extrg - whether or not to enable external triggers 
  // sftrg - whether or not to trigger using software trigger 
  // trg1 and trg2 - enable triggering on either of these channels 

  while(int i < nEvents){

    // reset address counters 
    ResetCounters();

    // enable sampling by setting WE* high and OE* low
    digitalWrite(DAQHalt,0);
    digitalWrite(DAQHalt,0);
    digitalWrite(DAQHalt,1);

    // reset all MX bits back to 0
    digitalWrite(MX0,0);
    digitalWrite(MX1,0);
    digitalWrite(MX2,0);

    // enable trigger based on passed parameter 
    if (extrg) digitalWrite(TrgExtEn, 1); 
    else 
      if (trg1) digitalWrite(Trg1En, 1);
      if (trg2) digitalWrite(Trg2En, 1);
    
    StartSampling(); 
    
    if (sftrg)
      delayMicroseconds(500); 
      SoftwareTrigger();
    
    // sit and wait for a trigger to come along 
    while (IsTriggered()==0){ 
      delayMicroseconds(2);
    }
    
    // hold DAQHalt high to stop data recording when trigger comes
    // (dead time starts now) ====================================== **
    // TODO: Record dead time 
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);
    digitalWrite(DAQHalt,1);

    // load SRAM data into global integer array, dataBlock
    readSRAMData(); 

    // DEBUG: print out the data (remove in prod
    for (int i=0; i<addressDepth; i++) {
        //if ((i+3)%10==0) cout <<i<<" "<<dataBlock[0][i] << " " << dataBlock[1][i]<<"\n";
        cout <<i<<" "<<dataBlock[0][i] << " " << dataBlock[1][i]<<"\n";
        //if (dataBlock[0][i] > 45) cout<<i<<" "<<dataBlock[0][i]<<" "<<dataBlock[1][i]<<"\n";
    }

    // dump the event data into the writeFile

    i++;
    
    }
}

int main(){

  setupPins(); 

  digitalWrite(DAQHalt,0);
  digitalWrite(SFTTRG,0);
  
  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  takeData(false, true, true, true, 100, false, false);

  return 0;

}
