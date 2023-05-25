#include "../include/testing.h"
#include "../include/setup.h"
#include "../include/components.h"

Testing::Testing(int mode_setting){
    mode = mode_setting; 
    cout<<"WARNING: POWER-ON TEST NOT YET IMPLEMENTED"<<endl;
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
    // what is the trigger precision? 
    // is it acceptible? 
    return true; 
}

bool Testing::trig_setup(){
    // is the trigger threshold accurate? 
    // how accurate? 
    // is it accurate across settings? 

    bool is_ok = false; 

    DIGIPOT TestDac1 = DIGIPOT(1,255); 
    DIGIPOT TestDac2 = DIGIPOT(2,255); 

    return true; 
}

bool Testing::clock_setting(){
    // does clock setting with i/o work? 
    return true; 
}

bool Testing::temp_sensor(){
    DIGI_TEMP test_sens = DIGI_TEMP(); 
    
    for(int i=0; i<30; i++){
        cout << test_sens.get_temp() << endl;
        delayMicroseconds(10000); 
    };

    return true; 
}