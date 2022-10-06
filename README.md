# ucsb-digitizer

Software package for control and data collection/processing from [Two-channel waveform digitizer for x-ray detector](http://dstuart.physics.ucsb.edu/Lgbk/pub/E41214.dir/E41214.html). This software is designed to be installed on Raspberry Pi 3.  

## Table of Contents

- [Start](#start)
- [Installation](#installation)
- [Usage](#usage)
- [Support](#support)
- [Contributing](#contributing)

## Start 
digi.cpp uses the wiringPi library for fast GPIO access. To compile, open terminal to directory with digi.cpp and run
```
g++ -O digi.cpp -o digi -lwiringPi
```


## Installation
digi.cpp is currently the only script that manages the data collection from digitizer to Raspberry Pi. The dependencies are: 
- wiringPi library 
- Adafruit_ADS1015 library 
- PCA9554.hpp 

## Usage
### Initialization

***void SetBias(int chan, int voltagedc, int persist)*** 
- Sets the bias voltage of the DAC in volts. persist=1 writes to flash.
- This bias voltage acts as the trigger threshold. 
- Triggering occurs when both channels (chan==1, chan==2) are enabled and go above threshold
- Function communicates with one of the two DACs (set by chan) 
- voltagedc sets the bias in volts. Min=0, Max=255


### Data Collection  
***void TakeData(int nEvents, bool isRandom = false)***
- Checks every 2 uS for samples until number of events == nEvents
- For each event: 
  - Write DAQHalt address to 1, which holds TrgHold high until memory can be written 
  - Calls TakeData() which calls WriteData -> opens the data file and writes to memory one data block at a time 
  - Does this for each nEvents number of bits 
  - Closes the file
  
  
### Data Collection 
### Data Readout

## Support

## Contributing
