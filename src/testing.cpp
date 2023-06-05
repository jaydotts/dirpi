#include "../include/testing.h"
#include "../include/setup.h"
#include "../include/components.h"
#include "../include/io.h"
#include <chrono> 
Testing::Testing(int mode_setting){
    mode = mode_setting; 
}

bool Testing::is_alive(){
    // is the dirpi plugged in? 
    return true; 
}

bool Testing::i2c_setup(){
    // can i2c be setup successfully? 
    return true; 
}

bool Testing::trig_precision(){ 
    /* First, set up trigger */

    //Timer timer = Timer(); 

    /* Initialize*/
    DIGIPOT CH1_PEDESTAL = DIGIPOT(1,255); 
    DIGIPOT CH2_PEDESTAL = DIGIPOT(2,255); 
    ThrDAC TestDac1 = ThrDAC(1,0,0); 
    ThrDAC TestDac2 = ThrDAC(2,0,0); 
    cout<<"Testing trigger point"<<endl;
    delayMicroseconds(1000000); 
    return true; 
}

bool Testing::trig_setup(){
    // is the trigger threshold accurate? 
    // how accurate? 
    // is it accurate across settings? 

    bool is_ok = false; 

    return true; 
}

bool Testing::clock_setting(){
    // does clock setting with i/o work? 
    return true; 
}

bool Testing::temp_sensor(){
    DIGI_TEMP test_sens = DIGI_TEMP(); 
    
    int errs = 0; 
    for(int i=0; i<30; i++){
        float temp = test_sens.get_temp(); 
        if (temp < 0){
            errs++; 
        }
    };
    return bool(!errs); 
}
