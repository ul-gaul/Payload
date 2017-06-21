import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib import animation, style
import serial.tools.list_ports
from serial import Serial
import time as timer
from math import pi
# from rotating_rocket import RocketModel
mpl.rcParams['toolbar'] = 'None'
style.use("seaborn-whitegrid")  # style.use("fivethirtyeight")
# ports = [port for port in serial.tools.list_ports.comports()]
# print(ports)
#
# ser = Serial('COM7', 9600)


class RocketGraphing:
    def __init__(self):
        self.fig, (self.ax1, self.ax2, self.ax3) = plt.subplots(1, 3)
        self.line1, = self.ax1.plot([], [], 'r-', lw=2)
        self.tail1, = self.ax1.plot([], [], color='red', lw=4)
        self.head1, = self.ax1.plot([], [], color='red', marker='o', markeredgecolor='r')
        self.line2, = self.ax2.plot([], [], 'g-', lw=2)
        self.tail2, = self.ax2.plot([], [], color='green', lw=4)
        self.head2, = self.ax2.plot([], [], color='green', marker='o', markeredgecolor='g')
        self.line3, = self.ax3.plot([], [], 'b-', lw=2)
        self.tail3, = self.ax3.plot([], [], color='blue', lw=4)
        self.head3, = self.ax3.plot([], [], color='blue', marker='o', markeredgecolor='b')

        self.lines = [self.line1, self.line2, self.line3, self.tail1, self.tail2, self.tail3, self.head1, self.head2, self.head3]

        for ax in [self.ax1, self.ax2, self.ax3]:
            ax.margins(0.05)
            ax.set_ylabel('Angular velocity [rad/s]')

        self.time, self.w_roll, self.w_pitch, self.w_yaw = [0], [0], [0], [0]

        self.begin = timer.time()
        # def init():
        #     line.set_data(x[:2], y[:2])
        #     return line
        self.anim = animation.FuncAnimation(self.fig, self.animate, interval=10, blit=True)

    def animate(self, i):
        with open('datatmp.txt') as myfile:
            try:
                t, x, y, z = list(myfile)[-1].split('#')
            except ValueError:
                return self.animate(i)
        # # ---- Bluetooth Live Data
        # lineser = str(ser.readline())[2:-5]
        # try:
        #     t, x, y, z = lineser.split('#')  # bits
        # except ValueError:
        #     return self.animate(i)
        rrad = (int(x)/5) * pi/180  # rads / sec
        prad = (512/5 - int(y)/5) * pi/180
        yrad = (512/5 - int(z)/5) * pi/180

        self.time.append(timer.time() - self.begin)  # secs
        self.w_roll.append(rrad)
        self.w_pitch.append(prad)
        self.w_yaw.append(yrad)

        # framemax = 650
        # secmax = 16
        # timemin = max(0, int(time[-1]) - secmax)
        # print(time[-1], len(time))
        # imin = min(range(len(time)), key=lambda k: abs(time[k]-timemin))
        # imin = min(0, time)
        framemax = 670
        imin = 0
        smx = 16
        if self.time[-1] > smx:
            imin = len(self.time) - framemax
        # elif time[-1] > smx and cue != 1:
        #     imin = len(time) - framemax
        #     print("fine")
        xdata = self.time[imin:]
        y1data = self.w_roll[imin:]
        y2data = self.w_pitch[imin:]
        y3data = self.w_yaw[imin:]

        # if time[-1] > secmax:

        self.lines[0].set_data(xdata, y1data)
        self.lines[1].set_data(xdata, y2data)
        self.lines[2].set_data(xdata, y3data)

        self.lines[3].set_data(xdata[-100:], y1data[-100:])
        self.lines[4].set_data(xdata[-100:], y2data[-100:])
        self.lines[5].set_data(xdata[-100:], y3data[-100:])

        self.lines[6].set_data(xdata[-1:], y1data[-1:])
        self.lines[7].set_data(xdata[-1:], y2data[-1:])
        self.lines[8].set_data(xdata[-1:], y3data[-1:])

        for ax in [self.ax1, self.ax2, self.ax3]:
            ax.relim()
            ax.autoscale()
            ax.set_ylim(-2, 4)
            ax.set_xticks([])
        # if max(ydata) < 15:
        #     plt.ylim(0, 15)
            if self.time[-1] < smx:
                ax.set_xlim(0, smx)

        return self.lines


RocketGraphing()
plt.tight_layout()
plt.show()

# plt.plot(time, w_yaw, 'k-', linewidth=1)
# plt.show()
