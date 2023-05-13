import numpy as np 
import re
import pyqtgraph as pg
import time
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QApplication, QDialog, QMainWindow, QPushButton
import sys
import configparser
import subprocess


config = configparser.ConfigParser()
config.read('config/config.ini')
plt_window_buff = 10

addr_depth = int(config['data']['memory_depth']) + plt_window_buff

def readData():
    data = np.zeros((addr_depth,3))
    with open("plot_buffer.txt",'r') as f: 
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

        if(self.c0.checkState()):
            self.data1_line.setData(self.x, self.ch1_data)  # Update the data.
        else: 
            self.data1_line.clear()

        if(self.c1.checkState()):
            self.data2_line.setData(self.x, self.ch2_data)  # Update the data.
        else: 
            self.data2_line.clear()


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

        self.push_config = QtWidgets.QPushButton("Configure", self)
        layout.addWidget(self.push_config)
        self.push_config.clicked.connect(self.open_configs)
        
        #self.trigger = QtWidgets.QPushButton(self)
        #self.trigger.setText("Stop")
        #layout.addWidget(self.trigger)
        #self.trigger.clicked.connect(self.toggle_triggers)

        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        layout.addItem(spacerItem)

        cboxes.setWidget(w)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, cboxes)
    
    def open_configs(self):                                                                                     
        dlg = ConfigDialog()
        dlg.setWindowTitle("HELLO!")
        dlg.exec()
    
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
