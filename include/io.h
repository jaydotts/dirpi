#ifndef IO_H
#define IO_H

/*
Helper functions for quick I/O operations 
*/

void SetMUXCode(int val); 

int ReadPin(int iPin); 

void ToggleSlowClock(); 

void reset_MUX(); 

#endif