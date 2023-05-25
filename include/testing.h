#ifndef TESTING_H
#define TESTING_H

class Testing{
    /*
    Handles power-on self testing of i2c 
    devices and trigger settings

    TEST MODES: 
    [1] Run All Tests 
    [2] Run only i2c tests 
    [3] Run only data collection tests
    */
    
    public:
        int mode; 

        Testing(int mode_setting); 
        bool is_alive(); 
        bool i2c_setup(); 
        bool trig_setup(); 
        bool trig_precision(); 
        bool clock_setting(); 
        bool temp_sensor();
};

#endif