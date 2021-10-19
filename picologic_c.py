#! /usr/bin/python3

INTERFACE = 0
TIMEOUT=0
BUFFER_SIZE=512

from ctypes import byref, create_string_buffer
import usb1
import struct
import tkinter

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.animation import FuncAnimation
#import psutil
import collections

# Listing of all USB devices
#handle = 0 

def readConf():
   path = './config/calibration.conf'
   amp = []
   bias = []
   with open(path) as fp:
      Lines = fp.readlines()
      for line in Lines:
         words=line.strip().split()
         #print(words)
         amp.append(float(words[3]))
         bias.append(float(words[4]))
   return amp, bias


            
def read_pAs(amp,bias):
   currents = [] 
   data=create_string_buffer(BUFFER_SIZE)

   #channels = (int *)malloc(64); 
   with usb1.USBContext() as context:
      handle = context.openByVendorIDAndProductID(1204, 4099,skip_on_error=False,)
      if handle is None:
         print('Device not present, or user is not allowed to access device.')
      with handle.claimInterface(INTERFACE):
         #print("handle = ", handle)
         # Do stuff with endpoints on claimed interface.
         #while True:
         e = handle._bulkTransfer( 0x82, data, BUFFER_SIZE,TIMEOUT)
         #print('Process data.')
         data_f=0
         k=0
         for i in range(0,511,2):
            c_2bits = struct.unpack('<H', bytes(data[i:i+2]))
            c=c_2bits[0]
            if c==0xfefe :
               data_f=1
               k=0
            
            elif data_f==1 and k<12:
               #print(hex(c))
               temp = int((c<<8|c>>8))&0xffff 
               chn = temp-0x8000
               currents.append(amp[k]*float(chn)-bias[k])
               k+=1
               i+=1
            if k==12:
               data_f=0
               break
      
                     
   return currents 

# function to update the data

if __name__ == '__main__':
   #picologic: Vendor-04b4 Product-1003
   #Reading configuration file
   amp, bias = readConf()
   #Reading currents

   # start collections with zeros
   current_arrays = [collections.deque(np.zeros(100)) for i in range(0,12)]
   time            = collections.deque(np.zeros(100)) 

   # define and adjust figure
   fig, ax = plt.subplots(3, 4, figsize=(12, 6), facecolor='#DEDEDE')

   #fig = plt.figure(figsize=(12,6), facecolor='#DEDEDE')
   #ax = plt.subplot(121)
   #ax1 = plt.subplot(122)
   #ax.set_facecolor('#DEDEDE')
   #ax1.set_facecolor('#DEDEDE')
   #time_f = True
   #run_time = 0
   def updateData(i):
      # get data
      time.popleft()
      time.append(time[-1]+1)
      currents = read_pAs( amp, bias)
      pad_x = 0 
      pad_y = 0
      for ic,current_array in enumerate(current_arrays):
         if pad_x==4:
            pad_x=0
            pad_y+=1
         current_array.popleft()
         current_array.append(currents[ic])
         ax[pad_y,pad_x].cla()
         ax[pad_y,pad_x].plot(time,current_array,alpha=0.8, color='C{}'.format(ic))
         #ax[pad_y,pad_x].plot(time,current_array,'-o',alpha=0.8)#,'-o',alpha=0.8)
         ax[pad_y,pad_x].set_ylim(np.min(current_array)-np.std(current_array),np.max(current_array)+np.std(current_array))
         pad_x+=1
      #current_array_2.popleft()
      #current_array_2.append(currents[1])
      # clear axis
      #ax.cla()
      #ax1.cla()
      # plot cpu
      #ax[0,0].plot(time,current_array,alpha=0.8)#,'-o',alpha=0.8)
      #ax.scatter(len(cpu)-1, cpu[-1])
      #ax.text(time[-1]*1.07,cpu[-1]*1.07, "{}%".format(cpu[-1]))
      #ax.set_ylim(0,100)
      #ax[0,0].set_ylim(np.min(current_array)-np.std(current_array),np.max(current_array)+np.std(current_array))
      #print(np.min(cpu)-np.std(cpu),np.max(cpu)+np.std(cpu))
      # plot memory
      #ax[0,1].plot(time,current_array_2)#,'-o',alpha=0.8)
      #ax[0,1].set_ylim(np.min(current_array_2)-np.std(current_array_2),np.max(current_array_2)+np.std(current_array_2))
      #if run_time==10:
      #   time_f = False
      #run_time+=1

   # animate
   #while time_f:
   ani = FuncAnimation(fig, updateData, interval=0)

   plt.show()

   


