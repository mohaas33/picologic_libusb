#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <iomanip>

#include <libusb-1.0/libusb.h>

#include "pa_utilities.h"

#define MAX_LINE_LENGTH 80

#define BULK_EP_OUT     0x82    
#define BULK_EP_IN      0x08  

using namespace std;

//  Some (all) of these may be necessary in multiple contexts
//  Global namespace makes it easy...
struct libusb_device_handle *handle = NULL;
struct libusb_device_handle *hDevice_expected = NULL;    
struct libusb_device *dev;
struct libusb_device *dev_expected;    
struct libusb_endpoint_descriptor *epdesc;    
struct libusb_interface_descriptor *intdesc;    


std::string communicate_pAs_v(double *amp, double *bias, int BUFFER_SIZE, int format)
{
  //  NOTE:  This routine assumes that the handle in global namespace is valid.
  
  vector<float> currents_out;
  unsigned char *my_string;
  unsigned short *data_in;
  int *channels;
  
  int transferred = 0;    
  int received = 0;    
  int length = 0; 
  int temp=0;
  int flaga=0,flagb=0,sync_a=0,k=0;   
  my_string = (unsigned char *)malloc(BUFFER_SIZE);    
  channels = (int *)malloc(64);    
  length=2;
  sync_a=0;
  
  // We will average over multiple readouts...
  currents_out.clear();
  
  // "currents" are the average of all "num" measurements
  // "readings" are the individual measurements.
  float currents[12], num[12];
  for (int i=0; i<12; i++)
    {
      currents[i] = 0;
      num[i] = 0;
    }
  
  char   reading[500];
  string readings;
  readings.clear();
  
  char   raw[500];
  string raws;
  raws.clear();
  
  float prior_chn[12];
  
  //cout << "Ready for Bulk Transfer" << endl;
  int e = libusb_bulk_transfer(handle,BULK_EP_OUT,my_string,BUFFER_SIZE,&received,0);     
  //cout << "Completed for Bulk Transfer" << endl;
  if(e==0)
    {    
      data_in = ((unsigned short*) my_string);
      for(int j=2; j<(BUFFER_SIZE-52)/2; j++)
	{
	  if(data_in[j]==0xFEFE && sync_a==0)
	    {
	      sync_a=1;
	    }
	  else if(sync_a==1 && data_in[j]!=0xFEFE)
	    {
	      for (int k=0; k<12; k++)
		{
		  if (data_in[j]!=0xFEFE)
		    {
		      temp=(int)(data_in[j]<<8|data_in[j]>>8)&0xffff;
		      //cout << " " << std::hex <<temp;
		      float chn=temp-0x8000;
		      prior_chn[k] = chn;
		      float cal = amp[k]*chn-bias[k];
		      currents[k] = currents[k] + cal;
		      num[k]++;
		      
		      // here we store as a massive string the entirety of the readout...
		      sprintf(reading,"%f,",cal);
		      readings.append(reading);
		      
		      sprintf(raw,"%d,",int(chn));
		      raws.append(raw);
		    }
		  else
		    {
		      float chn = prior_chn[k];
		      float cal = amp[k]*chn-bias[k];
		      currents[k] = currents[k] + cal;
		      num[k]++;
		      
		      // here we store as a massive string the entirety of the readout...
		      sprintf(reading,"%f,",cal);
		      readings.append(reading);
		      
		      sprintf(raw,"%d,",int(chn));
		      raws.append(raw);
		    }
		  j++;
		}
	      readings.append("\n");
	      raws.append("\n");
	      //cout << endl;
	      sync_a = 0;
	    }
	}
      for (int i=0; i<12; i++)
	{
	  if (num[i]>0)
	    {
	      currents_out.push_back(currents[i]/num[i]);
	    }
	  else
	    {
	      currents_out.push_back(0.0);
	    }
	}
    }
  else
    {
      printf("\nReceived: %d %d\n", e, length);  
    }
  
  //cout << num[0] << endl;
  
  char blah[5000];
  if (format == 0)
    {
      sprintf(blah,"%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",currents_out[0],currents_out[1],currents_out[2],currents_out[3],currents_out[4],currents_out[5],currents_out[6],currents_out[7],currents_out[8],currents_out[9],currents_out[10],currents_out[11]);
        return string(blah);
    }

  if (format == 1)
    {
        return readings;
    }

  return raws;
  
}

int pa_connect()
{
  cout << "Here we shall connect to the USB port" << endl;
  // Init libusb
  int r = libusb_init(NULL);    
  if(r < 0)    
  {    
    printf("\nfailed to initialise libusb\n");    
    return 1;    
  }    
  else
    printf("\nInit Successful!\n");    


  // Get the list of available USB devices    
  struct libusb_device **devs;    
  ssize_t cnt = libusb_get_device_list(NULL, &devs);    
  if (cnt < 0)    
  {    
      printf("\nThere are no USB devices on bus\n");    
      return -1;    
  }    
  printf("\nDevice Count : %d\n-------------------------------\n",cnt);    


  // Now search the device list for the pa using the "Vendor" and "Product" codes
  int e = 0,config2;    
  int ind = 0,index,j=0;    
  char str1[64], str2[64];    
  char found = 0;    
  for (int k=0;k<cnt;k++)
    {    
      //  Get the description of the kth device
      dev = devs[k];
      struct libusb_device_descriptor desc;    
      r = libusb_get_device_descriptor(dev, &desc);    
      if (r < 0)
	{
	  printf("failed to get device descriptor\n");    
	  libusb_free_device_list(devs,1);    
	  //  ??  libusb_close(handle);    
	  break;    
	}    

      //  Print the description
      printf("\nDevice Descriptors: ");    
      printf("\n\tVendor ID : %x",desc.idVendor);    
      printf("\n\tProduct ID : %x",desc.idProduct);    
      printf("\n\tSerial Number : %x",desc.iSerialNumber);    
      printf("\n\tSize of Device Descriptor : %d",desc.bLength);    
      printf("\n\tType of Descriptor : %d",desc.bDescriptorType);    
      printf("\n\tUSB Specification Release Number : %d",desc.bcdUSB);    
      printf("\n\tDevice Release Number : %d",desc.bcdDevice);    
      printf("\n\tDevice Class : %d",desc.bDeviceClass);    
      printf("\n\tDevice Sub-Class : %d",desc.bDeviceSubClass);    
      printf("\n\tDevice Protocol : %d",desc.bDeviceProtocol);    
      printf("\n\tMax. Packet Size : %d",desc.bMaxPacketSize0);    
      printf("\n\tNo. of Configuraions : %d\n",desc.bNumConfigurations);    

      //  Check to see if the device is a pa...if so, open it and assign the handle...
      if(desc.idVendor == 0x04b4 && desc.idProduct == 0x1003)
	{
	  // Gotcha
	  found = 1;

	  // Get the handle...
	  e = libusb_open(dev,&handle);    
	  printf("return value of opening : %d %s \n", e,libusb_error_name(e));
	  if (e < 0)
	    {    
	      printf("error opening device\n");    
	      libusb_free_device_list(devs,1);    
	      libusb_close(handle);    
	      break;    
	    }

	  //  Get the Manufacturer...
	  e = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));    
	  if (e < 0)
	    {
	      libusb_free_device_list(devs,1);    
	      libusb_close(handle);    
	      continue;    
	    }    
	  printf("\nManufactured : %s",str1);    
	  
	  
	  //  Get the Product
	  e = libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));    
	  if(e < 0)
	    {    
	      libusb_free_device_list(devs,1);    
	      libusb_close(handle);    
	      continue;    
	    }    
	  
	  
	  
	  printf("\nProduct : %s",str2);    
	  printf("\n----------------------------------------");    
        break;    
	}
      else
	{
	  continue;
	}   
      
    }//end of for loop

    
  // So...was the pa found???
  if(found == 0)
    {    
      printf("\nDevice NOT found\n");    
      libusb_free_device_list(devs,1);    
      libusb_close(handle);    
      return 1;    
    }
  else
    {
      printf("\nDevice found\n");    
      dev_expected = dev;    
      hDevice_expected = handle;    
    }    
  
  //  Read and print the configuration
  e = libusb_get_configuration(handle,&config2);    
  if(e!=0)
    {      
      printf("\n***Error in libusb_get_configuration\n");    
      libusb_free_device_list(devs,1);    
      libusb_close(handle);    
      return -1;    
    }    
  printf("\nConfigured value : %d",config2);    
  if(config2 != 1)
    {
      libusb_set_configuration(handle, 1);    
      if(e!=0)    
	{    
	  printf("Error in libusb_set_configuration\n");    
	  libusb_free_device_list(devs,1);    
	  libusb_close(handle);    
	  return -1;    
	}    
      else    
        printf("\nDevice is in configured state!");    
    }    
  

  // check if the kernel driver is OK
  libusb_free_device_list(devs, 1);  
  if(libusb_kernel_driver_active(handle, 0) == 1)    
  {    
    printf("\nKernel Driver Active");    
    if(libusb_detach_kernel_driver(handle, 0) == 0)    
      printf("\nKernel Driver Detached!");    
    else{    
      printf("\nCouldn't detach kernel driver!\n");    
      libusb_free_device_list(devs,1);    
      libusb_close(handle);    
      return -1;    
    }    
  }    

  //  Claim the interface as our own...
  e = libusb_claim_interface(handle, 0);    
  if(e < 0){    
    printf("\nCannot Claim Interface");    
    libusb_free_device_list(devs,1);    
    libusb_close(handle);    
    return -1;    
  }    
  else    
    printf("\nClaimed Interface\n");

  return 0;
}

void readCalibration(double *amp, double *bias)
{
  //File with calibration of each pA channel
  char path[] = "/home/daq/Work/picoammeters/picologic_libusb/config/calibration_bnl.conf";
  int start_line = 0;
  
  char line[MAX_LINE_LENGTH] = {0};
  unsigned int line_count = 0;
  
  /* Open file */
  FILE *file = fopen(path, "r");
  
  if (!file)
    {
      perror(path);
    }
  
  /* Get each line until there are none left */
  while (fgets(line, MAX_LINE_LENGTH, file))
    {
      ++line_count;
      if (line[strlen(line) - 1] != '\n')
	printf("\n");
      int init_size = strlen(line);
      char delim[] = " ";
      
      int wordN = 0;
      char *ptr = strtok(line, delim);
      while(ptr != NULL)
	{
	  if(wordN==3){
	    amp[line_count-1]=atof(ptr);
	  }
	  if(wordN==4){
	    bias[line_count-1]=atof(ptr);
	  }
	  ptr = strtok(NULL, delim);
	  wordN++;
	}
      
      
    }
  
  /* Close file */
  if (fclose(file))
    {
      perror(path);
    }
}

