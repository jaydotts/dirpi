#include "io.cpp"
using namespace std;


// begin data recording 
void takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int max_events, const char* output_folder){

  int nfile = 0; 
  cout<<"nfile:"<<std::to_string(nfile)<<endl;
  std::string output_fname = fname_prefix+std::to_string(nfile)+".txt";
  cout<<"fname: "<<output_fname<<endl;

  while(true){

    if ((eventNum!=0) & (eventNum % max_events == 0)){  
      nfile++;
      output_fname = fname_prefix+std::to_string(nfile)+".txt";
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
        eventNum = nEvents; 
        return;
        }
    };

    digitalWrite(DAQHalt,1);

    readSRAMData(); 

    WriteSRAMData(eventNum-1,(std::string(output_folder)+"/"+output_fname).c_str());

    }
}

int main(int argc, char* argv[]){
  
    printf("Running %s", argv[0]);


    const char * config_fpath;
    const char * output_folder;

    if (argc < 3){
      printf("\nRequired arguments config file path [1] or output folder [2] not found\n");
      printf("Exiting...\n");
      return 0; 
    };

    if (argc >= 3) {
      config_fpath = argv[1]; 
      output_folder = argv[2];
      cout<<"\nUsing config file "<<config_fpath<<endl;
      cout<<"Output folder name: "<<output_folder<<endl;
    }

  LoadRunConfiguration(config_fpath);

  setupPins();
  setupComponents();
  digitalWrite(PSCL,1);

  //takeData(bool extrg, bool sftrg, bool trg1, bool trg2, int nEvents, bool saveData, bool plotData)
  cout<<nEvents<<endl;
  takeData(extrg, sftrg, trg1, trg2, events_per_file, output_folder);

  cout<<"Finishing up..."<<endl;
  return 0;

}