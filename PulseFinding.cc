// PulseFinding.cc  David Stuart, March 2023
// This program handles pulse finding for DiRPi waveforms.
 
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <iomanip>  // Necessary for "setprecision"
#include <experimental/filesystem> // necessary for directory iterator

using namespace std;
namespace fs = std::experimental::filesystem;
using recursive_directory_iterator = fs::recursive_directory_iterator;


bool debug=false;
// Parameters for pulse finding
float Threshold_mV[8];
int nQuiet;
int nMinWidth[8];
int iStart = 50; // Number of initial samples to skip before looking for pulses
float ChannelScaleFactor[8];

// Information for all pulses in the event
int pulsenum; // Pulse number within the event across all channels
int nPulses[8]; // Number of pulses in each channel
vector<float> Pt; // Time of pulse
vector<int> Pch; // channel #
vector<int> Pipulse; // pulse # for this channel; first=0
vector<float> Pwidth; // Width
vector<float> PpeakV; // pulse height
vector<float> Parea; // pulse area
void ClearPulses() {
 pulsenum = 0;
 for (int i=0; i<8; i++)
  nPulses[i] = 0;
 while (!Pt.empty()) Pt.pop_back();
 while (!Pch.empty()) Pch.pop_back();
 while (!Pipulse.empty()) Pipulse.pop_back();
 while (!Pwidth.empty()) Pwidth.pop_back();
 while (!PpeakV.empty()) PpeakV.pop_back();
 while (!Parea.empty()) Parea.pop_back();
}

class Wave {
public: 
  vector<float> t, V, Vraw;
  int runNum, evtNum, chan;
  float tMax, tMin, VMax, VMin;
  float meanPre, rmsPre; // mean and rms measured from initial (pre)samples 
  void Clear();
  void AddPoint(float _t, float _Vraw, float _Vcor) {t.push_back(_t); V.push_back(_Vcor); Vraw.push_back(_Vraw);}
  int nSamples() {return t.size();}
  void Set(int _run, int _evt, int _chan) {runNum = _run; evtNum = _evt; chan = _chan;}
  void CalcRange();
  void CalcAverage(); // Calculate the nominal average and save in meanPre and rmsPre
  int CalcAverage(int i1, int i2, int nIter, float *mean, float *rms); // returns # of samples used
  int CalcMostProbable(int i1, int i2, float *mostProb); // returns most probable value, with 1mV binning, over passed range. Returns # of samples in most probable
  void Correct(); // Apply corrections
  void Dump(); // Dump contents to screen for debugging
  void FindPulses();
};
void Wave::Clear() {
  runNum = 0;
  evtNum = 0;
  chan = -1;
  meanPre = 0.;
  rmsPre = 1.;
  while (t.size()>0) t.pop_back();
  while (V.size()>0) V.pop_back();
  while (Vraw.size()>0) Vraw.pop_back();
} 
void Wave::CalcRange() {
  tMin = 9.E9; tMax = -9.E9;
  VMin = 9.E9; VMax = -9.E9;
  int n = nSamples();
  for (int i=0; i<n; i++) {
    if (t[i]<tMin) tMin = t[i];
    if (t[i]>tMax) tMax = t[i];
    if (V[i]<VMin) VMin = V[i];
    if (V[i]>VMax) VMax = V[i];
  }
}
int Wave::CalcMostProbable(int i1, int i2, float *mostProb) {
  // Calculate the most probable value in passed range. 
  // This is done with mV resolution for ease of sampling.
  // Count how often we have each value, from -40000mV to 40000mV
  int nCounts[80000];
  for (int i=0; i<80000; i++) {
    nCounts[i] = 0;
  }
  int n = nSamples();
  for (int i=0; i<n; i++) {
    int iV = int(10000*V[i])+40000;
    if (iV>=0 && iV<80000) {
      nCounts[iV] = nCounts[iV]+1;
    }
  }
  int imaxV = 0;
  float maxV = -9999999.;
  for (int i=0; i<80000;i++) {
    if (nCounts[i]>maxV) {
      maxV = nCounts[i];
      imaxV = i;
    }
  }
  *mostProb = (imaxV - 40000)/10000.; // Make it signed
  return maxV;
} // CalcMostProbable

int Wave::CalcAverage(int i1, int i2, int nIter, float *meanVal, float *rmsVal) {
 // Calculate the average and RMS for samples in the range i1 to i2.
 // Do an iterative calculation, removing any samples that fail for the following reasons:
 //   Differ from initial mean value by more than 3 times RMS
 //   Differ from either neighboring sample by more than 2 times RMS
 // We iterate repeatedly with an improved guess at the mean and rms each time
 // Return value is the number of samples used in the final calculation.
 float _mean = *meanVal;
 float _rms = *rmsVal*2; // Set an initial guess for first iteration
 if (_rms < 2.) _rms = 2.; // Protect against initial zero value
 int MaxIter = nIter;
 int nUsed = 0;
 if (i1<=0) i1=1; // Protect ranges; need to check neighbors
 if (i2>=nSamples()-1) i2=nSamples()-2; // Need to check neighbors
 if (i2<i1) return 0; // Protection.
 for (int iter = 0; iter < MaxIter; iter++) { // Iteration
   // Calculate the window
   float minVWindow = _mean-3*_rms;
   float maxVWindow = _mean+3*_rms;
   float maxdV = 2*_rms;
   float sumV = 0.;
   float sum2V = 0.;
   nUsed = 0;
   float filterval = 0.002;
   bool good = false; // Shows if a particular sample can be used
   for (int i=i1; i<i2; i++) {
      good = true;
      if (V[i] < minVWindow) good = false;
      if (V[i] > maxVWindow) good = false;
      if (fabs(V[i]-V[i+1])>maxdV) good = false;
      if (fabs(V[i]-V[i-1])>maxdV) good = false;
      if (good) {
        sumV += V[i];
        sum2V += V[i]*V[i];
        nUsed++;
      }
      good = true;
    }
    if (nUsed > 10) { // Enough points
      _mean = sumV/(nUsed);
      _rms = sqrt(sum2V/(nUsed) - _mean*_mean);
    } // Enough points
    else
      _rms *= 2; // Try again with a broader window
  } // iteration
  *meanVal = _mean;
  *rmsVal = _rms;
  return nUsed;
}
void Wave::CalcAverage() {
 // Calculate mean and RMS from first 2000 samples
 meanPre = 0.;
 rmsPre = 3.;
 int nUsed;
 nUsed = CalcAverage(0, 2000, 1, &meanPre, &rmsPre);
}
void Wave::Correct() {
  // Apply corrections, including scaling and smoothing
  int n = nSamples();
  for (int i=0; i<n; i++) {
    V[i] *= ChannelScaleFactor[chan-1];
    Vraw[i] *= ChannelScaleFactor[chan-1];
  }
  // Calculate the most probable value for the first 2000 samples
  float mostProb;
  int nMostProb = CalcMostProbable(0, 2000, &mostProb);
  if (evtNum<5)
    cout << "Event "<<evtNum<<" chan "<<chan<<" mostProb="<<mostProb<<" nMostProb="<<nMostProb<<"\n";
  if (nMostProb>20)
    for (int i=0; i<n; i++)
      V[i] -= mostProb;

  // Calculate the mean and RMS from the initial 2000 samples
  float meanVal = 0.;
  float rmsVal = 5.;
  int nUsed;
  nUsed = CalcAverage(0, 2000, 4, &meanVal, &rmsVal);
  meanPre = meanVal;
  rmsPre = rmsVal;
  // Subtract the mean from all samples
  if (nUsed > 20)
    for (int i=0; i<n; i++)
      V[i] = V[i] - meanPre;
  // Recalculate the mean and RMS values after this correction.
  nUsed = CalcAverage(0, 2000, 1, &meanVal, &rmsVal);
}

void Wave::Dump() {
  // Dump contents to screen for debugging
  cout << "Dump of Run "<<runNum<<" event "<<evtNum<<" channel "<<chan<<"\n";
  for (int i=0; i<nSamples(); i++)
    cout << i << " " << t[i] << " " << V[i] << "\n";
  cout << "\n";
}

void Wave::FindPulses() {
  // Find all pulses in this waveform
  // The algorithm is to iterate through the samples in each waveform
  // When a sample above threshold is found, it may be the start of a pulse.
  // We first check if there were a sufficient number of "quiet" samples before it,
  // i.e., consecutive samples below threshold.
  // Then we iterate through samples that remain above threshold and call it a pulse
  // if there a sufficient number.
  // We then save information about the pulse into the vectors of pulse info.
  int n = nSamples();
  float localThr = Threshold_mV[chan-1];
  for (int i=iStart; i<n-nMinWidth[chan-1]-nQuiet; i++) { // loop through the waveform
    if (V[i]>=localThr) { // Above threshold?
      // Check if previous samples are below threshold
      bool isQuiet = true;
      for (int j=1; j<=nQuiet; j++)
        if (V[i-j]>=localThr)
          isQuiet = false;
      if (isQuiet) { // Passes the presample quiet requirement?
        // Require at least nMinWidth subsequent samples above threshold
        bool goodTrig = true;
        for (int j=i+1; j<i+1+nMinWidth[chan-1] && j<n; j++)
          if (V[j]<localThr)
            goodTrig = false;
        if (goodTrig) { // Passes minWidth requirement?
          // Find the end of the pulse, as the time where nQuiet samples are below threshold in a row.
          int nBelow = 0;
          bool done = false;
          int j=i;
          while (!done && j<n-1) { // Looking for end of pulse
            j++;
            if (V[j]<localThr)
              nBelow++;
            else
              nBelow = 0;
            if (nBelow >= nQuiet)
              done = true;
          } // Looking for end of pulse
          int endIndex = j;
          float width = (t[endIndex-nQuiet]-t[i]); // width in ns
          // Find the end of the pulse by looping forward until it falls below threshold.
          pulsenum++;
          // Measure peak and area. Include nQuiet pre/postsamples for area calculation.
          float peakV = 0.;
          float peakT = 0.;
          int ipeak = i;
          double area = 0.;
          for (int k=i-nQuiet; k<=endIndex; k++) {
            area += V[k]*(t[k]-t[k-1]);
            if (V[k]>peakV){ 
              peakV = V[k];
              peakT = t[k];
              ipeak = k;
            }
          }
          // Save pulse
          nPulses[chan-1]++;
          Pch.push_back(chan);
          Pipulse.push_back(pulsenum-1);
          Pt.push_back(t[i]);
          Pwidth.push_back(width);
          PpeakV.push_back(peakV);
          Parea.push_back(area);
        } // Passes minWidth requirement
      } // Passes the presample quiet requirement?
    } // Above threshold? 
  } // loop through the waveform
}

// The waveform data from up to 8 channels in the current event
Wave w[8];

void FindAllPulses() { // Find all pulses across all 8 channels
 ClearPulses();
 for (int c=0; c<8; c++) { // Channel loop
  w[c].FindPulses();
 } // Channel loop
} // FindAllPulses

void DumpPulses(int EventNum) { // Dump pulses to cout
  for (int i=0; i<Pwidth.size(); i++) { 
    cout << setiosflags(ios::fixed) << setprecision(2);
    cout << "PULSEANT:";
    cout << " " << EventNum;  // 1=event number
    cout << " " << EventNum; // 2=time in seconds since previous event
    cout << " " << 1; // 3=time in seconds since previous event
    cout << " " << Pipulse.size();    // 4=#pulses in the event
    cout << " " << nPulses[0];    // 5=# of pulses in ch0
    cout << " " << nPulses[1];    // 6=# of pulses in ch1
    cout << " " << nPulses[2];    // 7=# of pulses in ch2
    cout << " " << nPulses[3];    // 8=# of pulses in ch3
    cout << " " << w[Pch[i]-1].rmsPre; // 9=RMS of this channel in sideband
    cout << " " << Pch[i];        // 10=channel number
    cout << " " << Pipulse[i];    // 11=pulsenum within event for this channel
    cout << " " << Pt[i];     // 12=start time of pulse within waveform in ns
    cout << " " << Parea[i];  // 13=pulse area in nVs
    cout << " " << PpeakV[i]; // 14=pulse peak height in mV
    cout << " " << Pwidth[i]; // 15=pulse width in ns
    cout << " " << Parea[i];  // 16=pulse area in SPE
    float keV = Parea[i];
    if (Pch[i]==1) // LYSO+SiPM1
      keV = keV * 0.00647 + 0.; // Patrick's calibration
    if (Pch[i]==2) // LYSO+SiPM1
      keV = keV * 0.00699 + 0.; // Patrick's calibration
    if (Pch[i]==5) // LaBr
      keV = keV * 0.0140 + 0.7144; // Brunel's calibration for LaBr from area in pVs
    cout << " " << keV;  // 17=pulse area in keV
    cout << "\n";
  }
  // Next we dump the "keV" formatting of pulse information.
  // It contains the total area per event rather than individual pulse information
  float keVSum[4];
  for (int c=0; c<4; c++) {
    keVSum[c] = 0.;
    for (int i=0; i<Pwidth.size(); i++) 
     if (Pch[i] == c+1) {
      keVSum[c] += Parea[i];
     }
  }
  cout << setiosflags(ios::fixed) << setprecision(5);
  cout << "KEVANT:";
  cout << " " << w[0].runNum; // 1=run number
  cout << " " << EventNum; // 2=event number
  cout << " " << keVSum[0]; // 3=Total energy in channel 1
  cout << " " << keVSum[1]; // 4=Total energy in channel 2
  cout << " " << keVSum[2]; // 5=Total energy in channel 3
  cout << " " << keVSum[3]; // 6=Total energy in channel 4
  cout << " " << w[0].rmsPre; // 7=RMS in sideband for channel 1
  cout << " " << w[1].rmsPre; // 8=RMS in sideband for channel 2
  cout << " " << w[2].rmsPre; // 9=RMS in sideband for channel 3
  cout << " " << w[3].rmsPre; // 10=RMS in sideband for channel 4
  cout << "\n";
} // DumpPulses

void ConvertDigiFiles(int _runNum) {
 // Read all the digi files in the local directory and write out a pulse ant file Run{_runNum}.ant.
 FILE* infDigi;
 FILE* outfAnt;
 int nchar; // Number of characters read; <0 means end of file.
 char fname[200];
 std::string fName;
 sprintf(fname,"Run%d.ant",_runNum);
 fName = fname;
 int fnum = 0;
 int _evtNum = -1; // Event number incremented across all files
 int prevEvt = -1; // Used to recognize new event
 float timePerSample = 1.; // for simplicity
 float VPerADC = 1.; // for simplicity
 while (fnum >= 0) { // Read through file; fnum increments but gets set to -1 when a read fails
  sprintf(fname,"Run%d_%d.txt",_runNum,fnum);
  cout<<"Reading "<<fname<<endl;
  fName = fname;
  infDigi = fopen(fName.c_str(),"r");
  if ( ! infDigi ) { // Failed to open input file?
   fnum = -1;
  } // Failed to open input file?
  else { // Opened input file?
    cout << "Opened file "<<fName<<"\n";
    nchar = 1; 
    int nSamples; // Number of samples per event
    while (nchar > 0) { // Read through file
     int iEvt, iSample, i1, i2; // Integers read from file
     nchar = fscanf(infDigi,"%d %d %d %d",&iEvt,&iSample,&i1,&i2);
     if (nchar>0) { // Read something?
      if (iEvt != prevEvt) { // Start of new event?
        if (_evtNum>0) { // Process a now complete (previous) event
         ClearPulses();
         for (int chan=0; chan<2; chan++) { // Channel loop
          w[chan].CalcAverage();
          w[chan].CalcRange();
          w[chan].Correct(); // Measure and subtract pedestal
          w[chan].FindPulses();
         } // Channel loop
         DumpPulses(_evtNum);
        } // Process a now complete (previous) event
        _evtNum++;
        prevEvt = iEvt;
        for (int chan=0; chan<2; chan++) { // Channel loop
          w[chan].Clear();
          w[chan].Set(_runNum,_evtNum,chan+1);
        } // Channel loop
      } // Start of new event?
      w[0].AddPoint(iSample*timePerSample,i1*1.,i1*VPerADC);
      w[1].AddPoint(iSample*timePerSample,i2*1.,i2*VPerADC);
     } // Read something?
     else { // Reached end of file?
      fclose(infDigi);
      fnum++;
     } // Reached end of file?
    } // Read through file
  } // Opened input file?
 } // Read through file
 return;
} // ConvertDigiFiles

void CheckChannelConsistency() { // Check that all 8 waveforms are aligned and consistent
 // Check that sizes match
 int n = w[0].nSamples();
 for (int c=1; c<8; c++) {
  int n2 = w[c].nSamples();
  if (n2 != n)
   cerr << "w["<<c<<"].nSamples != w[0].nSamples in event "<< w[0].evtNum <<"\n";
 }
 // Check that times match within 1 ns
 for (int c=1; c<8; c++) { // Channel loop
  for (int i=0; i<n; i++) { // index loop
    if (fabs(w[c].t[i]-w[0].t[i])>1.)
     cerr << "Time mistmatch in channel "<<c<<" at index "<<i<<"\n";
  } // index loop
 } // Channel loop
} // CheckChannelConsistency

int main(int argc, char **argv)
{
  int fin;
  std::string verb;
  std::string tmpstring;
  int i, N, n;
  bool averaging = false;
  int nAvg = 0;

  // Parameters for pulse finding
 Threshold_mV[0] = 4.;
 Threshold_mV[1] = 4.;
 Threshold_mV[2] = 4.;
 Threshold_mV[3] = 4.;
 Threshold_mV[4] = 4.;
 Threshold_mV[5] = 4.;
 Threshold_mV[6] = 4.;
 Threshold_mV[7] = 4.;
 nQuiet = 10;
 nMinWidth[0]= 10;
 nMinWidth[1]= 10;
 nMinWidth[2]= 10;
 nMinWidth[3]= 10;
 nMinWidth[4]= 10;
 nMinWidth[5]= 10;
 nMinWidth[6]= 10;
 nMinWidth[7]= 10;

 ChannelScaleFactor[0]= 1.;
 ChannelScaleFactor[1]= 1.;
 ChannelScaleFactor[2]= 1.;
 ChannelScaleFactor[3]= 1.;
 ChannelScaleFactor[4]= 1.;
 ChannelScaleFactor[5]= 1.;
 ChannelScaleFactor[6]= 1.;
 ChannelScaleFactor[7]= 1.;

 int RunNum = 10;
 ConvertDigiFiles(10);

 return 0;
}

