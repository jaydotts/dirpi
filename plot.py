#!/usr/bin/env python
# coding: utf-8

# In[98]:


import numpy as np 
import re
import matplotlib.pyplot as plt
import os 
import matplotlib

########################### params
# target filename
file = "DiRPi0_DATA_DRY/30-03-2023_01:35:500.txt"

# In[2]:

nEvents = 10

def parse_output(file):
    
    ################################ parse data out of txt file 
    with open(file,'r') as f: 
        lines = f.readlines()
    f.close()

    # Data format: 
    # [EVENTNUM, BITNUM(/4096), CH1SCALED(/255), CH2SCALED(/255)]

    data = np.zeros((len(lines),4))
    print("Parsing txt file...")

    for i,line in enumerate(lines): 
        line = line.split(',')
        s = [int(s) for s in re.findall(r'-?\d+\.?\d*', line[0])]
        data[i,:] = s
        
    print("done")
    # get the eventNums back out into an iterable list

    eventNums = [int(x) for x in (np.unique(data[:,0]))]
    
    # output format: 
    return eventNums, data


# In[108]:


def plot_histograms(data, chan1, chan2):
    
    
    eventNum = [int(x) for x in (np.unique(data[:,0]))]
    
    maxValsCh1 = []
    maxValsCh2 = []
    
    for event in eventNum:

        eventData = data[data[:,0]==event][:,1:] 
        bitNum = eventData[:,0]
        ch1Data = eventData[:,1]
        ch2Data = eventData[:,2]

        maxValsCh1.append(np.max(eventData[:,1],0))
        maxValsCh2.append(np.max(eventData[:,2],0))

    valsInMv = 3000*(np.array(maxValsCh1)/256)
    valsInMv = valsInMv[valsInMv<800]
    plt.hist(valsInMv, bins=18)
    plt.savefig("histograms.png")


# In[15]:


def plot_waveforms(data, maxPlotNum, chan1, chan2):
    
    wf_only = False
    eventNum = [int(x) for x in (np.unique(data[:,0]))]
    
    if(len(eventNum) > maxPlotNum):
        print(f"Plotting only first {maxPlotNum} events") # make this into a log
        eventNumFirst = eventNum[:maxPlotNum]

    fig, axs = plt.subplots(nrows=len(eventNumFirst), figsize=(15, 12), sharex = not wf_only)
    [ax.grid() for ax in axs]
    plt.subplots_adjust(hspace=0.5)

    for event in eventNumFirst: 

        # select only 4096 bits for this eventNum
        # also index out the eventNum to clean up the data

        eventData = data[data[:,0]==event][:,1:] 

        # now the format is 
        # [BITNUM(/4096), CH1SCALED(/255), CH2SCALED(/255)]
        bitNum = eventData[:,0]
        ch1Data = eventData[:,1]
        ch2Data = eventData[:,2]

        # if only waveforms, 
        
        minvalx = 100
        maxvalx = 3000
        
        if(wf_only): 
            # return loc of rising/falling edge of waveform
            ch1_locs = abs(np.diff(ch1Data))>1
            ch2_locs = abs(np.diff(ch2Data))>1

        else:
            plt.xticks(np.linspace(0,4096,5))
            if(chan1): axs[event].plot(bitNum[minvalx:maxvalx],ch1Data[minvalx:maxvalx],'--',label = 'CH1',color='red',alpha=0.5)
            #if(chan1): axs[event].scatter(bitNum[minvalx:maxvalx],ch1Data[minvalx:maxvalx],marker='x',color='red')
            if(chan2): axs[event].plot(bitNum[minvalx:maxvalx],ch2Data[minvalx:maxvalx],'--',label = 'CH2',color='blue',alpha=0.5)
            #if(chan2): axs[event].scatter(bitNum[minvalx:maxvalx],ch2Data[minvalx:maxvalx],marker='x',color='blue')
            axs[event].legend()
            axs[event].set_ylabel('Counts (/255)')
            axs[event].set_title(f"Event {event}")

        ch1MaxVal = np.max(ch1Data, axis=0)

    plt.xlabel('SRAM Memory Loc')
    fig.suptitle(f"{chan1*'CH1'} {chan2*'CH2'} Waveforms over {len(eventNum)} events", fontsize=18, y=0.95)

    plt.savefig("rawdata.png")


# In[113]:

eventNums, data = parse_output(file)

print("plotting histograms")
plot_histograms(data, True, True)

print("plotting waveforms")
plot_waveforms(data, 5, True, True)

