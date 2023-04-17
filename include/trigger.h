#ifndef TRIG_H
#define TRIG_H

// Class for channel-specific waveform triggers
class Trigger{
    private: 
        int chan;
    public: 
        Trigger(int channel); 
        void enable(); 
        void disable(); 
};

// Helper functions for global trigger mgmt 

int isTriggered(); 
void enable_extrg(); 
void disable_extrg();
void reset_counters(); 
void disable_triggers(); 
void software_trigger(int nhigh); 
void software_trigger(); 

#endif