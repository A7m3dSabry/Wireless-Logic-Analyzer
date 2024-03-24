import random
import socket

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button, Slider
import threading
import time

from CustomAxes import *

###############################
# Parameters

channelsNumber = 4
captureMax = 0xFFFF
timerStepTime = 1 / 1e6
HOST = '0.0.0.0'
PORT = 500

###############################


###############################
matplotlib.use('QtAgg')

# Channel Storage
x_vals = np.zeros((2,))
y_vals = np.zeros((2, channelsNumber))

# This is Just an offset for Drawing different channels
channelDrawingOffset = np.arange(0, channelsNumber * 2, 2)

# A Flag for Start Drawing
start_drawing = False

# defines the shown Window in the plot (used in magnification and moving)
window = np.array([0., 1.])

# A Flag used to choose if
moveFreelyFlag = CustomFlag(False)

# lock for synchronizing between drawing and receiving data
lk = threading.RLock()

newTime = 0


####################
# Tasks

def serverTask():
    global start_drawing, x_vals, y_vals, lk
    print("starting server")
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(('0.0.0.0', PORT))
    server.listen(1)
    buff = []

    packetStart = b'@'
    packetEnd = b'#'
    initPacket = b'!'

    while 1:
        try:
            # waiting for connection
            conn, adrs = server.accept()

            print(adrs)
            print("waiting for data")

            while True:
                buff = conn.recv(1)

                if buff == b'':
                    # Connection is Closed so Break to Receive new Connection
                    print("Connection Closed")
                    break
                    pass
                elif buff == packetStart:
                    # new edge detected

                    # Receive Complete Packet
                    buff = conn.recv(12)

		    # parse incomming packet
                    channel = int.from_bytes(buff[:2], 'little')
                    state = int.from_bytes(buff[2:4], 'little')
                    counter = int.from_bytes(buff[4:8], 'little')
                    old_capture = int.from_bytes(buff[8:10], 'little')
                    new_capture = int.from_bytes(buff[10:12], 'little')
                    
		    # Calculate duration
                    duration = ((new_capture - old_capture) + captureMax * counter) * timerStepTime
		    
		    # Acquire Lock to prevent error in drawing while editing
                    lk.acquire()
                    x_vals[-1] = x_vals[-2] + duration
                    y_vals[-1, channel] = state

                    x_vals = np.append(x_vals, x_vals[-1])
                    y_vals = np.append(y_vals, y_vals[-1].reshape(1, -1), axis=0)
                    y_vals[-1, channel] = 0 if state == 1 else 1
		    
		    # Release the Lock
                    lk.release()
                    start_drawing = True
                    pass
                elif buff == initPacket:
                    buff = conn.recv(2)
                    # this is the Initialization Packet
                    # it has the current state of the Channels
                    channel = int.from_bytes(buff[:1], 'little')
                    y_vals[-1, channel] = int.from_bytes(buff[1:], 'little')
                    start_drawing = True

        except Exception as err:
            print(f"ERROR 2 {err}")
            pass


def drawingTask(i):
    # Globalizing variable to Tell Python We Want to Change The Vars Outside
    global x_vals, y_vals, newTime, oldTime, window

    oldTime = newTime
    newTime = time.time_ns()
    if not start_drawing:
        return

    # Acquiring Lock Since We Need To Modify The x_vals values for Extension Simulation
    # the x_vals is Shared Between ServerManager and This Function
    # brief: this lock to prevent Race between this function and ServerManager
    lk.acquire(blocking=True)

    # This Part For Emulating the Extension Of The Graph
    #

    x_vals[-1] = x_vals[-1] + (newTime - oldTime) * 1e-9

    # Clear The Graph
    axes.cla()
    # Draw the Values
    linse = axes.step(x_vals, y_vals + channelDrawingOffset)

    # Release the Lock to allow Server Manager to Add new Values
    lk.release()

    # Define the start and the end x-axis window of the graph to show
    if moveFreelyFlag.getState():
        axes.set_xlim(window)
    else:
        axes.set_xlim(x_vals[-1] - (window[1] - window[0]), x_vals[-1])
        pass

    # Define the start and the end y-axis window of the graph to show
    axes.set_ylim(channelDrawingOffset[0] - 0.5, channelDrawingOffset[-1] + 1.5)

    fig.canvas.draw_idle()
    fig.canvas.flush_events()

    # return lines


####################


####################

# Configuring Plot Parameters

# creating new plot
fig = plt.figure()

# registering custom axes
matplotlib.projections.register_projection(CustomAxes)

# remove un-needed actions from plot figure except save action
toolbar = plt.get_current_fig_manager().toolbar
for action in toolbar.actions():
    if action.text() != 'Save':
        toolbar.removeAction(action)

# force figure to be in Pan mode (to allow moving)
fig.canvas.manager.toolbar.pan()

# creating axes
axes = fig.add_subplot(111, projection=CustomAxes.name)

# hiding legends (because it makes the drawing too lagy)
axes.get_xaxis().set_visible(False)
axes.get_yaxis().set_visible(False)

# register flag to enable moving
axes.registerFlag(moveFreelyFlag)

# register window variable
axes.registerPoints(window)

# enable zooming with mouse wheel
axes.enableZooming()


# Register Live Button
def buttonLive_clicked(e):
    moveFreelyFlag.setState(False)
    pass


buttonLive = Button(fig.add_axes([0.8, 0.9, 0.1, 0.075]), 'Live')
buttonLive.on_clicked(buttonLive_clicked)

####################


# start Server Thread
serverThread = threading.Thread(target=serverTask)
serverThread.start()

# start animation
animationFunction = FuncAnimation(fig, drawingTask, interval=1e-3, frames=1)
plt.show()
