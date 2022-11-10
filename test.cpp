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

bool testDigiPot = true; 

int main(){

if(testDigiPot){
    DIGIPOT digi = DIGIPOT();
    digi.setInputBias(2,200);
}

return 0;
}
