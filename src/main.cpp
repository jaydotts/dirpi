#include "../include/setup.h"
#include "../include/data.h"
#include "../include/trigger.h"
#include "../include/SRAM.h"
#include "../include/io.h"
#include "../include/setup.h"
#include "../include/testing.h"
#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <ctime>
#include <stdlib.h>
#include <string> 
#include <iostream>
#include <fstream>
using namespace std; 

/* 
PowerOnSelfTest:
Run test of loaded params 
Run test of critical components 
Return OK signal if ok
*/

bool POST(){
    int test_mode = 1; 

    Testing test(test_mode); 

    if ( not test.is_alive() ){
         return false;}
    
    return (
        true
        //test.i2c_setup() &
        //test.trig_setup() & 
        //test.trig_precision() &
        //test.clock_setting() 
        // & test.temp_sensor()
    ); 

}

int main(int argc, char* argv[]){

    if(POST()){

        std::string CONFIG_FILE_PATH = "config/config.ini";
        if(argc > 1){
            cout<<run_num<<endl;
            run_num = stoi(argv[2]);}
        if(argc > 2){
            cout<<max_files<<endl;
            max_files = stoi(argv[3]);}

        std::string output_fname;       // generated by code  

        initialize(CONFIG_FILE_PATH);

        // Initialize objects
        Data RunData(config->memory_depth);  
        Trigger Trg1 = Trigger(1); 
        Trigger Trg2 = Trigger(2); 
        SRAM RAM;

        int file_count = 0; 
        while(!isfile(".stop") & (file_count < 2+max_files)){ 
            if (RunData.eventNum % config->events_perFile == 0 & config->record_data){  
                // redefine file name after incrementing counter

                output_fname = 
                    fname_prefix + "_" + 
                    std::to_string(file_count)+
                    ".txt";

                std::string output_path = std::string(
                    output_folder+"/"+output_fname
                );

                file_count++;
                writeToFile(output_path.c_str(), RunData.PrintConfigs()); 
            };

            // enable SRAM sampling
            RAM.enable_sampling();

            // use run configs to enable/disable triggers
            if(config->trg1){Trg1.enable();}
            if(config->trg2){Trg2.enable();}
            if(config->extrg){enable_extrg();}

            // wait for OE* to go high -> indicates that 
            // trigger runout has gone high 
            if ((ReadPin(OEbar) == 1)){
                while(ReadPin(OEbar)==1);
                delayMicroseconds(1000); // Wait for entire MT cycle
                if (ReadPin(OEbar) == 0){
                    digitalWrite(DAQHalt,1);
                    digitalWrite(DAQHalt,1);
                    digitalWrite(DAQHalt,1);
                    RunData.Read(); 
                    RunData.Write((
                        output_folder+"/"+output_fname).c_str()
                        ); 
                    RunData.eventNum++; 
                    if(! config->record_data){
                        std::cout << '\r'<<"Events: "<< RunData.eventNum << std::flush;
                    }
                }
        }
    }

        std::cout<<"\nTotal Events: "<<RunData.eventNum<<endl;
        free(config);
        std::ofstream stop(".stop"); 
        stop.close();
        return 1; 
    }
}
