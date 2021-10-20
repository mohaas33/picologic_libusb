#include <stdio.h>    
#include <time.h>
#include <stdlib.h>   
#include <stdbool.h> 
#include <sys/types.h>    
#include <string.h>    
//#include </home/aw325/Downloads/libusb/libusb/libusb.h>    

#include <libusb-1.0/libusb.h>

#include <sys/time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

int verbocity=0;

void readConfiguration(double *amp, double *bias);

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

#define BULK_EP_OUT     0x82    
#define BULK_EP_IN      0x08    

bool savefile = true;
char filename[] = "data.txt";
float sleeptime = 1.0; // unit in seconds

int interface_ref = 0;    
int alt_interface,interface_number;    

int print_configuration(struct libusb_device_handle *hDevice,struct libusb_config_descriptor *config)    
{    
    char *data;    
    int index;    

    data = (char *)malloc(512);    
    memset(data,0,512);    

    index = config->iConfiguration;    

    libusb_get_string_descriptor_ascii(hDevice,index,data,512);    

    printf("\nInterface Descriptors: ");    
    printf("\n\tNumber of Interfaces : %d",config->bNumInterfaces);    
    printf("\n\tLength : %d",config->bLength);    
    printf("\n\tDesc_Type : %d",config->bDescriptorType);    
    printf("\n\tConfig_index : %d",config->iConfiguration);    
    printf("\n\tTotal length : %lu",config->wTotalLength);    
    printf("\n\tConfiguration Value  : %d",config->bConfigurationValue);    
    printf("\n\tConfiguration Attributes : %d",config->bmAttributes);    
    printf("\n\tMaxPower(mA) : %d\n",config->MaxPower);    

    free(data);    
    data = NULL;    
    return 0;    
}    

struct libusb_endpoint_descriptor* active_config(struct libusb_device *dev,struct libusb_device_handle *handle)    
{    
    struct libusb_device_handle *hDevice_req;    
    struct libusb_config_descriptor *config;    
    struct libusb_endpoint_descriptor *endpoint;    
    int altsetting_index,interface_index=0,ret_active;    
    int i,ret_print;    

    hDevice_req = handle;    

    ret_active = libusb_get_active_config_descriptor(dev,&config);    
    ret_print = print_configuration(hDevice_req,config);    

    for(interface_index=0;interface_index<config->bNumInterfaces;interface_index++)    
    {    
        const struct libusb_interface *iface = &config->interface[interface_index];    
        for(altsetting_index=0;altsetting_index<iface->num_altsetting;altsetting_index++)    
        {    
            const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_index];    

            int endpoint_index;    
            for(endpoint_index=0;endpoint_index<altsetting->bNumEndpoints;endpoint_index++)    
            {    
                const struct libusb_endpoint_desriptor *ep = &altsetting->endpoint[endpoint_index];    
                endpoint = ep;      
                alt_interface = altsetting->bAlternateSetting;    
                interface_number = altsetting->bInterfaceNumber;    
            }    

            printf("\nEndPoint Descriptors: ");    
            printf("\n\tSize of EndPoint Descriptor : %d",endpoint->bLength);    
            printf("\n\tType of Descriptor : %d",endpoint->bDescriptorType);    
            printf("\n\tEndpoint Address : 0x0%x",endpoint->bEndpointAddress);    
            printf("\n\tMaximum Packet Size: %x",endpoint->wMaxPacketSize);    
            printf("\n\tAttributes applied to Endpoint: %d",endpoint->bmAttributes);    
            printf("\n\tInterval for Polling for data Tranfer : %d\n",endpoint->bInterval);    
        }    
    }    
    libusb_free_config_descriptor(NULL);    
    return endpoint;    
}    
#define MAX_LINE_LENGTH 80
void readConfiguration(double *amp, double *bias){
    //File with configuration of each pA
    char *path = "/home/tpc/Products/picologic_libusb/config/calibration.conf";
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
        /* Print each line */
        //printf("line[%06d]: %s", ++line_count, line);
        ++line_count;
        /* Add a trailing newline to lines that don't already have one */
        if (line[strlen(line) - 1] != '\n')
            printf("\n");
	    int init_size = strlen(line);
	    char delim[] = " ";

        int wordN = 0;
	    char *ptr = strtok(line, delim);
	    while(ptr != NULL)
	    {
		    //printf("%d %d '%s'\n", line_count,wordN, ptr);
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


int main(int argc, char **argv)    
{
    int tsec=100;
    if (verbocity==1) printf("Setting parameters for the readout");
    if(argc==1){
        printf("Default arguments supplied: \n");
      printf("- output File name: %s\n", filename);
      printf("- measurement interval: %d [sec]\n", tsec);
    }else{
       if( argc == 3 ) {
           strcpy(filename, argv[1]);
          tsec = atoi(argv[2]);
          if (verbocity==1){
            printf("The arguments supplied: \n");
            printf("- output File name: %s\n", filename);
            printf("- measurement interval: %d [sec]\n", tsec);
            //printf("The arguments supplied: is %s\n", argv[2]);
          }
       }
       else if( argc > 3 ) {
          printf("Too many arguments supplied.\n");
       }
       else {
          printf("Two arguments expected.\n");
       }
    }
   if (verbocity==1)printf("\n");
   double amp[12];
   double bias[12];
   readConfiguration(amp,bias);
   for(int n=0; n<12;n++){
       if (verbocity==1)printf("%d %f | %f \n",n,amp[n],bias[n]);
   }
    FILE* fptr;
    if(savefile){
        
        //if( access( filename, F_OK ) == 0 ) {
        //    // file exists
        //    printf("File %s Exists !! \n",filename);
        //    printf("Please use different File name. \n",filename);
        //    return 1;
        //} else {
            // file doesn't exist
            if (verbocity==1)printf("Append to the File: %s \n",filename);
            fptr=fopen(filename, "a+");
        //}
    }
    
    int r = 1;    
    struct libusb_device **devs;    
    struct libusb_device_handle *handle = NULL, *hDevice_expected = NULL;    
    struct libusb_device *dev,*dev_expected;    

    struct libusb_device_descriptor desc;    
    struct libusb_endpoint_descriptor *epdesc;    
    struct libusb_interface_descriptor *intdesc;    

    ssize_t cnt;    
    int e = 0,config2;    
    int i = 0,index,j=0;    
    char str1[64], str2[64];    
    char found = 0;    

// Init libusb     
    r = libusb_init(NULL);    
    if(r < 0)    
    {    
        printf("\nfailed to initialise libusb\n");    
        return 1;    
    }    
    else    
        if(verbocity==1)printf("\nInit Successful!\n");    

// Get a list os USB devices    
    cnt = libusb_get_device_list(NULL, &devs);    
    if (cnt < 0)    
    {    
        printf("\nThere are no USB devices on bus\n");    
        return -1;    
    }    
    if (verbocity==1)printf("\nDevice Count : %d\n-------------------------------\n",cnt);    

    //while ((dev = devs[i++]) != NULL)    
    for (int k=0;k<cnt;k++)
    {    
        dev = devs[k];
        r = libusb_get_device_descriptor(dev, &desc);    
        if (r < 0)    
            {    
            printf("failed to get device descriptor\n");    
            libusb_free_device_list(devs,1);    
            libusb_close(handle);    
            break;    
        }    
        if (verbocity==1){
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
        }
        if(desc.idVendor == 0x04b4 && desc.idProduct == 0x1003)    
        {    
            found = 1; 
            e = libusb_open(dev,&handle);    
            if (verbocity==1)printf("return value of openning : %d %s \n", e,libusb_error_name(e));
        
            if (e < 0)    
            {    
            printf("error opening device\n");    
            libusb_free_device_list(devs,1);    
            libusb_close(handle);    
            break;    
            }  

            e = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));    
            if (e < 0)    
            {    
            libusb_free_device_list(devs,1);    
            libusb_close(handle);    
            continue;    
            }    
            if (verbocity==1)printf("\nManufactured : %s",str1);    

            e = libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));    
            if(e < 0)    
            {    
            libusb_free_device_list(devs,1);    
            libusb_close(handle);    
            continue;    
            }    
        
            if (verbocity==1)printf("\nProduct : %s",str2);    
            if (verbocity==1)printf("\n----------------------------------------");    

 
            break;    
        } else{
            continue;
        }   
    }//end of while    
    
    if(found == 0)    
    {    
        printf("\nDevice NOT found\n");    
        libusb_free_device_list(devs,1);    
        libusb_close(handle);    
        return 1;    
    }    
    else    
    {    
        if(verbocity==1)printf("\nDevice found\n");    
        dev_expected = dev;    
        hDevice_expected = handle;    
    }    

    e = libusb_get_configuration(handle,&config2);    
    if(e!=0)    
    {    
        printf("\n***Error in libusb_get_configuration\n");    
        libusb_free_device_list(devs,1);    
        libusb_close(handle);    
        return -1;    
    }    
    if (verbocity==1)printf("\nConfigured value : %d",config2);    

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

    libusb_free_device_list(devs, 1);    

    //printf("3 \n");
    if(libusb_kernel_driver_active(handle, 0) == 1)    
    {    
        printf("\nKernel Driver Active");    
        if(libusb_detach_kernel_driver(handle, 0) == 0)    
            printf("\nKernel Driver Detached!");    
        else    
        {    
            printf("\nCouldn't detach kernel driver!\n");    
            libusb_free_device_list(devs,1);    
            libusb_close(handle);    
            return -1;    
        }    
    }    

    //printf("4 \n");
    e = libusb_claim_interface(handle, 0);    
    if(e < 0)    
    {    
        printf("\nCannot Claim Interface");    
        libusb_free_device_list(devs,1);    
        libusb_close(handle);    
        return -1;    
    }    
    else    
        if(verbocity==1)printf("\nClaimed Interface\n");    

    //active_config(dev_expected,hDevice_expected);    

    //   Communicate     

    if (verbocity==1)printf("Communicate \n");
    char *my_string;
    unsigned short *data_in;
    int *channels;
	   
    int transferred = 0;    
    int received = 0;    
    int length = 0; 
    int temp=0;
    int flaga=0,flagb=0,sync_a=0,k=0;   

    my_string = (char *)malloc(512);    
    channels = (int *)malloc(64);    

    /*memset(my_string,'\0',64);    
    memset(my_string1,'\0',64);    

    strcpy(my_string,"prasad divesd");    
    length = strlen(my_string);    

    printf("\nTo be sent : %s",my_string);    

    e = libusb_bulk_transfer(handle,BULK_EP_IN,my_string,length,&transferred,0);    
    if(e == 0 && transferred == length)    
    {    
        printf("\nWrite successful!");    
        printf("\nSent %d bytes with string: %s\n", transferred, my_string);    
    }    
    else    
        printf("\nError in write! e = %d and transferred = %d\n",e,transferred);    */

    //sleep(1);    
    i = 0;    
    length=2;
    sync_a=0;
    //for(i = 0; i < length; i++) 

    while(1)	   
    {    
        //printf("handle=%d",handle);
        e = libusb_bulk_transfer(handle,BULK_EP_OUT,my_string,512,&received,0);     
        if(e==0)    
        {    
            data_in=my_string;	
            for(j=0; j<256; j++){
                
                if(data_in[j]==0xFEFE && sync_a==0)
		        {
                   length=j;
		            sync_a=1;
		            k=0;
		            if(savefile==false)
		                printf("%lld ", current_timestamp());
		            else
		                fprintf(fptr, "%lld ", current_timestamp());
                }
                else if(sync_a==1){
			        temp=(int)(data_in[j]<<8|data_in[j]>>8)&0xffff;
			        channels[k]=temp-0x8000;
			        //if(i%100==0) 
			        if(savefile==false)
			            //printf("%5.3f ",0.0039*(float)channels[k]);
			            printf("%5.3f ",amp[k]*(float)channels[k]-bias[k]);
			        else{
    			        //fprintf(fptr, "%5.3f ",0.0039*(float)channels[k]);
    			        fprintf(fptr, "%5.3f ",amp[k]*(float)channels[k]-bias[k]);
                    }
                    k++;
			        if(k==12)            //number of channels
			        {
				        sync_a=0;
				        //if(i%100==0)
				        if(savefile==false)
				            printf("\n");
				        else
				            fprintf(fptr, "\n");
				        break;
			        }
		        }
		
		
		        //printf(" %04X",data_in[j]&0xffff);    //will read a string from lcp2148
	        }
	    
	        i++;
 
        }else{
		    printf("\nReceived: %d %d\n", e, length);  
		    return -1; 	
	    }
	
	sleep(sleeptime);
	if(i%5==0){
	    time_t T= time(NULL);
            struct  tm tm = *localtime(&T);
            if(verbocity==1)printf("%04d%02d%02d %02d%02d%02d \n", tm.tm_year+1990, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
	if (i>=tsec) break;        
    }    
    
    //swapped = (num>>8) | (num<<8);
    e = libusb_release_interface(handle, 0);    

    libusb_close(handle);    
    libusb_exit(NULL);    

    if(verbocity==1)printf("\n");    
    if(savefile){
        fclose(fptr);
    }
    return 0;    
}    
