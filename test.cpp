

void TestTrgThr(int chan) // Test the setting of the trigger threshold
{
  // Vary the trigger threshold so it can be monitored on the scope
  for (int voltage=0; voltage<3301; voltage++) { // Loop over voltage
    cout << "Trigger threshold = "<<voltage<<" mV\n";
    SetTriggerThreshold(chan, voltage, 0);
    delayMicroseconds(10000);
  } // Loop over voltage    
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

void CheckForSRAMErrors(int nEvent)
{  
  // Repeatedly read the same event and check for discrepancies
  if (nEvent<=0) return;
  cout << "Checking for SRAM errors...\n";
  for(int i=0; i<2000; i++) {
    cout << "\rEvent "<<i<<" ";cout.flush();
    StartSampling();
    delayMicroseconds(20);
    SoftwareTrigger();
    while (ReadPin(OEbar)==0) {
      delayMicroseconds(2);
    }
    digitalWrite(DAQHalt,1);
    ReadAllData();
    ReReadAllData(); // Check for errors
  }
}