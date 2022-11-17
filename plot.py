import numpy as np 
import re
import matplotlib.pyplot as plt
import os 

########################### params
# target filename
file = "outputfile.txt"

# max number of plots to display
maxPlotNum = 10

# toggle plotting only the waveforms (skip empty space) 
wf_only = False

# choose channels to plot
chan1 = True
chan2 = True

################################ parse data out of txt file 
with open(file,'r') as f: 
    lines = f.readlines()
f.close()

# Data format: 
# [EVENTNUM, BITNUM(/4096), CH1SCALED(/255), CH2SCALED(/255)]

data = np.zeros((len(lines),4))

for i,line in enumerate(lines): 
    line = line.split(',')
    s = [int(s) for s in re.findall(r'-?\d+\.?\d*', line[0])]
    data[i,:] = s

# get the eventNums back out into an iterable list 
eventNum = [int(x) for x in (np.unique(data[:,0]))]

############################### plot functions 

if(len(eventNum) > maxPlotNum):
    print(f"Plotting only first {maxPlotNum} events") # make this into a log
    eventNum = eventNum[:maxPlotNum]

fig, axs = plt.subplots(nrows=len(eventNum), figsize=(15, 12), sharex = not wf_only)
[ax.grid() for ax in axs]
plt.subplots_adjust(hspace=0.5)

for event in eventNum: 
    
    # select only 4096 bits for this eventNum
    # also index out the eventNum to clean up the data
    eventData = data[data[:,0]==event][:,1:] 

    # now the format is 
    # [BITNUM(/4096), CH1SCALED(/255), CH2SCALED(/255)]
    bitNum = eventData[:,0]
    ch1Data = eventData[:,1]
    ch2Data = eventData[:,2]

    # if only waveforms, 
    
    if(wf_only): 
        # return loc of rising/falling edge of waveform
        ch1_locs = abs(np.diff(ch1Data))>1
        ch2_locs = abs(np.diff(ch2Data))>1
    
    else:
        plt.xticks(np.linspace(0,4096,5))
        if(chan1): axs[event].plot(bitNum,ch1Data,label = 'CH1')
        if(chan2): axs[event].plot(bitNum,ch2Data,label='CH2')
        axs[event].legend()
        axs[event].set_ylabel('Counts (/255)')
        axs[event].set_title(f"Event {event}")
    
plt.xlabel('SRAM Memory Loc')
fig.suptitle(f"{chan1*'CH1'} {chan2*'CH2'} Waveforms over {len(eventNum)} events", fontsize=18, y=0.95)

plt.savefig("plot.png")

