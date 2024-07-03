import tkinter as tk
from tkinter import ttk
import threading
import random
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import time
import subprocess
import numpy as np
import datetime
# importing mean() 
from statistics import mean 
import os
# Function to fetch data using netcat command
def fetch_data():
    try:
        result = subprocess.getstatusoutput(f'echo bulk | netcat -w 1 -t localhost 50003')
        lines = result[1].splitlines()
        data = [list(map(float, line[:-1].split(','))) for line in lines]
        return np.array(data), result
    except Exception as e:
        print(f"Error fetching data: {e}")
        return None

class NumberGeneratorPlotter:
    def __init__(self, root):
        self.root = root
        self.root.title("Number Generator and Plotter")
        
        self.currents = [ [] for i in range(12) ]
        self.zero_currents_perch = [ 0.0 ] * 12
        self.time = []
        self.update_intervals = [500] * 12  # Initial update intervals in milliseconds
        self.writing = False
        self.server_up = False
        self.file_writer = None
        self.file_name = tk.StringVar()
        self.file_name.set("OutputFile")
        self.y_min = tk.DoubleVar()
        self.y_max = tk.DoubleVar()
        self.limitset = False
        self.y_min.set(-124.0)
        self.y_max.set(124.0)
        self.draw_y_min = -124.0
        self.draw_y_max = 124.0

        self.t_min = datetime.datetime.now()
        self.t_max = datetime.datetime.now()
        self.create_widgets()
        #if self.server_up:
        #    self.start_updates()
        #self.init_plot()
    
    def create_widgets(self):
        self.labels = []
        self.checkboxes = []
        
        for i in range(12):
            label = tk.Label(self.root, text="Ch {}: {:.5f}".format(i+1, 0.0))
            label.grid(row=i, column=0, padx=10, pady=5, sticky='w')
            self.labels.append(label)
            
            var = tk.IntVar(value=1)
            checkbox = tk.Checkbutton(self.root, text="Update/Drawing", variable=var)
            checkbox.grid(row=i, column=1, padx=10, pady=5, sticky='e')
            self.checkboxes.append(var)

        self.start_button = tk.Button(self.root, text="Start Server", command=self.start_server)
        self.start_button.grid(row=0, column=2, columnspan=1, pady=10)

        self.zero_currents_button = tk.Button(self.root, text="Zero Currents", command=self.zero_currents)
        self.zero_currents_button.grid(row=12, column=1, columnspan=1, pady=10)
        

        self.zero_currents_indicator = tk.Label(self.root, text=" OFF ", bg='red', width=3)
        self.zero_currents_indicator.grid(row=12, column=2, columnspan=1, pady=10)

        # File Name Entry
        ttk.Label(self.root, text="File Name:").grid(row=13, column=0, columnspan=1, pady=10)
        self.file_entry = ttk.Entry(self.root, textvariable=self.file_name, width=30)
        self.file_entry.grid(row=13, column=1, columnspan=1, pady=10)

        # Start/Stop Button
        self.write_file_button = tk.Button(self.root, text="Record Currents", command=self.write_file)
        self.write_file_button.grid(row=13, column=2, columnspan=1, pady=10)

        # Write Status Indicator
        self.status_label = ttk.Label(self.root, text="")
        self.status_label.grid(row=13, column=3, columnspan=2, pady=10)

        # Plotting widget
        self.plot_canvas = plt.Figure(figsize=(8*0.8, 6*0.8), dpi=100)
        self.plot_ax = self.plot_canvas.add_subplot(111)
        self.plot_canvas_widget = FigureCanvasTkAgg(self.plot_canvas, master=self.root)
        self.plot_canvas_widget.get_tk_widget().grid(row=0, column=3, rowspan=12, padx=10, pady=10)
        
        self.toolbar_frame = tk.Frame(master=self.root)
        self.toolbar_frame.grid(row=12, column=3, columnspan=3, padx=10, pady=10)

        self.canvas_toolbar = NavigationToolbar2Tk(self.plot_canvas_widget, self.toolbar_frame)
        # Y Range Entry
        ttk.Label(self.root, text="Y Min:").grid(row=14, column=0, columnspan=1, pady=10)
        self.file_entry = ttk.Entry(self.root, textvariable=self.y_min, width=30)
        self.file_entry.grid(row=14, column=1, columnspan=1, pady=10)
        ttk.Label(self.root, text="Y Max:").grid(row=15, column=0, columnspan=1, pady=10)
        self.file_entry = ttk.Entry(self.root, textvariable=self.y_max, width=30)
        self.file_entry.grid(row=15, column=1, columnspan=1, pady=10)

        # Set Limits Button
        self.set_limits_button = tk.Button(self.root, text="Set Limits", command=self.set_limits)
        self.set_limits_button.grid(row=14, column=2, rowspan=2, columnspan=1, pady=10)

     
    def start_updates(self):
        self.running = True
        self.update_thread = threading.Thread(target=self.update_currents_thread, daemon=True)
        self.update_thread.start()

    def start_server(self):
            self.server_up = True
            result = os.system(". ./start_pa_server.sh")
            #result = subprocess.getstatusoutput(f'source ./start_pa_server.sh')
            #time.sleep(1)
            #print(result)
            if result==0:
                self.start_updates()
                self.init_plot()
        #self.running = False
        #self.root.after_cancel(self.update_id)
        #self.update_thread.join()

    #def stop_updates(self):
    #    self.running = False
    #    self.root.after_cancel(self.update_id)
    #    self.update_thread.join()

    def zero_currents(self):
        for i in range(12):
            self.zero_currents_perch[i] = self.currents[i][-1]
        
        if self.zero_currents_indicator["bg"] == 'red':
            self.zero_currents_indicator.config(bg='green', text='  ON  ')
        else:
            self.zero_currents_indicator.config(bg='red', text=' OFF ')
    
    def write_file(self):
        if not self.writing:
            file_name = "./data/" + self.file_name.get() + "_" + datetime.datetime.now().strftime("%Y_%m_%d_h%H_m%M_s%S")
            try:
                self.file_writer = open(file_name+".txt", 'a')  # Open file in append mode
                self.writing = True
                self.write_file_button.config(text="Stop")
                self.status_label.config(text="Writing to file...")
            except IOError as e:
                self.status_label.config(text=f"Error: {e}")
        else:
            if self.file_writer:
                self.file_writer.close()
                self.file_writer = None
            self.writing = False
            self.write_file_button.config(text="Write")
            self.status_label.config(text="")
                
    def set_limits(self):
        if not self.limitset:
            self.limitset = True
        self.draw_y_min = self.y_min.get()
        self.draw_y_max = self.y_max.get()

    def update_currents_thread(self):
        time_count = 0
        
        while self.running:
            new_data, result = fetch_data()
#            if self.writing:
#                self.file_writer.write(result[1]+"\n")
            channels_data = [ [] for i in range(12) ]
            if new_data is not None:
                last_currents = []
                for data in new_data: 
                    for ch in range(12):
                        channels_data[ch].append(data[ch])
                for i in range(12):
                    ch_current = mean(channels_data[i])-self.zero_currents_perch[i]
                    self.currents[i].append(ch_current)
                    last_currents.append(ch_current)
                    #print(i)
                    if i==0:
                        self.time.append(datetime.datetime.now())
                    if self.checkboxes[i].get() == 1:
                       self.labels[i].config(text="Ch {}: {:.5f}".format(i+1, self.currents[i][-1]))
                if self.writing:
                    #self.currents[:][-1]
                    print(len(last_currents))
                    self.file_writer.write(', '.join(["{:.6f}".format(i) for i in last_currents])+"\n")
            
            # Update plot every second
            if time_count % 1 == 0:
                self.update_plot()
            
            time.sleep(0.5)
            time_count += 0.5
    
    def update_plot(self):
        self.plot_ax.clear()
        self.plot_ax.set_xlim(self.t_min, self.t_max)
        for i in range(12):
            if self.checkboxes[i].get() == 1:
                self.plot_ax.plot(self.time , self.currents[i], label=f'Ch {i+1}')
        self.plot_canvas.autofmt_xdate()
        delta_t = self.time[-1] - self.time[0]
        difference_in_minutes = int(delta_t.total_seconds() / (5*60))
        if self.time[-1]> self.t_max:
            self.t_min = self.time[0] + datetime.timedelta( minutes = 5*difference_in_minutes )        
            self.t_max = self.time[0] + datetime.timedelta( minutes = 5*(difference_in_minutes+1) )

        if self.limitset:
            self.plot_ax.set_ylim(self.draw_y_min, self.draw_y_max)

        self.plot_ax.set_xlabel('Time ')
        self.plot_ax.set_ylabel('I [nA]')
        self.plot_ax.set_title('Currents vs Time')
        #self.plot_ax.set_xlim(self.time[0], self.time[0] + datetime.timedelta( minutes = 5))

        self.plot_ax.legend()
        self.plot_canvas_widget.draw()
    
    def init_plot(self):
        self.plot_ax.set_xlabel('Time ')
        self.plot_ax.set_ylabel('I [nA]')
        self.plot_ax.set_title('Random currents vs Time')
        self.plot_canvas_widget.draw()

if __name__ == "__main__":
    root = tk.Tk()
    app = NumberGeneratorPlotter(root)
    root.mainloop()