#include "../include/setup.h"
#include "../include/trigger.h"
#include "../include/io.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>


Trigger::Trigger(int channel){ 
    chan = channel;
}

void Trigger::enable(){
    switch(chan){
        case 1: 
            digitalWrite(Trg1En, 1);
            digitalWrite(Trg1En, 1);
            digitalWrite(Trg1En, 1);
            break;
        case 2: 
            digitalWrite(Trg2En, 1);
            digitalWrite(Trg2En, 1);
            digitalWrite(Trg2En, 1);
            break;
    }
}

void Trigger::disable(){
    switch(chan){
        case 1: 
            digitalWrite(Trg1En, 0);
            digitalWrite(Trg1En, 0);
            digitalWrite(Trg1En, 0);
            break;
        case 2: 
            digitalWrite(Trg2En, 0);
            digitalWrite(Trg2En, 0);
            digitalWrite(Trg2En, 0);
            break;
    }
}

void enable_extrg() {
    digitalWrite(TrgExtEn, 1);
    digitalWrite(TrgExtEn, 1);
    digitalWrite(TrgExtEn, 1);
}

void disable_extrg() {
    digitalWrite(TrgExtEn, 0);
    digitalWrite(TrgExtEn, 0);
    digitalWrite(TrgExtEn, 0);
}

void reset_counters() {
    /*
    Toggle MR high then low to reset 
    trigger counters
    */
    digitalWrite(MR,1);
    digitalWrite(MR,1);
    digitalWrite(MR,1);
    digitalWrite(MR,0);
    digitalWrite(MR,0);
    digitalWrite(MR,0);
}

int IsTriggered() {
  for (int i=0; i<100; i++)
   if (ReadPin(OEbar) == 1) return 0;
  return 1; 
}

void disable_triggers(){ // disables all triggers
    digitalWrite(Trg1En, 0);
    digitalWrite(Trg1En, 0);
    digitalWrite(Trg1En, 0);
    digitalWrite(Trg2En, 0);
    digitalWrite(Trg2En, 0);
    digitalWrite(Trg2En, 0);
    disable_extrg(); 
}

void software_trigger(int nhigh)
{
  for(int i=0; i<nhigh; i++) 
   digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

void software_trigger() {
  software_trigger(3);
  delayMicroseconds(1); 
}