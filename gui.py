import numpy as np 
import re
import pyqtgraph as pg
import time
from PyQt5 import QtCore, QtGui, QtWidgets
import sys
import configparser


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

        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        layout.addItem(spacerItem)

        cboxes.setWidget(w)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, cboxes)

# create plot
#data = readData(); 
#plt = pg.plot(data[:,0], data[:,1], title="Plot", pen='r')
#plt.showGrid(x=True,y=True)

## Start Qt event loop.
if __name__ == '__main__':
    import sys
    #if sys.flags.interactive != 1 or not hasattr(pg.QtCore, 'PYQT_VERSION'):
    #    pg.QtWidgets.QApplication.exec_()
    app = QtWidgets.QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec_())
