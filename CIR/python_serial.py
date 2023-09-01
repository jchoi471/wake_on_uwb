import serial
import ast
import matplotlib.pyplot as plt
import math
import numpy as np
import time
from drawnow import *

# x = np.linspace(0, 10, 96)
# y = np.zeros(96)

plt.ion()

# figure, ax = plt.subplots(figsize=(10, 8))
# line1, = ax.plot(x, y)

# plt.title("CIR", fontsize=20)

# plt.xlabel("X-axis")
# plt.ylabel("Y-axis")

ser = serial.Serial('/dev/ttyACM0',115200)
f = open('serial.out',"a")

global new_y

def makeGraph():
    plt.title('CIR')      #Plot the title
    # plt.grid(True)                                  #Turn the grid on
    plt.ylabel('Magnitude')                            #Set ylabels
    plt.plot(new_y)       #plot the temperature
    # plt.legend(loc='upper left')                    #plot the legend   

while True:
    while (ser.inWaiting()==0): #Wait here until there is data
        pass #do nothing
    try:
        line = ser.readline()
        line = line.decode()
        line = line.rstrip()
        print(line)
        f.write(line)
        f.write('\n')

    except:
        print("Keyboard Interrupt")
        break
    if (line[0:3] == "CIR"):
        line = "[" + line[11:-1] + "]\n"
        print("printing line")
        print(line)
        try:
            packet = ast.literal_eval(line)
        except:
            continue
        print("PRINTING PACKET")
        print(packet)
        new_y = []
        max_magnitude = 0
        max_index = 0
        for index, i in enumerate(packet):
            mag = math.sqrt(i[1]**2 + i[2]**2)
            new_y += [ mag ]
            if(max_magnitude < mag):
                max_magnitude = mag
                max_index = index

        print("PRINTING MAGNITUDES")
        
        #Fixing the location of the first peak on the graph
        new_y = new_y[(max_index - 10):]
        

        #Normalisation of the graph
        for i in range(len(new_y)):
            new_y[i] = (new_y[i]/max_magnitude) * 1000

        print(new_y)

        drawnow(makeGraph)
        plt.pause(.000001)

        # line1.set_xdata(x)
        # line1.set_ydata(new_y)
        # figure.canvas.draw()
        # figure.canvas.flush_events()
        # time.sleep(0.1)

ser.close()
f.close()
