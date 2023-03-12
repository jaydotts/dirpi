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

    cout<<timeout<<endl;
    float max_time = float(timeout*60); 

    auto start = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = time-start;

    // sit and wait for a trigger to come along 
    while (ReadPin(OEbar) == 1){

      time = std::chrono::system_clock::now();
      elapsed_seconds = time-start;

      if (elapsed_seconds.count() > max_time){
        cout<<"Trigger timed out"<<endl;
        return;
        }
    }

    digitalWrite(DAQHalt,1);

    readSRAMData(); 

    WriteSRAMData(eventNum-1,output_fname);

    }
}

void CosmicCatcher(bool extrg, bool sftrg, bool trg1, bool trg2, int nFiles, int nEvents){

  for(int i = 0; i<nFiles; i++){
    const char* fname = (fname_prefix+std::to_string(i)+".txt").c_str(); 
    takeData(extrg, sftrg, trg1, trg2, nEvents, false, false, fname);
  };
}

int main(){
  
  LoadRunConfiguration("config/digi.config");

  setupPins();
  setupComponents();
  digitalWrite(PSCL,1);

  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  //takeData(extrg, sftrg, trg1, trg2, nEvents, false, false);

  CosmicCatcher(extrg, sftrg, trg1, trg2, nEvents, 10);

  cout<<"Finishing up..."<<endl;
  return 0;

}