#include "io.cpp"
using namespace std;


// begin data recording 
void takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, int max_events){

  int nfile = 0; 
  const char* output_fname = (fname_prefix+std::to_string(nfile)+".txt").c_str();

  while(eventNum < nEvents){

    if ((eventNum!=0) & (eventNum % max_events == 0)){  
      nfile++;
      output_fname  = (fname_prefix+std::to_string(nfile)+".txt").c_str(); 
    };

    cout<<eventNum<<endl;

    // Toggle SLWCLCK high, Reset counters, Deassert DAQHalt
    // to enable sampling mode
    StartSampling(extrg,trg1,trg2); 
    
    if (sftrg){
      cout<<"Triggering"<<endl;
      delayMicroseconds(100); 
      SoftwareTrigger();
    }

    // start timer - ensure trigger does not time out
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
    };

    digitalWrite(DAQHalt,1);

    readSRAMData(); 

    WriteSRAMData(eventNum-1,output_fname);

    }
}

int main(){
  
  LoadRunConfiguration("config/digi.config");

  setupPins();
  setupComponents();
  digitalWrite(PSCL,1);

  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  cout<<nEvents<<endl;
  takeData(extrg, sftrg, trg1, trg2, nEvents, events_per_file);

  cout<<"Finishing up..."<<endl;
  return 0;

}