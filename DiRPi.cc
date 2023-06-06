// DiRPi.cc  David Stuart, April 2023
// This program handles pulse finding and compressed data output for DiRPi waveforms.
 
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <iomanip>  // Necessary for "setprecision"

using namespace std;

bool debug=false;
int lossycompress = 0; // Set to 1 if we want to allow lossy compression

unsigned long long int toffset = (unsigned long long int)1660000000000; // Offset to unix time (in ms) when saving

// Parameters for pulse finding
int ScaleFactor[2]; // Integer scale factor to apply to each channel, e.g., to negate
int Threshold[2]; // Threshold for start and end of pulse, in ADC counts
int nQuiet[2]; // Number of quiet pre-samples required
int nMinWidth[2];
int iStartSearch = 5; // Number of initial samples to skip before looking for pulses

// A class that handles everything about one event
class Event {
public: 
  unsigned long long int time; // Unix event time in milliseconds
  int dRunTime; // Time since start of the run in milliseconds
  int dEvtTime; // Time since previous event in milliseconds
  int rawD[2][65536]; // Raw data from both channels
  int D[2][65536]; // Corrected data from both channels
  int evtNum;
  int runNum;
  int TrgCnt[2]; // Number of trigger primitives counted
  int nSamples; // Number of samples, either 4096 or 65536.
  int dMin[2],dMax[2]; // Min and max data value (aka ADC)
  int preMeanX10[2], preRMSX10[2]; // Mean and RMS (x10) calculated from first samples
  int mostProb[2]; // Most probable value within waveform
  // Information for all pulses in the event
  int pulsenum; // Pulse number within the event across all channels
  int nPulses[2]; // Number of pulses in each channel
  vector<int> Pt; // Time of pulse, in address units
  vector<int> Phalft; // Time of pulse half-threshold transit, in 16*address units
  vector<int> Pch; // channel #
  vector<int> Pipulse; // pulse # within this channel; first=0
  vector<int> Pwidth; // Width, in address units
  vector<int> PpeakV; // pulse height, in ADC units
  vector<int> Parea; // pulse area, in ADC*Address units
  Event() {Clear();}
  ~Event() {Clear();}
  void ClearPulses(); // Erase any existing pulses
  void Clear();
  void CopyRaw(); // Copy from rawD to D
  void CalcRange(); // Calculate voltage range
  int CalcMostProbable(int chan, int t1, int t2, int *mostProb); // returns most probable value within passed time range. Returns # of samples in most probable
  int CalcAverage(int chan, int t1, int t2, int nIter, int *meanX10, int *rmsX10); // returns # of samples used
  void SubtractAverage(); // Calculate and subtract the nominal average (and RMS); save in preMeanX10 and preRMSX10
  void FindPulses(); // Find all pulses in the event
  void WritePulses(); // Write (append) pulses to the appropriate filename
  void DumpPulses(); // Write pulses to stdout
  void Dump(); // Dump info about the event to the screen
  void Correct(); // Apply corrections to the event
  vector<unsigned char> ChanBuf[2]; // Compressed data for each channel
  void Compress(); // Compress the waveform data for writing
  void LossyCompress(); // Compress the waveform data allowing 1 ADC count precision loss
  void UnCompress(); // Uncompress the waveform data from buffers read from data file
  void TestCompress(); // Test that we can compress and uncompress without problems
  void Display(int minT, int maxT); // Make an event display for the event
};

void Event::ClearPulses() {
 pulsenum = 0;
 for (int i=0; i<2; i++)
  nPulses[i] = 0;
 while (!Pt.empty()) Pt.pop_back();
 while (!Phalft.empty()) Phalft.pop_back();
 while (!Pch.empty()) Pch.pop_back();
 while (!Pipulse.empty()) Pipulse.pop_back();
 while (!Pwidth.empty()) Pwidth.pop_back();
 while (!PpeakV.empty()) PpeakV.pop_back();
 while (!Parea.empty()) Parea.pop_back();
}

void Event::Clear() {
  evtNum = -1;
  nSamples = 0;
  time = 0;
  TrgCnt[0] = 0;
  TrgCnt[1] = 0;
  ClearPulses();
} 

void Event::CopyRaw() {
 // Copy from rawD to D. This allows corrections to D without changing rawD.
 for (int c=0; c<2; c++)
  for (int i=0; i<nSamples; i++)
    D[c][i] = rawD[c][i];
}

void Event::CalcRange() {
 for (int c=0; c<2; c++) {
  dMin[c] = 999; dMax[c] = -999;
  for (int i=0; i<nSamples; i++) {
    if (D[c][i]<dMin[c]) dMin[c] = D[c][i];
    if (D[c][i]>dMax[c]) dMax[c] = D[c][i];
  }
 }
}

int Event::CalcAverage(int c, int t1, int t2, int nIter, int *meanX10, int *rmsX10) {
 // Calculate the average and RMS for samples in the range t1 to t2.
 // Do an iterative calculation, removing any samples that fail for the following reasons:
 //   Differ from initial mean value by more than 3 times RMS
 //   Differ from either neighboring sample by more than 2 times RMS
 // We iterate repeatedly with an improved guess at the mean and rms each time
 // Return value is the number of samples used in the final calculation.
 int _meanX10 = *meanX10;
 int _rmsX10 = *rmsX10*2; // Set an initial guess for first iteration
 if (_rmsX10 < 20) _rmsX10 = 20; // Protect against initial zero value
 int MaxIter = nIter;
 int nUsed = 0;
 if (t1<=0) t1=1; // Protect ranges; need to check neighbors
 if (t2>=nSamples-1) t2=nSamples-2; // Need to check neighbors
 if (t2<t1) return 0; // Protection.
 for (int iter = 0; iter < MaxIter; iter++) { // Iteration
    // Calculate the window
    int minWindow = (_meanX10-3*_rmsX10)/10;
    int maxWindow = (_meanX10+3*_rmsX10)/10;
    int maxdV = 2*_rmsX10/10;
    int sumV = 0.;
    int sum2V = 0.;
    nUsed = 0;
    float filterval = 0.002;
    bool good = false; // Shows if a particular sample can be used
    for (int i=t1; i<t2; i++) {
      good = true;
      if (D[c][i] < minWindow) good = false;
      if (D[c][i] > maxWindow) good = false;
      if (fabs(D[c][i]-D[c][i+1])>maxdV) good = false;
      if (fabs(D[c][i]-D[c][i-1])>maxdV) good = false;
      if (good) {
        sumV += D[c][i];
        sum2V += D[c][i]*D[c][i];
        nUsed++;
      }
      good = true;
    }
    if (nUsed > 10) { // Enough points
      float _mean = sumV/(nUsed);
      float _rms = sqrt(sum2V/(nUsed) - _mean*_mean);
      _meanX10 = int(10*_mean);
      _rmsX10 = int(10*_rms);
    } // Enough points
    else
      _rmsX10 *= 2; // Try again with a broader window
  } // iteration
  *meanX10 = _meanX10;
  *rmsX10 = _rmsX10;
  return nUsed;
}

void Event::SubtractAverage() {
 // Calculate mean and RMS from first few samples, then subtract
 int meanPre = (D[0][0]+D[0][1])/2;
 int rmsPre = 10;
 int nUsed;
 nUsed = CalcAverage(0, 1, 81, 4, &meanPre, &rmsPre);
 //preMeanX10[0] = meanPre;
 //preRMSX10[0] = rmsPre;
 int offset = meanPre/10;
 if (nUsed > 20)
  for (int i=0; i<nSamples; i++)
    D[0][i] -= offset;
 meanPre = (D[1][0]+D[1][1])/2;
 rmsPre = 10;
 CalcAverage(1, 1, 81, 4, &meanPre, &rmsPre);
 //preMeanX10[1] = meanPre;
 //preRMSX10[1] = rmsPre;
 offset = meanPre/10;
 if (nUsed > 20)
  for (int i=0; i<nSamples; i++)
    D[1][i] -= offset;
}

int Event::CalcMostProbable(int chan, int t1, int t2, int *mostProb) {
  // Calculate the most probable ADC value for specified channel in passed time range. 
  // This is done by counting how often we have each value, from -1024 to 1023,
  // where the larger range allows future expansion
  int nCounts[2048];
  for (int i=0; i<2048; i++) {
    nCounts[i] = 0;
  }
  for (int i=t1; i<=t2; i++) {
    int iV = int(D[chan][i])+1023;
    if (iV>=0 && iV<2048) {
      nCounts[iV]++;
    }
  }
  int imaxV = 0;
  int maxV = -99999;
  for (int i=0; i<2048;i++) {
    if (nCounts[i]>maxV) {
      maxV = nCounts[i];
      imaxV = i;
    }
  }
  *mostProb = imaxV - 1023; // Make it signed
  return maxV;
} // CalcMostProbable

void Event::Correct() {
  // Apply corrections, including scaling and smoothing
  CopyRaw(); // First copy in the raw data to be corrected
  for (int i=0; i<nSamples; i++) { 
    D[0][i] *= ScaleFactor[0]; // Now scale
    D[1][i] *= ScaleFactor[1];
  }

  // Calculate the mean and RMS. Don't subtract them yet.
  int meanPre = (D[0][0]+D[0][1])/2;
  int rmsPre = 10;
  int nUsed;
  nUsed = CalcAverage(0, 1, 81, 4, &meanPre, &rmsPre);
  preMeanX10[0] = meanPre;
  preRMSX10[0] = rmsPre;
  meanPre = (D[1][0]+D[1][1])/2;
  rmsPre = 10;
  CalcAverage(1, 1, 81, 4, &meanPre, &rmsPre);
  preMeanX10[1] = meanPre;
  preRMSX10[1] = rmsPre;

  // Calculate the most probable value for the full waveform
  int _mostProb;
  int nMostProb;
  nMostProb = CalcMostProbable(0, 0, nSamples-1, &_mostProb);
  mostProb[0] = _mostProb;
  if (nMostProb>20)
    for (int i=0; i<nSamples; i++)
      D[0][i] -= _mostProb;
  nMostProb = CalcMostProbable(1, 0, nSamples-1, &_mostProb);
  mostProb[1] = _mostProb;
  if (nMostProb>20)
    for (int i=0; i<nSamples; i++)
      D[1][i] -= _mostProb;
  // Calculate the mean from the initial samples (and calculate RMS)
  SubtractAverage();
} // Event::Correct

void Event::FindPulses() {
  // Find all pulses in the event
  for (int chan = 0; chan<2; chan++) { // Channel loop
    float localThr = Threshold[chan];
    int maxN = nSamples-nMinWidth[chan]-nQuiet[chan];
    for (int i=iStartSearch; i<maxN; i++) { // loop through the waveform
      if (D[chan][i]>=localThr) { // Above threshold?
        // Check if previous samples are below threshold
        bool isQuiet = true;
        for (int j=1; j<=nQuiet[chan]; j++)
          if (D[chan][i-j]>=localThr)
            isQuiet = false;
        if (isQuiet) { // Passes the presample quiet requirement?
          // Require at least nMinWidth subsequent samples above threshold
          bool goodTrig = true;
          for (int j=i+1; j<i+1+nMinWidth[chan] && j<nSamples; j++)
            if (D[chan][j]<localThr)
              goodTrig = false;
          if (goodTrig) { // Passes minWidth requirement?
            // Find the end of the pulse, as the time where nQuiet samples are below threshold in a row.
            int nBelow = 0;
            bool done = false;
            int j=i;
            while (!done && j<nSamples-1) { // Looking for end of pulse
              j++;
              if (D[chan][j]<localThr)
                nBelow++;
              else
                nBelow = 0;
              if (nBelow >= nQuiet[chan])
                done = true;
            } // Looking for end of pulse
            int endIndex = j;
            float width = (endIndex-nQuiet[chan]-i); // width in address space
            // Find the end of the pulse by looping forward until it falls below threshold.
            pulsenum++;
            // Measure peak and area. Include nQuiet pre/postsamples for area calculation.
            int peakV = 0;
            int peakT = 0;
            int ipeak = i;
            int area = 0;
            for (int k=i-nQuiet[chan]; k<=endIndex; k++) {
              area += D[chan][k];
              if (D[chan][k]>peakV){ 
                peakV = D[chan][k];
                peakT = k;
                ipeak = k;
              }
            }
            // Save pulse
            nPulses[chan]++;
            Pch.push_back(chan);
            Pipulse.push_back(pulsenum-1);
            Pt.push_back(i);
            Phalft.push_back(i*16); // Temporary until we implement calculation
            Pwidth.push_back(width);
            PpeakV.push_back(peakV);
            Parea.push_back(area);
          } // Passes minWidth requirement
        } // Passes the presample quiet requirement?
      } // Above threshold? 
    } // loop through the waveform
  } // Channel loop
} // Event::FindPulses

void Event::WritePulses() {
  // Write (append) pulses to the appropriate file name
  // The file is Run${runNum}_pulses.ant and Run${runNum}_keV.ant
  FILE* outF;
  char fname[100];
  std::string fName;
  sprintf(fname,"Run%d_pulses.ant",runNum);
  fName = fname;
  if (evtNum==0)
    outF = fopen(fName.c_str(),"w");
  else
    outF = fopen(fName.c_str(),"a");
  for (int i=0; i<Pwidth.size(); i++) { 
    fprintf(outF,"%d ",evtNum);
    fprintf(outF,"%4f ",dRunTime/1000.);
    fprintf(outF,"%4f ",dEvtTime/1000.);
    fprintf(outF,"%d ",int(Pwidth.size()));
    fprintf(outF,"%d ",nPulses[0]);
    fprintf(outF,"%d ",nPulses[1]);
    fprintf(outF,"%d ",0);
    fprintf(outF,"%d ",0);
    fprintf(outF,"%.4f ",preRMSX10[Pch[i]]/10.); // 9=RMS of this channel in sideband
    fprintf(outF,"%d ",Pch[i]+1); // 10=channel number
    fprintf(outF,"%d ",Pipulse[i]); // 11=pulsenum within event for this channel
    fprintf(outF,"%d ",Pt[i]); // 12=start time of pulse within waveform in ns
    fprintf(outF,"%d ",Parea[i]); // 13=pulse area in nVs
    fprintf(outF,"%d ",PpeakV[i]); // 14=pulse peak height in mV
    fprintf(outF,"%d ",Pwidth[i]); // 15=pulse width in ns
    fprintf(outF,"%d ",Parea[i]); // 16=pulse area in SPE; not currently calibrated
    float keV = Parea[i]; // Not currently calibrated
    fprintf(outF,"%.4f ",keV); // 17=pulse area in keV
    fprintf(outF,"\n");
  }
  fclose(outF);
  // Next we dump the "keV" formatting of pulse information.
  // It contains the total area per event rather than individual pulse information
  sprintf(fname,"Run%d_keV.ant",runNum);
  fName = fname;
  if (evtNum==0)
    outF = fopen(fName.c_str(),"w");
  else
    outF = fopen(fName.c_str(),"a");
  float keVSum[2];
  for (int c=0; c<2; c++) {
    keVSum[c] = 0.;
    for (int i=0; i<Pwidth.size(); i++) 
     if (Pch[i] == c) {
      keVSum[c] += Parea[i];
     }
  }
  fprintf(outF,"%d ",runNum); // 1=run number
  fprintf(outF,"%d ",evtNum); // 2=event number
  fprintf(outF,"%.4f ",keVSum[0]); // 3=Total energy in channel 1
  fprintf(outF,"%.4f ",keVSum[1]); // 4=Total energy in channel 2
  fprintf(outF,"0 "); // 5=Total energy in channel 3
  fprintf(outF,"0 "); // 6=Total energy in channel 4
  fprintf(outF,"%.4f ",preRMSX10[0]/10.); // 7=RMS in sideband for channel 1
  fprintf(outF,"%.4f ",preRMSX10[1]/10.); // 8=RMS in sideband for channel 2
  fprintf(outF,"0 "); // 9=RMS in sideband for channel 3
  fprintf(outF,"0 "); // 10=RMS in sideband for channel 4
  fprintf(outF,"\n");
  fclose(outF);
} // Event::WritePulses

void Event::DumpPulses() {
  // Write (append) pulses to the appropriate file name
  // The file is Run${runNum}_pulses.ant and Run${runNum}_keV.ant
  for (int i=0; i<Pwidth.size(); i++) { 
    cout << "PULSEANT:";
    cout << " " << evtNum;  // 1=event number
    cout << " " << dRunTime/1000.;  // 2=time in seconds since start of run
    cout << " " << dEvtTime/1000.;  // 3=time in seconds since previous event
    cout << " " << Pwidth.size(); // 4=#pulses in the event
    cout << " " << nPulses[0];    // 5=# of pulses in ch0
    cout << " " << nPulses[1];    // 6=# of pulses in ch1
    cout << " " << 0;       // 7=# of pulses in ch2
    cout << " " << 0;       // 8=# of pulses in ch3
    cout << " " << preRMSX10[Pch[i]]/10.; // 9=RMS of this channel in sideband
    cout << " " << Pch[i];        // 10=channel number
    cout << " " << Pipulse[i];    // 11=pulsenum within event for this channel
    cout << " " << Pt[i];     // 12=start time of pulse within waveform in ns
    cout << " " << Parea[i];  // 13=pulse area in nVs
    cout << " " << PpeakV[i]; // 14=pulse peak height in mV
    cout << " " << Pwidth[i]; // 15=pulse width in ns
    cout << " " << Parea[i];  // 16=pulse area in SPE; not currently calibrated
    float keV = Parea[i]; // Not currently calibrated
    cout << " " << keV;  // 17=pulse area in keV
    cout << "\n";
  }
  // Next we dump the "keV" formatting of pulse information.
  // It contains the total area per event rather than individual pulse information
  float keVSum[2];
  cout << setiosflags(ios::fixed) << setprecision(3);
  for (int c=0; c<2; c++) {
    keVSum[c] = 0.;
    for (int i=0; i<Pwidth.size(); i++) 
     if (Pch[i] == c) {
      keVSum[c] += Parea[i];
     }
  }
  cout << setiosflags(ios::fixed) << setprecision(5);
  cout << "KEVANT:";
  cout << " " << runNum; // 1=run number
  cout << " " << evtNum; // 2=event number
  cout << " " << keVSum[0]; // 3=Total energy in channel 1
  cout << " " << keVSum[1]; // 4=Total energy in channel 2
  cout << " " << 0; // 5=Total energy in channel 3
  cout << " " << 0; // 6=Total energy in channel 4
  cout << " " << preRMSX10[0]/10.; // 7=RMS in sideband for channel 1
  cout << " " << preRMSX10[1]/10.; // 8=RMS in sideband for channel 2
  cout << " " << 0; // 9=RMS in sideband for channel 3
  cout << " " << 0; // 10=RMS in sideband for channel 4
  cout << "\n";
} // Event::DumpPulses()

void Event::Dump() {
  // Dump information about the event to the screen
  cout << "Dump of Run,Event "<<runNum<<","<<evtNum<<"\n";
  cout << "time "<<runNum<< " "<<evtNum<<" "<<time<<"\n";
  cout << "dRunTime "<<runNum<< " "<<evtNum<<" "<<dRunTime<<"\n";
  cout << "dEvtTime "<<runNum<< " "<<evtNum<<" "<<dEvtTime<<"\n";
  cout << "nSamples "<<runNum<< " "<<evtNum<<" "<<nSamples<<"\n";
  cout << "min0 "<<runNum<< " "<<evtNum<<" "<<dMin[0]<<"\n";
  cout << "min1 "<<runNum<< " "<<evtNum<<" "<<dMin[1]<<"\n";
  cout << "max0 "<<runNum<< " "<<evtNum<<" "<<dMax[0]<<"\n";
  cout << "max1 "<<runNum<< " "<<evtNum<<" "<<dMax[1]<<"\n";
  cout << "preMean0 "<<runNum<< " "<<evtNum<<" "<<preMeanX10[0]<<"\n";
  cout << "preMean1 "<<runNum<< " "<<evtNum<<" "<<preMeanX10[1]<<"\n";
  cout << "preRMS0 "<<runNum<< " "<<evtNum<<" "<<preRMSX10[0]<<"\n";
  cout << "preRMS1 "<<runNum<< " "<<evtNum<<" "<<preRMSX10[1]<<"\n";
  cout << "mostProb0 "<<runNum<< " "<<evtNum<<" "<<mostProb[0]<<"\n";
  cout << "mostProb1 "<<runNum<< " "<<evtNum<<" "<<mostProb[1]<<"\n";
  cout << "TrgCnt0 "<<runNum<< " "<<evtNum<<" "<<TrgCnt[0]<<"\n";
  cout << "TrgCnt1 "<<runNum<< " "<<evtNum<<" "<<TrgCnt[1]<<"\n";
  cout << "pulsenum "<<runNum<< " "<<evtNum<<" "<<pulsenum<<"\n";
  cout << "nPulses0 "<<runNum<< " "<<evtNum<<" "<<nPulses[0]<<"\n";
  cout << "nPulses1 "<<runNum<< " "<<evtNum<<" "<<nPulses[1]<<"\n";
  cout << "PulsesSize "<<runNum<< " "<<evtNum<<" "<<Pt.size()<<"\n";
} // Event::Dump

void Event::LossyCompress() {
  // Compress the data by rounding off ADC values.
  // Only adjust channels that are at most 1 count away from their two neighbors
  // Require the two neighbor values to be equal to each other
  // Require the neighbors to be at most 2 counts from the most probable value
  for (int c=0; c<2; c++) { // Channel loop
    for (int i=2; i<nSamples-2; i++) { // Loop through channel's waveform, with enough space on edges
      if ((abs(rawD[c][i] - rawD[c][i-1])==1) && 
          (rawD[c][i+1] == rawD[c][i-1]) &&
          ((rawD[c][i+1] == rawD[c][i+2]) || (rawD[c][i-1] == rawD[c][i-2])) )
        rawD[c][i] = rawD[c][i-1];
    } // Loop through channel's waveform
  } // Channel loop
} // Event::LossyCompress

void Event::Compress() {
  if (lossycompress) LossyCompress();
  // Compress the data into a string of single bytes (unsigned chars)
  unsigned char d; // Character pushed onto list
  for (int c=0; c<2; c++) { // Channel loop
    while (!ChanBuf[c].empty()) ChanBuf[c].pop_back();
    d = rawD[c][0];  // Push first sample on
    if (d==255) d=254; // Avoid special value
    int nSame = 0; // Keeps track of # of sequential samples that are the same
    ChanBuf[c].push_back(d);
    for (int i=1; i<nSamples; i++) { // Loop through channel, starting at 2nd index
      if (rawD[c][i]==255) rawD[c][i]=254; // Avoid special value
      if ((rawD[c][i] == rawD[c][i-1]) && (nSame<250)) { // Same and not full?
        nSame++; // Sample is the same as last one
      } // Same?
      else { // Not same?
        if (nSame < 4) { // Short sequence, so just write
          for (int j=0; j<nSame; j++)
            ChanBuf[c].push_back(d);
        } // Short sequence, so just write
        else { // Non-short sequence, so write number and then value
          ChanBuf[c].push_back(255); // Code word for repeats
          d = nSame;
          ChanBuf[c].push_back(d);
        } // Non-short sequence, so write number
        nSame = 0; // Reset sequence count
        d = rawD[c][i]; // Save this new value for the next possible sequence
        ChanBuf[c].push_back(d); // Write it as the head of a possible sequence
      } // Not same?
    } // Loop through channel
    // Write out any remaining sequence
    if (nSame > 0 && nSame < 4) { // Short sequence, so just write
      for (int j=0; j<nSame; j++)
        ChanBuf[c].push_back(d);
    } // Short sequence, so just write
    else if (nSame >= 4) { // Non-short sequence, so write number and then value
      ChanBuf[c].push_back(255); // Code word for repeats
      d = nSame;
      ChanBuf[c].push_back(d);
      nSame = 0; // Reset sequence count
    } // Non-short sequence, so write number
  } // Channel loop
} // Event::Compress

void Event::UnCompress() {
  // Uncompress the waveform data from the ChanBuf buffers
  unsigned char d, dsize, dsave; // Bytes from buffers
  dsize=0; dsave=0;
  for (int c=0; c<2; c++) { // Channel loop
    unsigned int nbuf = ChanBuf[c].size();
  if (debug) cout << "Uncompressing e="<<evtNum<<" c="<<c<<" nbuf="<<nbuf<<"\n";
    unsigned int index = 0; // Index into waveform
    unsigned int i = 0; // Index into buffer
    while (i<nbuf) { // Loop through buffer
      d = ChanBuf[c].at(i); // Get next byte
  if (debug) cout << "i="<<i<<" d="<<(int)d<<"\n";
      i++;
      if (d == 255) { // Sequence?
        dsize = 0; // Handle issues at the last sample
        if (i<nbuf) dsize = ChanBuf[c].at(i);
  if (debug) cout << "dsize="<<(int)dsize<<"\n";
        i++;
        while (dsize>0) {
          rawD[c][index] = dsave;
          index++;
          dsize--;
        }
      } // Sequence?
      else { // Not a sequence?
        if (index < 65536)
          rawD[c][index] = d; // Save value
        index++;
        dsave = d; // Remember in case there is a sequence
      } // Not a sequence?
    } // Loop through buffer
    nSamples = index;
  if (debug) cout << "Uncompressing finished. nSamples="<<nSamples<<"\n";
  } // Channel loop
} // Event::UnCompress

void Event::TestCompress() {
  CopyRaw(); // Test
  Compress();
  // Clear waveform
  for (int c=0; c<2; c++)
    for (int i=0; i<nSamples; i++)
      rawD[c][i] = 0;
  UnCompress();
  // Test recovered waveform
  for (int c=0; c<2; c++)
    for (int i=0; i<nSamples; i++)
      if (rawD[c][i] != D[c][i])
        cout << "Compress error: Evt="<<evtNum << " ch=" << c << " i=" << i << " raw=" << rawD[c][i] << " D=" << D[c][i] << "\n";
} // Event::TestCompress

void Event::Display(int minT, int maxT) {
  // Make an event display. 
  // To avoid bogging the code down with plotting tools,
  // we just write the waveform data to a temporary file
  // and call a plotter script.
  FILE* outF;
  std::string fName;
  fName = "tmpplot.ant";
  outF = fopen(fName.c_str(),"w");
  for (unsigned int i=0; i<nSamples; i++)
    fprintf(outF,"%d %d %d\n",i,rawD[0][i],rawD[1][i]);
  fclose(outF);
  char command[100];
  sprintf(command,"DiRPiEventDisplay.csh %d %d %d %d",runNum,evtNum,minT,maxT);
  system(command);
  DumpPulses();
} // Event::Display

class Run {
public: 
  unsigned long long int time; // Unix event time in milliseconds
  int runNum;
  int nEvts; // Number of events.
  string Desc; // Data taking run description
  int fVers; // File format version number
  int bVers; // Board hardware version number
  int clkspeed; // Clock speed in MHz (0=unknown)
  int memdepth; // memory depth in kB (0=unknown)
  float Temperature; // SiPM temperature
  int Prescale; // OR trigger prescale
  int TrigPosition; // Trigger position (1=early, 2=late)
  bool sftrg; // Software trigger enabled
  bool extrg; // External trigger enabled
  bool TrgCh1; // CH1 trigger enabled
  bool TrgCh2; // CH2 trigger enabled
  int PotVal1; // Digi pot setting 1 
  int PotVal2; // Digi pot setting 2 
  int PotVal3; // Digi pot setting 3 
  int PotVal4; // Digi pot setting 4 
  int DACValCh1; // Trig threshold DAC for channel 1 
  int DACValCh2; // Trig threshold DAC for channel 2 
  string ChName[2]; // Name for each channel
  vector<Event> Evts; // All events
  ifstream inF; // Used for writing .drp files
  ofstream outF; // Used for reading .drp files
  Run() {Clear();}
  void Clear(); // Clear all information
  void ClearEvts(); // Delete all events, but don't erase other information.
  void ClearPulses(); // Delete pulses from all events, but don't erase other information.
  void ReadTXT(int fnum); // Read from Data{fnum}.txt
  void ReadDRP(int fnum); // Read from Data{fnum}.drp
  void ReadDesc(); // Read a description string
  void ReadDRPiLine(); // Read a DRPi header line
  void ReadChName(int c); // Read a channel name string
  void ReadBoardLine(); // Read a board header line
  void ReadRunLine(); // Read a run header line
  void ReadMeanRMSBlock(int e); // Read a mean&rms block
  void ReadWaveformBlock(int e); // Read a waveform block
  void ReadPulseBlock(int e); // Read a pulse block
  void ReadEventBlock(); // Read an event block
  void Proc(); // Process all events in run
  void WriteDRP(int fnum, bool _w); // Write to Data{fnum}.drp(w) containing pulses and optionally waveforms
  void WriteTXT(int fnum); // Write to Data{fnum}.out
  void TestCompress(); // Test event compression
  void WritePulses(); // Write (append) the pulses to the appropriate files
  void DumpPulses(); // Write the pulses to the screen
  void Dump(); // Write event information to the screen
  void Display(int evtnum, int minT, int maxT); // Make an event display for the passed event number
};

void Run::ClearEvts() {
  while (Evts.size()>0) Evts.pop_back();
} 

void Run::ClearPulses() {
  for (unsigned int i=0; i<Evts.size(); i++) Evts[i].ClearPulses();
} 

void Run::Clear() {
  runNum = 0;
  nEvts = 0;
  time = 0;
  clkspeed = 0;
  memdepth = 0;
  TrigPosition = 0;
  Temperature = 0.;
  Prescale = 0;
  PotVal1 = 0;
  PotVal2 = 0;
  PotVal3 = 0;
  PotVal4 = 0;
  DACValCh1 = 0;
  DACValCh2 = 0;
  sftrg = false;
  extrg = false;
  fVers = 0;
  bVers = 2;
  Desc = "none";
  ChName[0] = "Ch1";
  ChName[1] = "Ch2";
  ClearEvts();
} 

void Run::ReadTXT(int fnum) {
 // Read events from text file Data{fnum}.txt in the local directory
 char fname[200];
 sprintf(fname,"Run%d_%d.txt",runNum,fnum);
 string fName;
 fName = fname;
 ifstream inF;
 inF.open(fName.c_str());
 if (debug) cout << "Opened file "<<fName<<"\n";
 unsigned long long int t; // time
 int evt, iAddr, C1, C2; // values from from file
 int prevdRunTime = 0;
 string line;
 Event Evt; // Local version of an event that we save and then fill.
 Evt.Clear();
 Evt.runNum = runNum;
 bool done = false; // end of file 
 while (! done) { // Read through file
  getline(inF,line,'\n');
  //if (debug) cout << "Read: [" << line << "]\n";
  if (strncmp(line.c_str(),"TIME ",5)==0) { // TIME line
    string word = line.substr(5,line.length()); // Cut off "TIME "
    sscanf(word.c_str(),"%llu",&t);
    //t -= toffset; // Apply an offset to expand time range
    if (Evts.size()==0 && time==0) { // First event in the run?
      time = t; // Save the first event time as the run time
    } // First event?
/*
    // This indicates the start of the next event.
    if (Evt.nSamples != 0) { // Non-empty event?
      // Save the previous event if it is not empty
      Evts.push_back(Evt);
      Evt.Clear(); // Set event as empty
    } // Non-empty event?
*/
    Evt.time = t; // Save the event time
    Evt.dRunTime = int(t-time); // Save the time since the start of the run
    Evt.dEvtTime = Evt.dRunTime-prevdRunTime; // Save the time since the previous event
    //if (debug) 
    //    cout << "TIME "<<t<<" RunTime="<<time
    //         <<" dRunTime="<<Evt.dRunTime
    //         <<" dEvtTime="<<Evt.dEvtTime
    //         <<" prevdRunTime="<<prevdRunTime<<"\n";
    prevdRunTime = Evt.dRunTime; // Save the time since the previous event
  } // TIME line
  if (strncmp(line.c_str(),"DATA: ",6)==0) { // DATA: line
    string word = line.substr(6,line.length());  // Cut off "DATA: "
    sscanf(word.c_str(),"%d %d %d %d",&evt, &iAddr, &C1, &C2);
    if (iAddr == 0 || Evts.size()==0) {
      Evt.evtNum = evt;
      Evts.push_back(Evt);
      nEvts++;
    }
    if (iAddr >= 0 && iAddr < 65536) {
      Evts[Evts.size()-1].rawD[0][iAddr] = C1;
      Evts[Evts.size()-1].rawD[1][iAddr] = C2;
      Evts[Evts.size()-1].nSamples++;
    }
    else
      cerr << "Illegal address read: "<<iAddr<<"\n";
  } // DATA line
  if (strncmp(line.c_str(),"memory_depth ",13)==0) { // memory_depth line
    string word = line.substr(13,line.length()); // Cut off "memory_depth "
    int _memdepth;
    sscanf(word.c_str(),"%d",&_memdepth);
    if (_memdepth == 4096) memdepth = 4;
    if (_memdepth == 65536) memdepth = 64;
  } // memory_depth line
  if (strncmp(line.c_str(),"sftrg ",6)==0) { // sftrg line
    string word = line.substr(6,line.length()); // Cut off "sftrg "
    int _sftrg;
    sscanf(word.c_str(),"%d",&_sftrg);
    if (_sftrg == 0) sftrg = false;
    if (_sftrg == 1) sftrg = true;
  } // sftrg line
  if (strncmp(line.c_str(),"extrg ",6)==0) { // extrg line
    string word = line.substr(6,line.length()); // Cut off "extrg "
    int _extrg;
    sscanf(word.c_str(),"%d",&_extrg);
    if (_extrg == 0) extrg = false;
    if (_extrg == 1) extrg = true;
  } // extrg line
  if (strncmp(line.c_str(),"TrgCh1 ",7)==0) { // TrgCh1 line
    string word = line.substr(7,line.length()); // Cut off "TrgCh1 "
    int _trgCh1;
    sscanf(word.c_str(),"%d",&_trgCh1);
    if (_trgCh1 == 0) TrgCh1 = false;
    if (_trgCh1 == 1) TrgCh1 = true;
  } // TrgCh1 line
  if (strncmp(line.c_str(),"TrgCh2 ",7)==0) { // TrgCh2 line
    string word = line.substr(7,line.length()); // Cut off "TrgCh2 "
    int _trgCh2;
    sscanf(word.c_str(),"%d",&_trgCh2);
    if (_trgCh2 == 0) TrgCh2 = false;
    if (_trgCh2 == 1) TrgCh2 = true;
  } // TrgCh2 line
  if (strncmp(line.c_str(),"TrgCnt1 ",8)==0) { // TrgCnt1 line
    string word = line.substr(8,line.length()); // Cut off "TrgCnt1 "
    int _count;
    sscanf(word.c_str(),"%d",&_count);
    Evt.TrgCnt[0]=_count;
    if (Evts.size()>0) Evts[Evts.size()-1].TrgCnt[0]=_count;
  } // TrgCnt2 line
  if (strncmp(line.c_str(),"TrgCnt2 ",8)==0) { // TrgCnt2 line
    string word = line.substr(8,line.length()); // Cut off "TrgCnt2 "
    int _count;
    sscanf(word.c_str(),"%d",&_count);
    Evt.TrgCnt[1]=_count;
    if (Evts.size()>0) Evts[Evts.size()-1].TrgCnt[1]=_count;
  } // TrgCnt2 line
  if (strncmp(line.c_str(),"Prescale ",9)==0) { // Prescale line
    string word = line.substr(9,line.length()); // Cut off "Prescale "
    sscanf(word.c_str(),"%d",&Prescale);
  } // Prescale line
  if (strncmp(line.c_str(),"Position ",9)==0) { // Position line
    string word = line.substr(9,line.length()); // Cut off "Position "
    sscanf(word.c_str(),"%d",&TrigPosition);
  } // Position line
  if (strncmp(line.c_str(),"clckspeed ",10)==0) { // clckspeed line
    string word = line.substr(10,line.length()); // Cut off "clckspeed "
    sscanf(word.c_str(),"%d",&clkspeed);
  } // clckspeed line
  if (strncmp(line.c_str(),"PotValCh1 ",10)==0) { // PotValCh1 line
    string word = line.substr(10,line.length()); // Cut off "PotValCh1 "
    sscanf(word.c_str(),"%d",&PotVal1);
  } // PotValCh1 line
  if (strncmp(line.c_str(),"PotValCh2 ",10)==0) { // PotValCh2 line
    string word = line.substr(10,line.length()); // Cut off "PotValCh2 "
    sscanf(word.c_str(),"%d",&PotVal2);
  } // PotValCh2 line
  if (strncmp(line.c_str(),"PotValCh3 ",10)==0) { // PotValCh3 line
    string word = line.substr(10,line.length()); // Cut off "PotValCh3 "
    sscanf(word.c_str(),"%d",&PotVal3);
  } // PotValCh3 line
  if (strncmp(line.c_str(),"PotValCh4 ",10)==0) { // PotValCh4 line
    string word = line.substr(10,line.length()); // Cut off "PotValCh4 "
    sscanf(word.c_str(),"%d",&PotVal4);
  } // PotValCh4 line
  if (strncmp(line.c_str(),"DACValCh1 ",10)==0) { // DACValCh1 line
    string word = line.substr(10,line.length()); // Cut off "DACValCh1 "
    sscanf(word.c_str(),"%d",&DACValCh1);
  } // DACValCh1 line
  if (strncmp(line.c_str(),"DACValCh2 ",10)==0) { // DACValCh2 line
    string word = line.substr(10,line.length()); // Cut off "DACValCh2 "
    sscanf(word.c_str(),"%d",&DACValCh2);
  } // DACValCh2 line
  if (strncmp(line.c_str(),"TEMP ",5)==0) { // TEMP line
    string word = line.substr(5,line.length()); // Cut off "TEMP "
    float _temp;
    sscanf(word.c_str(),"%f",&_temp);
    Temperature = _temp;
  } // TEMP line
  if (line[0] == 0) {
    done = true;
    if (Evt.nSamples != 0) { // Non-empty event?
      // Save the previous event if it is not empty
      Evts.push_back(Evt);
      Evt.Clear(); // Set event as empty
    } // Non-empty event?
  }
 } // Read through file
 if (memdepth==0 && Evts.size()>0) memdepth = Evts[0].nSamples;
 inF.close();
 return;
} // Run::ReadTXT

void Run::Proc() {
 for (unsigned int e=0; e<Evts.size(); e++) {
  Evts[e].Correct();
  Evts[e].CalcRange();
  Evts[e].FindPulses();
 }
} // Run::Proc

void Run::TestCompress() {
 for (unsigned int e=0; e<Evts.size(); e++) 
  Evts[e].TestCompress();
} // Run::TestCompress

void Run::WriteDRP(int fnum, bool _w) {
 // Write file in format described in http://dstuart.physics.ucsb.edu/cgi-bin/lgbk?41265
 unsigned char d1,d2,d3,d4,d5; // Used to write single bytes
 unsigned int ival;
 unsigned short ishrt;
 char ibyte;
 char buf[100];
 char fname[200];
 std::string fName;
 if (_w) // Including waveform?
  sprintf(fname,"Run%d_%d.drpw",runNum,fnum);
 else
  sprintf(fname,"Run%d_%d.drp",runNum,fnum);
 fName = fname;
 outF.open(fName.c_str(), ios::out | ios::binary);
 sprintf(buf,"DRPiv%dB%d\n",0,0); // Currently just 0's.
 outF.write(buf,9);
 sprintf(buf,"No description\n"); // Current empty.
 outF.write(buf,strlen(buf)); 
 int serialNum=0; // Board serial number; currently just 0
 sprintf(buf,"Brd%05d_CH1=description\n",serialNum); // Description currently empty
 outF.write(buf,strlen(buf)); 
 sprintf(buf,"Brd%05d_CH2=description\n",serialNum); // Description currently empty
 outF.write(buf,strlen(buf)); 
 // Skip the eprom block for now
 sprintf(buf,"Run %d %d %d %d\n",runNum,fnum,clkspeed,memdepth);
 outF.write(buf,strlen(buf)); 
 outF.write((char*)&time,sizeof(time)); // Write the run start time
 // Skip the configuration blocks
 // Write each event
 for (unsigned int e=0; e<Evts.size(); e++) { // event loop
  buf[0]='E';
  outF.write(buf,1);
  ival = Evts[e].evtNum;
  outF.write((char*)&ival,sizeof(ival)); // Event number within run (not necessarily within file)
  unsigned long long int dt = Evts[e].time;
  outF.write((char*)&dt,sizeof(dt)); // Time in ms
  // Skip quality flags
  // Skip trigger primitive counts
  // Mean and RMS:
  buf[0]='m';
  outF.write(buf,1);
  ibyte = (char) 0;
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte = (char) Evts[e].preMeanX10[0]/10;
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte=(char)((4*Evts[e].preRMSX10[0])/10);
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte = (char) Evts[e].mostProb[0];
  outF.write((char*)&ibyte,sizeof(ibyte));
  buf[0]='m';
  outF.write(buf,1);
  ibyte = (char) 1;
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte = (char) Evts[e].preMeanX10[1]/10;
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte=(char)((4*Evts[e].preRMSX10[1])/10);
  outF.write((char*)&ibyte,sizeof(ibyte));
  ibyte = (char) Evts[e].mostProb[1];
  outF.write((char*)&ibyte,sizeof(ibyte));
  if (_w) { // Write the waveform blocks?
    Evts[e].Compress(); // Build compressed waveform as a list of bytes
    for (int c=0; c<2; c++) { // Channel loop
      unsigned short cbsize = Evts[e].ChanBuf[c].size();
      buf[0]='w';
      outF.write(buf,1);
      ibyte = (char) c;
      outF.write((char*)&ibyte,sizeof(ibyte));
      outF.write((char*)&cbsize,sizeof(cbsize));
      char buflong[65536];
      for (int jj=0; jj<cbsize; jj++)
        buflong[jj] = Evts[e].ChanBuf[c].at(jj);
      outF.write(buflong,cbsize);
    } // Channel loop
  } // Write the waveform blocks?
  // Skip I2C block
  // Skip alarm block
  // Skip IO block
  // Skip deadtime block
  // Write the pulse information block
  for (int ip=0; ip<int(Evts[e].Pwidth.size()); ip++) { // Pulse loop
    // Write a pulse header
    buf[0]='p';
    outF.write(buf,1);
    ibyte = (char) Evts[e].Pch[ip];
    outF.write((char*)&ibyte,sizeof(ibyte));
    ishrt = (unsigned short) Evts[e].Pt[ip]; // Start of pulse
    outF.write((char*)&ishrt,sizeof(ishrt));
    ishrt = (unsigned short) Evts[e].Pwidth[ip]; // Width of pulse
    outF.write((char*)&ishrt,sizeof(ishrt));
    ival = (unsigned int) Evts[e].Phalft[ip]; // Half-risetime of pulse
    outF.write((char*)&ival,sizeof(ival));
    ibyte = (char) Evts[e].PpeakV[ip]; // Height of pulse
    outF.write((char*)&ibyte,sizeof(ibyte));
    ival = (unsigned int) Evts[e].Parea[ip]; // Area of pulse
    outF.write((char*)&ival,sizeof(ival));
    // Save 16 samples for the pulse, starting from two samples before the pulse starts.
    for (int j=Evts[e].Pt[ip]-2; j<Evts[e].Pt[ip]+14; j++) {
      ibyte = 0;
      if (j>=0 && j<Evts[e].nSamples)
        ibyte = (char) Evts[e].rawD[Evts[e].Pch[ip]][j];
      outF.write((char*)&ibyte,sizeof(ibyte));
    }
  } // Pulse loop
  buf[0]='e';
  outF.write(buf,1); // End of event keyword
 } // event loop
  buf[0]='f';
  outF.write(buf,1); // End of file keyword
 outF.close();
} // Run::WriteDRP

void Run::ReadDesc() { // Read a description string
 char desc[1000];
 char buf[1];
 int i=0;
 desc[0] = 0;
 buf[0] = 0;
 while (buf[0] != '\n') {
  inF.read(buf,1); // Read the next character
  desc[i] = buf[0];
  i++;
 }
 desc[i] = 0; // End the string
 Desc = desc; // Save into the Run object
} // Read a description string

void Run::ReadDRPiLine() { // Read a DRPi header line
  char buf[10];
  inF.read(buf,4); // Read and ignore the "RPiv" header characters
  inF.read(buf,1); // Read the file format version number
  buf[1] = 0;
  sscanf(buf,"%d",&fVers);
  inF.read(buf,1); // Read and ignore the "b" character
  inF.read(buf,1); // Read the board version number
  buf[1] = 0;
  sscanf(buf,"%d",&bVers);
  inF.read(buf,1); // Read and ignore the '\n'
  ReadDesc();   // Now read the description string
} // Read a DRPi header line

void Run::ReadChName(int c) { // Read a channel name string
 char chname[1000];
 char buf[1];
 int i=0;
 chname[0] = 0;
 buf[0] = 0;
 while (buf[0] != '\n') {
  inF.read(buf,1); // Read one character
  chname[i] = buf[0];
  i++;
 }
 chname[i] = 0; // End the string
 ChName[c] = chname; // Save into the Run object
} // Read a channel name string

void Run::ReadBoardLine() { // Read a Board header line
  char buf[10];
  inF.read(buf,2); // Read and ignore the "rd" header characters
  inF.read(buf,5); // Read the serial number
  buf[5]=0;
  int c; // channel number
  sscanf(buf,"%05d",&c);
  inF.read(buf,3); // Read and ignore the "_CH" characters
  inF.read(buf,1); // Read and ignore the = character
  ReadChName(c); // Read the channel name
} // Read a DRPi header line

void Run::ReadRunLine() { // Read a Run header line
  char line[1000];
  char buf[1];
  int i=0;
  line[0] = 0;
  inF.read(buf,3); // Read and ignore the "un " characters
  buf[0] = 0;
  while (buf[0] != '\n') {
    inF.read(buf,1); // Read one character
    line[i] = buf[0];
    i++;
  }
  line[i] = 0; // End the string
  int _runnum,_fnum; // We ignore these for now
  sscanf(line,"%d %d %d %d",&_runnum,&_fnum,&clkspeed,&memdepth);
cout << "Read run="<<_runnum<<" vs "<<runNum<<" _fnum="<<_fnum<<"\n";
  inF.read(reinterpret_cast<char *>(&time),sizeof(time)); // Read the run start time
} // Read a Run header line

void Run::ReadMeanRMSBlock(int e) { // Read a mean&rms block
  // This is called after finding a "m" special character.
  char ibyte;
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  int chan = (int)ibyte;
  if (chan == 48) chan = 0; // Correct or character format if needed
  if (chan == 49) chan = 1; // Correct or character format if needed
  if (chan<0 || chan>1) {
    cerr << "Illegal channel number: "<<chan<< ". Forcing into chan=0.\n";
    chan = 0;
  }
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  Evts[e].preMeanX10[chan] = (int)ibyte;
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  Evts[e].preRMSX10[chan] = (int)ibyte;
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  Evts[e].mostProb[chan] = (int)ibyte;
if (debug) cout << "MeanRMSBlock e="<<e<<" c="<<chan<<" 10m="<<Evts[e].preMeanX10[chan]<<" 10r="<<Evts[e].preRMSX10[chan]<<" mp="<<Evts[e].mostProb[chan]<<"\n";
} // Read a MeanRMS block

void Run::ReadWaveformBlock(int e) { // Read a waveform block
  // This is called after finding a "w" special character.
  char ibyte;
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  int chan = (int)ibyte;
  if (chan == 48) chan = 0; // Correct or character format if needed
  if (chan == 49) chan = 1; // Correct or character format if needed
  if (chan<0 || chan>1) {
    cerr << "Illegal channel number: "<<chan<< ". Forcing into chan=0.\n";
    chan = 0;
  }
  while (!Evts[e].ChanBuf[chan].empty()) 
    Evts[e].ChanBuf[chan].pop_back(); // Clear buffer
  unsigned short cbsize;
  inF.read(reinterpret_cast<char *>(&cbsize),sizeof(cbsize));
  char buflong[65536];
  inF.read(buflong,cbsize);
  for (unsigned short icb = 0; icb<cbsize; icb++) {
    char cb = buflong[icb];
    Evts[e].ChanBuf[chan].push_back(cb);
  }
} // Read a waveform block

void Run::ReadPulseBlock(int e) { // Read a pulse block
  // This is called after finding a "p" special character.
  char ibyte;
  unsigned short ishrt;
  unsigned int ival;
  int intval;
  // Read channel number
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  int chan = (int)ibyte;
if (debug) cout << "ReadPulseBlock e="<<e<<" c="<<chan<<"\n";
  if (chan == 48) chan = 0; // Correct or character format if needed
  if (chan == 49) chan = 1; // Correct or character format if needed
  if (chan<0 || chan>1) {
    cerr << "Illegal channel number: "<<chan<< ". Forcing into chan=0.\n";
    chan = 0;
  }
  Evts[e].Pch.push_back(chan);
  unsigned int ip = Evts[e].Pch.size()-1; // Index of this pulse
  if (debug) cout << "ip="<<ip<<"\n";
  // Figure out the index for this pulse within this channel.
  int ipulse = -1;
  for (int i=0; i<int(Evts[e].Pch.size()); i++)
    if (Evts[e].Pch[i]==Evts[e].Pch[ip])
      ipulse++;
  Evts[e].Pipulse.push_back(ipulse);
  // Read time of start of pulse
  inF.read(reinterpret_cast<char *>(&ishrt),sizeof(ishrt));
  intval = (int)ishrt;
  Evts[e].Pt.push_back(intval);
  // Read width of pulse
  inF.read(reinterpret_cast<char *>(&ishrt),sizeof(ishrt));
  intval = (int)ishrt;
  Evts[e].Pwidth.push_back(intval);
  // Read time of rising above half-max of pulse
  inF.read(reinterpret_cast<char *>(&ival),sizeof(ival));
  intval = (int)ival;
  Evts[e].Phalft.push_back(intval);
  // Read height of pulse
  inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
  intval = (int) ibyte;
  if (intval<0) intval += 256;
  Evts[e].PpeakV.push_back(intval); // Height of pulse
  // Read area of pulse
  inF.read(reinterpret_cast<char *>(&ival),sizeof(ival));
  intval = (int)ival;
  Evts[e].Parea.push_back(intval); // Area of pulse
  // Read 16 samples from the pulse, starting from two samples before the pulse starts.
  int pulseD[16];
  for (int j=0; j<16; j++) {
    inF.read(reinterpret_cast<char *>(&ibyte),sizeof(ibyte));
    pulseD[j] = (int)ibyte;
    if (pulseD[j]<0) pulseD[j] += 256;
    int index = Evts[e].Pt[ip]-2;
    if (index>=0 && index<65536)
      Evts[e].rawD[Evts[e].Pch[ip]][index] = pulseD[j];
  }
  if (debug) cout << "Done reading pulse block.\n";
} // Read a pulse block

void Run::ReadEventBlock() { // Read an event block
  Event evt;
  evt.runNum = runNum;
  Evts.push_back(evt);
  int e = Evts.size()-1;
  char buf[10];
  int ival;
  inF.read(reinterpret_cast<char *>(&ival),sizeof(ival)); // Event number within run (not necessarily within file)
  Evts[e].evtNum = ival;
  unsigned long long int t;
  inF.read(reinterpret_cast<char *>(&t),sizeof(t)); // Time since start of run in ms
  Evts[e].time = t;
  Evts[e].dRunTime = int(t-time); // Save the time since the start of run
  if (e==0)
    Evts[e].dEvtTime = 0;
  else
    Evts[e].dEvtTime = Evts[e].dRunTime-Evts[e-1].dRunTime;
  // Skip quality flags
  // Skip trigger primitive counts
  bool done = false;
  while (! done) { // Read blocks to end of event
    inF.read(buf,1); // Read the next header character
  if (debug) cout << "Header character = "<<buf[0]<<"\n";
    if (buf[0]=='e')
      done = true;
    else if (buf[0]=='m')
      ReadMeanRMSBlock(e);
    else if (buf[0]=='w')
      ReadWaveformBlock(e);
    else if (buf[0]=='p')
      ReadPulseBlock(e);
  } // Read blocks to end of event
  Evts[e].UnCompress();
} // Read an event block

void Run::ReadDRP(int fnum) {
 // Read file in format described in http://dstuart.physics.ucsb.edu/cgi-bin/lgbk?41265
 char buf[10];
 char inchar;
 char fname[200];
 string fName;
 // Try to read the waveform file, if that fails, just read the pulse file
 sprintf(fname,"Run%d_%d.drpw",runNum,fnum);
 fName = fname;
 inF.open(fName.c_str(), ios::in | ios::binary);
 if (! inF.good()) { // Try to read the pulse file
  sprintf(fname,"Run%d_%d.drp",runNum,fnum);
  fName = fname;
  inF.open(fName.c_str(), ios::in | ios::binary);
 } // Try to read the pulse file
 if (! inF.good()) { // Failed to read?
  cerr << "Could not read "<<fName<<"\n";
  return;
 } // Failed to read?
 inF.read(buf,1);
 while (inF.good()) { // Read until end of file
  if (buf[0] == 'D') ReadDRPiLine();
  else if (buf[0] == 'B') ReadBoardLine();
  else if (buf[0] == 'R') ReadRunLine();
  // Skip the eprom block for now
  // Skip the configuration blocks
  else if (buf[0] == 'E') {
    ReadEventBlock();
    if (time == 0) time = Evts[0].time; // Update run time
  }
  // Skip I2C block
  // Skip alarm block
  // Skip IO block
  // Skip deadtime block
  // Skip pulse block for now
  inF.read(buf,1);
 } // Read until end of file
 inF.close();
} // Run::ReadDRP

void Run::WriteTXT(int fnum) {
 FILE* outF;
 char fname[200];
 std::string fName;
 sprintf(fname,"Run%d_%d.txt",runNum,fnum);
 fName = fname;
 outF = fopen(fName.c_str(),"w");
 for (unsigned int e=0; e<Evts.size(); e++) { // event loop
    fprintf(outF,"TIME %llu\n",Evts[e].time);
    //fprintf(outF,"TIME %ld\n",Evts[e].time+toffset);
    for (int i=0; i<Evts[e].nSamples; i++)
      fprintf(outF,"DATA: %d  %d %d %d\n",Evts[e].evtNum,i,Evts[e].rawD[0][i],Evts[e].rawD[1][i]);
 } // event loop
 fclose(outF);
} // Run::WriteTXT

void Run::WritePulses() {
  // Append the pulse ant information from each event to the appopriate files
  for (unsigned int e=0; e<Evts.size(); e++) { // event loop
    Evts[e].WritePulses();
  } // event loop
} // Run::WritePulses

void Run::Display(int evtnum, int minT, int maxT) {
  if (evtnum>=0 && evtnum<int(Evts.size()))
    Evts[evtnum].Display(minT, maxT);
}

void Run::DumpPulses() {
  // Dump all the pulse information to the screen
  for (unsigned int e=0; e<Evts.size(); e++) { // event loop
    Evts[e].DumpPulses();
  } // event loop
}

void Run::Dump() {
  // Dump all run information to the screen
  cout << "Dump of run "<<runNum<<'\n';
  cout << "time "<<runNum<<" "<<time<<"\n";
  cout << "nEvts "<<runNum<<" "<<nEvts<<"\n";
  cout << "EvtsSize "<<runNum<<" "<<Evts.size()<<"\n";
  cout << "Desc "<<runNum<<" "<<Desc<<"\n";
  cout << "ChName0 "<<runNum<<" "<<ChName[0]<<"\n";
  cout << "ChName1 "<<runNum<<" "<<ChName[1]<<"\n";
  cout << "fVers "<<runNum<<" "<<fVers<<"\n";
  cout << "bVers "<<runNum<<" "<<bVers<<"\n";
  cout << "clkspeed "<<runNum<<" "<<clkspeed<<"\n";
  cout << "memdepth "<<runNum<<" "<<memdepth<<"\n";
  cout << "Temperature "<<runNum<<" "<<Temperature<<"\n";
  cout << "Prescale "<<runNum<<" "<<Prescale<<"\n";
  cout << "TrigPosition "<<runNum<<" "<<TrigPosition<<"\n";
  cout << "sfttrg "<<runNum<<" "<<sftrg<<"\n";
  cout << "exttrg "<<runNum<<" "<<extrg<<"\n";
  cout << "TrgCh1 "<<runNum<<" "<<TrgCh1<<"\n";
  cout << "TrgCh2 "<<runNum<<" "<<TrgCh2<<"\n";
  cout << "PotVal1 "<<runNum<<" "<<PotVal1<<"\n";
  cout << "PotVal2 "<<runNum<<" "<<PotVal2<<"\n";
  cout << "PotVal3 "<<runNum<<" "<<PotVal3<<"\n";
  cout << "PotVal4 "<<runNum<<" "<<PotVal4<<"\n";
  cout << "DAQValCh1 "<<runNum<<" "<<DACValCh1<<"\n";
  cout << "DAQValCh2 "<<runNum<<" "<<DACValCh2<<"\n";
  cout << "Dump of all events and pulses follows...\n";
  for (unsigned int e=0; e<Evts.size(); e++) { // event loop
    Evts[e].Dump();
    Evts[e].DumpPulses();
  } // event loop
}

int main(int argc, char *argv[])
{
  // Set defaults for pulse finding
  ScaleFactor[0]=1.;
  ScaleFactor[1]=1.;
  Threshold[0]=4;
  Threshold[1]=4;
  nQuiet[0]=5;
  nQuiet[1]=5;
  nMinWidth[0]=4;
  nMinWidth[1]=4;
  iStartSearch = 5;

  // argv[0] is the command: compress, lossycompress, uncompress, or pulses
  // argv[1] is the run number
  // argv[2] is the file number
  // The file names are built from these inputs.
  // Text input/output files are Run<RunNumber>_<fileNum>.txt
  // Compressed input/output files are Run<RunNumber>_<fileNum>.drp
  if (argc < 3) {
    cerr << "Usage: DiRPi Command RunNumber FileNumber\n";
    cerr << "where Command = compress, lossycompress, uncompress, or pulses\n";
    return 1;
  }
  // Read the run and file number from the command line parameters
  int rnum, fnum;
  sscanf(argv[2],"%d",&rnum);
  sscanf(argv[3],"%d",&fnum);
  // Read and process the command from the command line parameters
  if (strncmp(argv[1],"compress",13)==0) { // Compress the file
    cout << "Compressing Run"<<rnum<<"_"<<fnum<<".txt\n";
    lossycompress = 0;
    Run r;
    r.runNum = rnum;
    r.ReadTXT(fnum);
    r.Proc();
    r.WriteDRP(fnum,true);
    return 0;
  } // Compress the file
  if (strncmp(argv[1],"lossycompress",13)==0) { // Compress the file
    cout << "LossyCompressing Run"<<rnum<<"_"<<fnum<<".txt\n";
    lossycompress = 1;
    Run r;
    r.runNum = rnum;
    r.ReadTXT(fnum);
    r.Proc();
    r.Dump();
    r.WriteDRP(fnum,true);
    return 0;
  } // Compress the file
  if (strncmp(argv[1],"savepulses",13)==0) { // Save only pulse information
    cout << "Saving pulses for Run"<<rnum<<"_"<<fnum<<".txt\n";
    Run r;
    r.runNum = rnum;
    r.ReadTXT(fnum);
    r.Proc();
    r.WriteDRP(fnum,false);
    return 0;
  } // Compress the file
  if (strncmp(argv[1],"uncompress",13)==0) { // Compress the file
    cout << "Uncompressing Run"<<rnum<<"_"<<fnum<<".drpw\n";
    Run r;
    r.runNum = rnum;
    r.ReadDRP(fnum);
    r.Proc();
    r.WriteTXT(fnum);
    r.Clear();
    return 0;
  } // Compress the file
  if (strncmp(argv[1],"pulses",6)==0) { // Pulses
    if (fnum<0) { // Negative fnum implies loop
      Run r;
      r.Clear();
      r.runNum = rnum;
      for (int ifnum=0; ifnum<=abs(fnum); ifnum++) { // file loop
        //cout << "Writing pulse information from Run"<<rnum<<"_"<<ifnum<<".drp\n";
        r.ReadDRP(ifnum);
        r.WritePulses();
        r.ClearEvts();
      } // file loop
    } // Negative fnum implies loop
    else { // Positive fnum
      cout << "Writing pulse information from Run"<<rnum<<"_"<<fnum<<".drp\n";
      Run r;
      r.runNum = rnum;
      r.ReadDRP(fnum);
      r.WritePulses();
      r.Clear();
    } // Positive fnum
    return 0;
  } // Pulses
  if (strncmp(argv[1],"findpulses",10)==0) { // Pulses
    if (fnum<0) { // Negative fnum implies loop
      Run r;
      r.Clear();
      r.runNum = rnum;
      for (int ifnum=0; ifnum<=abs(fnum); ifnum++) { // file loop
        //cout << "Writing pulse information from Run"<<rnum<<"_"<<ifnum<<".drp\n";
        r.ReadDRP(ifnum);
        r.ClearPulses();
        r.Proc();
        r.WritePulses();
        r.ClearEvts();
      } // file loop
    } // Negative fnum implies loop
    else { // Positive fnum
      cout << "Writing pulse information from Run"<<rnum<<"_"<<fnum<<".drp\n";
      Run r;
      r.runNum = rnum;
      r.ReadDRP(fnum);
      r.ClearPulses();
      r.Proc();
      r.WritePulses();
      r.Clear();
    } // Positive fnum
    return 0;
  } // FindPulses
  if (strncmp(argv[1],"display",13)==0) { // Display
    if (argc < 6) {
      cerr << "Must pass event number, min time, and max time for display\n";
    }
    else {
      int evtnum;
      sscanf(argv[4],"%d",&evtnum);
      int minT, maxT;
      sscanf(argv[5],"%d",&minT);
      sscanf(argv[6],"%d",&maxT);
      if (minT==0 && maxT==0) {
	     minT = 0;
	     maxT = 65536;
      }
      cout << "Making event display for event "<<evtnum<<" from Run"<<rnum<<"_"<<fnum<<".drp\n";
      Run r;
      r.runNum = rnum;
      r.ReadDRP(fnum);
      r.ClearPulses(); // Remove any pre-existing pulses. They will be regenerated.
      r.Proc();
      r.Display(evtnum,minT,maxT);
      r.Clear();
    }
    return 0;
  } // Display
  // If we get here, there was an unexpected command
  cerr << "Unexpected command parameters. Nothing done.\n";
  return 1;
}
