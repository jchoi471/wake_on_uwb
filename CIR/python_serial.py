import serial
import ast
import matplotlib.pyplot as plt
import math
from drawnow import *

plt.ion()

ser = serial.Serial('/dev/ttyACM0',115200) #Change the serial port name to the one the board is connected to on the device
f = open('serial.out',"a")

global new_y

def makeGraph():
    plt.title('CIR')      #Plot the title
    plt.ylabel('Magnitude')                            #Set ylabels
    plt.plot(new_y)       #plot the temperature

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
        print("Error reading line")
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

ser.close()
f.close()
