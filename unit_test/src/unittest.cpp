#include "../../include/setup.h"
#include "../../include/components.h"
#include "../../include/io.h"
#include "../../include/INIReader.h"
#include "../include/unittest.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

int Unittest::test_config_loader(){
    load_configs("../../config/config.ini"); 
}

int Unittest::test_temp_sensor(){
    wiringPiSetup();
    setupPins();

    DIGI_TEMP test_sens = DIGI_TEMP(); 

    test_sens.get_temp();
}

