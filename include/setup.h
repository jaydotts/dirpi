#include <string>
#ifndef SETUP_H
#define SETUP_H

/* 
Define pin numbers as globals
*/
extern const int MX0;       // OUTPUT. Lowest bit of the multiplexer code.
extern const int MX1;       // OUTPUT. Middle bit of the multiplexer code.
extern const int MX2;       // OUTPUT. Highest bit of the multiplexer code.
extern const int MX1Out;    // INPUT. Data bit out of multiplexer for CH1.
extern const int MX2Out;    // INPUT. Data bit out of multiplexer for CH2.
extern const int SLWCLK;    // OUTPUT. Set high when runtning fast clock, and toggle to slowly clock addresses for readout.
extern const int OEbar;     // INPUT. Goes low when ready for readout.
extern const int DAQHalt;   // OUTPUT. Hold high to stay in readout mode.
extern const int Calib;     // OUTPUT. Pulled high to fire a calibration pulse.
extern const int MR;        // OUTPUT. Pulse high to reset counters before starting a new event.
extern const int Trg1Cnt;   // INPUT. Goes high when the trigger counter for CH1 overflows.
extern const int Trg2Cnt;   // INPUT. Goes high when the trigger counter for CH2 overflows.
extern const int SFTTRG;    // OUTPUT. Pulse high to force a software trigger through TrgExt.
extern const int TrgExtEn;  // OUTPUT. Set high to enable external triggers and software triggers.
extern const int Trg1En;    // OUTPUT. Set high to enable CH1 triggers.
extern const int Trg2En;    // OUTPUT. Set high to enable CH2 triggers.
extern const int PSCL;          // OUTPUT (PWM). Used to prescale the number of CH1 OR CH2 triggers accepted.

/* 
Define parameters to be loaded 
from the config file. Initialize 
with defaults in case of a read 
problem. 
*/

extern const char* CONFIG_FILE_PATH; 

extern bool trg1; 
extern bool trg2; 
extern bool sftrg;
extern bool extrg;  

extern int PotValCh1;
extern int PotValCh2;
extern int DACValCh1; 
extern int DACValCh2;
extern int clckspeed; 
extern int PSCLduty;
extern int events_perFile; 
extern const int address_depth; 
extern bool record_data; 
extern std::string fname_prefix; 
extern std::string output_folder; 
extern int run_num; 

void setupPins();

bool setupComponents();

void load_configs(const char * CONFIG_FILE_PATH); 

bool setupFiles(int run_num); 

bool initialize(const char * CONFIG_FILE_PATH);

#endif