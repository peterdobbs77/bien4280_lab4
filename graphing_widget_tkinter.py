# @author Peter
# Date: 12/20/2017
# adapted from code by David Herzfeld
#

import Tkinter
import threading
import time
import serial

from Tkinter import *  # from tk_pack found online

# Global graphing widget
my_graphing_widget = None
# Global exit flag
exit_flag = False
flag_arduino = False
flag_save = False
flag_alarm = False
# Global serial port
port = serial.Serial()

x1 = "time"


# Create a class which extends gtk.DrawingArea
class GraphingWidget(Tkinter.Canvas):
    # _gsignals = { "expose-event": "override" }

    def __init__(self, master, num_series=1, history=1, y_min=-1, y_max=1, **options):
        """Initialize a new drawing widget. Customizeable parameters:
           1) num_series -- the number of simultaneous plots to display
           1) history -- the history time (in seconds)
           2) y_min -- the minimum y value
           3) y_max -- the maximum y value"""
        Tkinter.Canvas.__init__(self, master, background="black", **options)
        # Make an internal sempahore (mutex)
        self.semaphore = threading.Semaphore(1)

        # Make sure num_series is valid
        try:
            num_series = int(num_series)
        except:
            raise RuntimeError('num_series must be an integer')
        if num_series < 1:
            raise RuntimeError('Cannot have less than 1 series')
        # Make sure time is valid is valid
        try:
            history = float(history)
        except:
            raise RuntimeError('history must be a float')
        if history <= 0:
            raise RuntimeError('Cannot display zero or negative amount of time')
            # Check y_min and y_max
        try:
            y_min = float(y_min)
            y_max = float(y_max)
        except:
            raise RuntimeError('y_min and y_max must be floats')
        if y_min == y_max:
            raise RuntimeError('y_min and y_max cannot be identical')
        # If y_max < y_min, swap the values
        if y_max < y_min:
            temp = y_max
            y_max = y_min
            y_min = temp
            # Save the necessary values
        self.num_series = num_series
        self.history = history
        self.y_max = y_max
        self.y_min = y_min

        # Create empty FIFO queues
        self.enabled = []
        self.t_values = []
        self.y_values = []
        self.line_colors = []
        self.line_widths = []
        for i in range(0, self.num_series):
            # Create the empty lists (i.e. a list of lists)
            self.t_values.append([])
            self.y_values.append([])
            self.enabled.append(True)  # All plots enabled by default
            self.line_colors.append("green")  # Default color is green
            self.line_widths.append(1)  # Default width is 1 pixel

        # Connect the expose event
        self.bind("<Expose>", self.expose_event)

        # Setup widget to animate every 1/24th of a second
        self.animate()

        titleframe = Frame(master)
        titleframe.pack(side="top")
        labelt = Label(titleframe, text="Ultrasonic Sensor GUI", relief=RAISED, bg="white", fg="blue")
        labelt.pack(fill=X)

        frame = Frame(master)
        frame.pack()
        self.arduinoButton = Button(frame, text="Connect Arduino", bg="green", command=self.toggleArduino)
        self.arduinoButton.pack(side=LEFT)
        self.saveButton = Button(frame, text="Save Data", bg="blue", command=self.quickSave)
        self.saveButton.pack(side=LEFT)
        self.alarmButton = Button(frame, text="No Alarm", bg="white", command=self.alarmEvent)
        self.alarmButton.pack(side=LEFT)
        self.quitbutton = Button(frame, text="QUIT", bg="red", command=frame.quit)
        self.quitbutton.pack(side=RIGHT)

        frame4 = Frame(master)
        frame4.pack(side="left", anchor="w")
        labeld = Label(frame4, text="Distance-cm", relief=RAISED, fg="black", wraplength=1)
        labeld.pack(side="bottom", fill=X)

        frame5 = Frame(master)
        frame5.pack(side="bottom")
        labeld = Label(frame5, text="Time (Sec)", relief=RAISED, fg="black")
        labeld.pack(side="bottom", fill=X)

        frame2 = Frame(master)
        frame2.pack(side="left", anchor="nw")
        labele = Label(frame2, text="200", relief=RAISED, bg="white")
        labele.pack(side="top", fill=X)

        frame3 = Frame(master)
        frame3.pack(side="left", anchor="sw")
        labeld = Label(frame3, text="0", relief=RAISED, bg="white")
        labeld.pack(side="bottom", fill=X)

    def toggleArduino(self):
        global flag_arduino
        global flag_alarm
        global port

        if self.arduinoButton.config('text')[-1] == 'Connect Arduino':
            self.arduinoButton.config(text='Disconnect Arduino', bg="red")
            port = serial.Serial(port='com3',
                                 baudrate=38400,
                                 bytesize=serial.EIGHTBITS,
                                 stopbits=serial.STOPBITS_ONE,
                                 timeout=1)
            flag_arduino = True
        else:
            self.arduinoButton.config(text='Connect Arduino', bg="green")
            flag_arduino = False
            port.close()

    def write_slogan(self):
        print("Tkinter is easy to use!")

    def quickSave(self):
        global flag_save
        flag_save = True
        print("Data Saved!")

    def alarmEvent(self):
        if flag_alarm == TRUE:
            self.alarmButton.config(text='Keep Calm')
            self.alarmButton.config(bg='red')
        else:
            self.alarmButton.config(text='No Alarm')
            self.alarmButton.config(bg='white')

    def disable_series(self, series=0):
        """Disable a series"""
        try:
            series = int(series)
        except:
            raise RuntimeError('Series must be an integer')
        if series < 0 or series >= self.num_series:
            raise RuntimeError('Invalid series number in disable_series')
        self.enabled[series] = False

    def enabled_series(self, series=0):
        """Enabled a series"""
        try:
            series = int(series)
        except:
            raise RuntimeError('Series must be an integer')
        if series < 0 or series >= self.num_series:
            raise RuntimeError('Invalid series number in enabled_series')
        self.enabled[series] = True

    def animate(self):
        self.plot_tk()
        self.update_idletasks()
        # Call to animate faster than 10 fps
        self.master.after(10, self.animate)

    def expose_event(self, event):
        self.plot_tk()

    def plot_tk(self):
        # Delete all previously plotted items
        self.delete("all")

        # Get width and height
        width = self.winfo_width()
        height = self.winfo_height()

        # Store the base time
        base_time = time.time()

        # Pop all unnecessary values (OLD) off of each queue
        self.semaphore.acquire()
        for i in range(0, self.num_series):
            while len(self.t_values[i]) > 0 and \
                    (self.t_values[i][0] + self.history < base_time):
                self.t_values[i] = self.t_values[i][1:]
                self.y_values[i] = self.y_values[i][1:]  # Remove 0th value
            # Make sure that x and y values are the same length
            if len(self.t_values[i]) != len(self.y_values[i]):
                raise RuntimeError('Invalid lengths for t and y values')
        self.semaphore.release()
        # Create the translation from pixels to coordinates
        # (0, y_min) ----------- (history, y_min)
        #   |                  |
        #   |                  |
        # (0, y_max) ----------- (history, y_max)

        # Lambda function (given width & height)
        to_y_coordinate = lambda y: height - (y - self.y_min) * height / (self.y_max - self.y_min)
        to_t_coordinate = lambda t: width - (base_time - t) * width / self.history

        # Start to plot each series
        for i in range(0, self.num_series):
            # Check if we are enabled
            if self.enabled[i] == False:
                continue  # Move onto next plot
            line_coords = []
            for j in range(0, len(self.y_values[i])):
                # cairo_context.line_to(to_t_coordinate(self.t_values[i][j]),
                #    to_y_coordinate(self.y_values[i][j]))
                line_coords.append(to_t_coordinate(self.t_values[i][j]))
                line_coords.append(to_y_coordinate(self.y_values[i][j]))
            if len(line_coords) >= 4:
                self.create_line(*tuple(line_coords), fill=self.line_colors[i],
                                 width=self.line_widths[i])

    def change_series_color(self, color, series=0):
        """Change the color of a series"""
        try:
            series = int(series)
        except:
            raise RuntimeError('Series must be an integer')
        if series < 0 or series >= self.num_series:
            raise RuntimeError('Invalid series number in change_series_color')
        self.line_colors[series] = color

    def change_series_width(self, width, series=0):
        """Change the width of a given series"""
        try:
            series = int(series)
        except:
            raise RuntimeError('Series must be an integer')
        if series < 0 or series >= self.num_series:
            raise RuntimeError('Invalid series number in change_series_color')
        self.line_widths[series] = width

    def add_y_value(self, value, series=0):
        """Adds a new y-value to the given series"""
        # Make sure value is a float
        try:
            value = float(value)
        except:
            raise RuntimeError('Y-values must be floats')
        try:
            series = int(series)
        except:
            raise RuntimeError('Series must be an integer')
        if series < 0 or series >= self.num_series:
            raise RuntimeError('Invalid series number in add_y_value')
        # Append the necessary values to the queue (FIFO)
        self.semaphore.acquire()
        self.y_values[series].append(value)
        self.t_values[series].append(time.time())  # The current time
        self.semaphore.release()


# New process starts here which periodically returns a new value of
def get_serial():
    import sys
    import math

    # defining all flags
    global flag_alarm
    global flag_arduino
    global flag_save
    global x1
    global port

    i = 0
    j = 0
    dataArray = []
    stringarray = []
    timeArray = []

    xl1 = StringVar()
    labela = Label(root, textvariable=xl1, relief=RAISED, bg="white")
    labela.pack(side="right")
    labela.pack()

    # xl2 = StringVar()
    # labelb = Label(root, textvariable=xl2, relief=RAISED, bg="white")
    # labelb.pack(side="top")
    # labelb.pack()

    xl3 = StringVar()
    labelc = Label(root, textvariable=xl3, relief=RAISED, bg="white")
    labelc.pack(side="left")
    labelc.pack(padx=50)

    startTime = time.time() - 3

    alarmthreshold = 50

    # port = serial.Serial(port='com3',
    #                     baudrate=38400,
    #                     bytesize=serial.EIGHTBITS,
    #                     stopbits=serial.STOPBITS_ONE,
    #                     timeout=1)

    while exit_flag == False:
        if flag_arduino:
            ultrasonicdata = port.read(1)
            if len(ultrasonicdata) != 0:
                if ultrasonicdata == '\n':
                    port.read(1)  # reading '\r' character to prevent that character being read
                    j = 0
                    datapoint = ""
                    for k in range(0, len(stringarray)):
                        datapoint = datapoint + stringarray[k]
                    del stringarray[:]
                    if flag_arduino:
                        sys.stdout.write(datapoint)
                        sys.stdout.write('\n')
                        my_graphing_widget.add_y_value(int(datapoint), series=0)
                        dataArray.append(int(datapoint))

                        xl1time = int(time.time() - startTime)
                        xl1.set(str(xl1time) + " Sec")
                        xl3.set(str(xl1time - 5) + " Sec")
                        timeArray.append(xl1time)

                        if int(datapoint) < alarmthreshold:
                            flag_alarm = True
                            print("Alarm Occured!")
                            my_graphing_widget.alarmEvent()
                        else:
                            flag_alarm = False
                            my_graphing_widget.alarmEvent()
                    i = (i + math.pi / 128) % (2.0 * math.pi)
                else:
                    stringarray.append(str(ultrasonicdata))
            if flag_save:
                out = open('UltrasonicData.csv', 'w')
                for l in range(0, len(dataArray)):
                    out.write('%d, ' % dataArray[l])
                    out.write('%d' % timeArray[l])
                    out.write('\n')
                out.close()
    port.close()


if __name__ == '__main__':
    root = Tkinter.Tk()
    frame = Tkinter.Frame(root, width=300, height=300)
    frame.pack(fill=Tkinter.BOTH, expand=Tkinter.YES)
    my_graphing_widget = GraphingWidget(frame, num_series=3, history=5, y_min=0, y_max=255)
    my_graphing_widget.pack(fill=Tkinter.BOTH, expand=Tkinter.YES)
    my_graphing_widget.change_series_color("cyan", 0)
    my_graphing_widget.change_series_width(3, 0)

    # Create a new thread to add data
    t = threading.Thread(target=get_serial)
    t.setDaemon(True)  # It is a daemon
    t.start()

    root.mainloop()

    exit_flag = True  # Set the exit flag
    t.join()
