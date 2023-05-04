# dirpi
## *Di*gitizer for *R*aspberry *Pi*

Driver software package for [two-channel waveform digitizer for x-ray detector](http://dstuart.physics.ucsb.edu/Lgbk/pub/E41214.dir/E41214.html). This software is designed to be installed on Raspberry Pi 3. 

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
Software requirements can be found in ***requirements.txt***. 

## Usage  
The digitizer driver software is split into (?) translation units: 
- ***components.h***: class definitions for all board components
- ***components.cpp***: implementations of all board components 
- ***io.cpp***: methods for fast read/write of component data
- ***test.cpp***: self-test routines 

## Support

## Contributing
