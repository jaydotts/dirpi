#include "../include/setup.h"
#include "../include/data.h"
#include "../include/trigger.h"
#include "../include/SRAM.h"
#include "../include/io.h"
#include "../include/setup.h"
#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <ctime>
using namespace std; 

/* 
PowerOnSelfTest:
Run test of loaded params 
Run test of critical components 
Return OK signal if ok
*/

string getTime(){
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,sizeof(buffer),"%d-%m-%Y_%H:%M:%S",timeinfo);
  std::string str(buffer);
  return str; 
}

bool POST(){
    return true; 
}

int main(int argc, char* argv[]){

    if(POST()){

        const char * CONFIG_FILE_PATH = "config/digi.config";
        
        initialize(CONFIG_FILE_PATH);

        // Initialize objects
        Data RunData;  
        Trigger Trg1 = Trigger(1); 
        Trigger Trg2 = Trigger(2); 
        SRAM RAM; 

        const char * output_folder = "OUTPUTS";

        int max_events = 100; 
        int nfile = 0; 
        cout<<"nfile:"<<std::to_string(nfile)<<endl;
        std::string fname_prefix = getTime(); 
        std::string output_fname = fname_prefix+std::to_string(nfile)+".txt";
        cout<<"fname: "<<output_fname<<endl;
        cout<<"folder: "<<output_folder<<endl;

        if ((RunData.eventNum!=0) & (RunData.eventNum % max_events == 0)){  
            nfile++;
            output_fname = fname_prefix+std::to_string(nfile)+".txt";
        };
        
        while(true){ 
            RAM.enable_sampling();
            enable_extrg(); 
            while(ReadPin(OEbar)==1); 
            digitalWrite(DAQHalt,1);
            digitalWrite(DAQHalt,1);
            digitalWrite(DAQHalt,1);
            RunData.Read(); 
            RunData.Write((std::string(output_folder)+"/"+output_fname).c_str()); 
            RunData.eventNum++; 
            std::cout << '\r'<<RunData.eventNum << std::flush;
        }
    }
}