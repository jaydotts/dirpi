# dirpi
## *Di*gitizer for *R*aspberry *Pi*

Driver software package for [two-channel waveform digitizer for x-ray detector](http://dstuart.physics.ucsb.edu/Lgbk/pub/E41214.dir/E41214.html). This software is designed to be installed on Raspberry Pi 3. 

## Table of Contents

- [Start](#start)
- [Installation](#installation)
- [Usage](#usage)
- [Support](#support)
- [Contributing](#contributing)

## Installation
Software requirements can be found in ***requirements.txt***. 
This software package also depends on the WiringPi library, which is depracated. The necessary packages are included in the repository's 'build' folder. 

To install, simply navigate to the dirpi directory where the code will be run and run 

```
./build.sh
```

## Usage  
To start a fresh data acquisition, use the command 
```
./dirpi.sh start <run_number>
```

If provided, the *run_number* argument will be used to generate an output directory called Run<run_number>. If not supplied, data will not be saved. The user interface will still work in this mode. 

To stop a run, use the command 
```
./dirpi.sh stop
```
which will stop all processes other than the gui, which is stopped manually by the user. 

## Support

## Contributing
