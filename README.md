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

## Usage
### Initialization
### Triggering 
### Data Collection 
### Data Readout

## Support

## Contributing
