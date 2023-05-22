#include "../include/unittest.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>

int unit_test(){

    Unittest test; 

    test.test_temp_sensor();

}