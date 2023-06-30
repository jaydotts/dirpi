import numpy as np 
import re
import pyqtgraph as pg
import time
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QApplication, QDialog, QMainWindow, QPushButton
import sys
import configparser
import subprocess
from os import system

buffer_path = sys.argv[1]

config = configparser.ConfigParser()
config.read('config/config.ini')
plt_window_buff = 10

addr_depth = int(config['data']['memory_depth']) + plt_window_buff

def readData():
    data = np.zeros((addr_depth,3))
    with open(str(buffer_path),'r') as f: 
        lines = f.readlines()
    f.close()
    for i,line in enumerate(lines): 
        line = line.split(',')
        s = [int(s) for s in re.findall(r'-?\d+\.?\d*', line[0])]
        try: 
            data[i,:] = s
        except: 
            pass 
    return data

class MainWindow(QtWidgets.QMainWindow):

    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)

        # separate plots for waveforms and histograms
        # self.layoutWidget = lay
        
        # 
        self.graphWidget = pg.PlotWidget()
        self.setCentralWidget(self.graphWidget)

        self.x = np.arange(addr_depth)
        self.ch1_data = np.zeros(addr_depth)
        self.ch2_data = np.zeros(addr_depth)

        self.graphWidget.setRange(yRange=(0,255))
        self.graphWidget.showGrid(True, True)
        self.setFixedWidth(1000)
        self.setFixedHeight(500)
        self.createDockWindows()

        pen1 = pg.mkPen(color=(255, 255, 0))
        pen2 = pg.mkPen(color=(135, 206, 235))
        self.data1_line = self.graphWidget.plot(self.x, self.ch1_data, pen=pen1)
        self.data2_line = self.graphWidget.plot(self.x, self.ch2_data, pen=pen2)
        
        # arrays for histograms counts
        self.hist_data_1 = np.zeros(256, dtype=int)
        self.hist_data_2 = np.zeros(256, dtype=int)
        
        # remember previous maximum values in order to check whether event is new
        # start at -1 so that the first event is always kept.
        self.last_max_1 = -1
        self.last_max_2 = -1 
        
        # determine x axis for histograms (256 points, match addrr_depth)
        # since we're using step mode, there needs to be one more point in x than there is in y.
        self.hist_x = np.linspace(0, 256, 256 + 1) - 0.5
        # stretch histogram to match x axis of waveform?
        # self.hist_x = self.hist_x * addr_depth / 255
        
        # create histogram plot objects
        self.hist_line_1 = self.graphWidget.plot(
            self.hist_x,
            self.hist_data_1,
            pen=pg.mkPen(color=(215,215, 96)),
            #fillLevel=0.0,
            #fillBrush=(215, 215, 96, 16),
            stepMode="center",
        )
        self.hist_line_2 = self.graphWidget.plot(
            self.hist_x,
            self.hist_data_2,
            pen=pg.mkPen(color=(150,195,210)),
            #fillLevel=0.0,
            #fillBrush=(150, 195, 210, 15),
            stepMode="center",
        )
        
        # 
        self.timer = QtCore.QTimer()
        self.timer.setInterval(1)
        self.timer.timeout.connect(self.update_plot_data)
        self.timer.start()

    def update_plot_data(self):

        #self.x = self.x[1:]  # Remove the first y element.
        #self.x.append(self.x[-1] + 1)  # Add a new value 1 higher than the last.
        data = readData()
        self.ch1_data = data[:,1]
        self.ch2_data = data[:,2]
        
        # get maximum value for each waveform
        max_1 = self.ch1_data.max()
        max_2 = self.ch2_data.max()
        
        # check whether max values have changed.
        # we should skip incrementing the histogram counts if neither value has changed.
        if (max_1 != self.last_max_1) or (max_2 != self.last_max_2):
            update_hists = True
            self.last_max_1 = max_1
            self.last_max_2 = max_2
            # determine which histogram bins should be incremented.
            # enforce that value is in range [0, 255] since there are occasionally values > 255, for currently unknown reasons.
            # - Brunel, 2023-05-18
            bin_1 = min([int(max_1), 255])
            bin_2 = min([int(max_2), 255])
        else:
            update_hists = False


        if(self.c0.checkState()):
            # Update the waveform data.
            self.data1_line.setData(self.x, self.ch1_data)
            # update histogram is event is new
            if update_hists:
                self.hist_data_1[bin_1] += 1
                self.hist_line_1.setData(self.hist_x, self.hist_data_1)
        else: 
            self.data1_line.clear()
            self.hist_line_1.clear()

        if(self.c1.checkState()):
            # Update the waveform data.
            self.data2_line.setData(self.x, self.ch2_data)
            # update histogram if event is new
            if update_hists:
                self.hist_data_2[bin_2] += 1
                self.hist_line_2.setData(self.hist_x, self.hist_data_2)
        else: 
            self.data2_line.clear()
            self.hist_line_2.clear()


    def createDockWindows(self):
        cboxes = QtWidgets.QDockWidget("Menu", self)
        cboxes.setAllowedAreas(QtCore.Qt.LeftDockWidgetArea)

        w = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout()
        w.setLayout(layout)

        self.c0 = QtWidgets.QCheckBox("CH1 (Y)")
        self.c0.toggle()
        layout.addWidget(self.c0)

        self.c1 = QtWidgets.QCheckBox("CH2 (B)")
        layout.addWidget(self.c1)
        self.c1.toggle()

        self.push_hist_reset = QtWidgets.QPushButton("Reset Histogram", self)
        layout.addWidget(self.push_hist_reset)
        self.push_hist_reset.clicked.connect(self.reset_histogram)
        
        #self.trigger = QtWidgets.QPushButton(self)
        #self.trigger.setText("Stop")
        #layout.addWidget(self.trigger)
        #self.trigger.clicked.connect(self.toggle_triggers)

        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        layout.addItem(spacerItem)

        cboxes.setWidget(w)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, cboxes)
    
    def reset_histogram(self):                                                                                     
        self.hist_data_1 = np.zeros(256, dtype=int)
        self.hist_data_2 = np.zeros(256, dtype=int)
    
    def toggle_triggers(self): 
        btn_text = self.trigger.text()
        if btn_text == "Stop":
            subprocess.run(["./dirpi.sh","stop"])
            self.trigger.setText("Start")
        else:
            subprocess.run(["./dirpi.sh","start-soft"])
            self.trigger.setText("Stop")

        

class ConfigDialog(QDialog):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("HELLO!")
        self.layout = QtWidgets.QVBoxLayout()
        self.le = QtWidgets.QLabel(self)
        self.le.move(30, 62)
        self.le.resize(400,22)
        self.showDialog()
        #self.setLayout(self.layout)

    
    def showDialog(self):
        self.first = QtWidgets.QLineEdit(self)
        self.second = QtWidgets.QLineEdit(self)
        buttonBox = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel, self)
        layout = QtWidgets.QFormLayout(self)
        layout.addRow("First text", self.first)
        layout.addRow("Second text", self.second)
        layout.addWidget(buttonBox)
        buttonBox.accepted.connect(self.accept)
        buttonBox.rejected.connect(self.reject)

    def getInputs(self):
        return (self.first.text(), self.second.text())

## Start Qt event loop.
if __name__ == '__main__':
    import sys
    #if sys.flags.interactive != 1 or not hasattr(pg.QtCore, 'PYQT_VERSION'):
    #    pg.QtWidgets.QApplication.exec_()
    app = QtWidgets.QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec_())
