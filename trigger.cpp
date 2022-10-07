

void SoftwareTrigger()
{
  digitalWrite(SFTTRG,1);
  //digitalWrite(SFTTRG,1);
  //digitalWrite(SFTTRG,1);
  digitalWrite(SFTTRG,0);
}

void TestTrgThr(int chan) // Test the setting of the trigger threshold
{
  // Vary the trigger threshold so it can be monitored on the scope
  for (int voltage=0; voltage<3301; voltage++) { // Loop over voltage
    cout << "Trigger threshold = "<<voltage<<" mV\n";
    SetTriggerThreshold(chan, voltage, 0);
    delayMicroseconds(10000);
  } // Loop over voltage    
}

int IsTriggered() {
  for (int i=0; i<100; i++)
   if (ReadPin(OEbar) == 1) return 0;
  return 1; 
}

void ToggleSlowClock() // Toggle the slow clock down then up
{
 // Repeat twice to avoid rare errors.
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,1);
 digitalWrite(SLWCLK,0);
 digitalWrite(SLWCLK,0);
}

void TestSLWCLK()
{
  int val;
  for (int i=0; i<5; i++) {
    StartSampling();
    val = ReadPin(OEbar);
    cout << "Started sampling for a new event, OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    cout << "\n";
    val = ReadPin(OEbar);
    cout << "OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    SoftwareTrigger();
    val = ReadPin(OEbar);
    cout << "Trig with SLWCLK high, OEbar="<<val<<"\n"; cout.flush();
    delayMicroseconds(2000000);
    digitalWrite(DAQHalt,1);
  }
}

void ResetCounters() // Reset the address counters
{
 if (debug) {cout << "Reseting the address counters.\n"; cout.flush();}
 // Toggle high then low.
 // Repeat to avoid rare failures.
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,1);
 digitalWrite(MR,0);
 digitalWrite(MR,0);
 digitalWrite(MR,0);
}