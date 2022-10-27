// quick tests 

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <iomanip>
#include <sys/time.h>
#include <vector>

#include <chrono>
#include "components.cpp"
using namespace std;


int main(){

DIGIPOT digi = DIGIPOT();
//digi.SetVoltage(1,200);
digi.SetVoltage(2,200);
return 0;
}
