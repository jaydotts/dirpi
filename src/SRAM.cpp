#include "../include/SRAM.h"
#include "../include/io.h"
#include "../include/trigger.h"
#include "../include/setup.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>

void SRAM::enable_sampling(){

    /*
    Toggle slowclock once, then set 
    high for sampling 
    */
    ToggleSlowClock(); 
    digitalWrite(SLWCLK,1);
    digitalWrite(SLWCLK,1);

    disable_triggers();
    reset_counters();
    reset_MUX(); 

    // Write DAQHalt low to enable sampling
    digitalWrite(DAQHalt,0);
    digitalWrite(DAQHalt,0);
    digitalWrite(DAQHalt,0);

    delayMicroseconds(210);
    
}