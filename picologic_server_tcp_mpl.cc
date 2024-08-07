// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <time.h>
#include <stdbool.h> 

#include <libusb-1.0/libusb.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define PORT  50001
#define MAXLINE (1024*2)

bool savefile = false;
//char filename[] = "data.txt";
float sleeptime = 1.0; // unit in seconds

int interface_ref = 0;    
int alt_interface,interface_number;    

#define BULK_EP_OUT     0x82    
#define BULK_EP_IN      0x08  

#define MAX_LINE_LENGTH 80

#include <fstream>
#include <iostream>
#include <iomanip>

#define _USE_MATH_DEFINES
#include <cmath>
#include "matplotlibcpp.h"

#include <vector>

namespace plt = matplotlibcpp;


using namespace std;

string filename = "data.txt";


long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

vector<string> split (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}
int AcceptClient(int s, int timeout)
{
   int iResult;
   struct timeval tv;
   fd_set rfds;
   FD_ZERO(&rfds);
   FD_SET(s, &rfds);

   tv.tv_sec = (long)timeout;
   tv.tv_usec = 0;

   iResult = select(s+1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
   if(iResult > 0)
   {
      return accept(s, NULL, NULL);
   }
   else
   {
     //always here
   }
   return 0;
}
void readConfiguration(double *amp, double *bias){
    //File with configuration of each pA
    char path[] = "./config/calibration.conf";
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
string convertToString(char* a, int size)
{
  // Converts character array
  // to string and returns it
  int i;
  string s = "";
  for (i = 0; i < size; i++) {
      s = s + a[i];
  }
  return s;
}

static string nextWord(string &S, string &word)
{
  //Finds the next word in a line
  static char delimiters[] = " :\n\t\r";
  string re;
  string::size_type   p1 = 0, p2 = 0;
  p1 = S.find(word)+word.length();
  if (p1 == string::npos) 
      return "";
  p2 = S.find_first_of(delimiters, p1);
  re = S.substr(p1, (p2 - p1));

  return re;
}
string communicate_pAs(libusb_device_handle *handle, double *amp, double *bias){
  //   Communicate     
  printf("Communicate \n");
  FILE* fptr;
  if(savefile){

    int n = filename.length();
 
    // declaring character array
    char char_filename[n + 1];
 
    // copying the contents of the
    // string to char array
    strcpy(char_filename, filename.c_str());
      if( access( char_filename, F_OK ) == 0 ) {
          // file exists
          printf("File %s Exists !! \n",filename);
          printf("Please use different File name. \n",filename);
          //return 1;
          
      } else {
          // file doesn't exist
          printf("Create NEW File: %s \n",filename);
          fptr=fopen(char_filename, "a+");
      }
  }
  //std::stringstream ss;
  string currents_out;
  unsigned char *my_string;
  unsigned short *data_in;
  int *channels;

  int transferred = 0;    
  int received = 0;    
  int length = 0; 
  int temp=0;
  int flaga=0,flagb=0,sync_a=0,k=0;   
  my_string = (unsigned char *)malloc(512);    
  channels = (int *)malloc(64);    
  //int ind = 0;    
  length=2;
  sync_a=0;
  //for(i = 0; i < length; i++) 
  //while(1){
    currents_out.clear();
    int e = libusb_bulk_transfer(handle,BULK_EP_OUT,my_string,512,&received,0);     
    if(e==0){    
      data_in = ((unsigned short*) my_string);	
      for(int j=0; j<256; j++){
        if(data_in[j]==0xFEFE && sync_a==0){
          length=j;
	        sync_a=1;
	        k=0;
	        if(savefile==false){
              char c_tmp[20];
	            sprintf(c_tmp,"%lld ", current_timestamp());
              currents_out.append(c_tmp);
          }else{
	            fprintf(fptr, "%lld ", current_timestamp());
          }
        }else if(sync_a==1){
		      temp=(int)(data_in[j]<<8|data_in[j]>>8)&0xffff;
		      channels[k]=temp-0x8000;
		      if(savefile==false){
		        //printf("%5.3f ",0.0039*(float)channels[k]);
            char c_tmp[20];
		        sprintf(c_tmp,"%5.4f ",amp[k]*(float)channels[k]-bias[k]);
            currents_out.append(c_tmp);
		      }else{
  		      //fprintf(fptr, "%5.3f ",0.0039*(float)channels[k]);
  		      fprintf(fptr, "%5.3f ",amp[k]*(float)channels[k]-bias[k]);
          }
          k++;
		      if(k==12){            //number of channels
		        sync_a=0;
		        //if(i%100==0)
		        if(savefile==false){
		            printf("\n");
                cout<< currents_out <<endl;
            }else{
		            fprintf(fptr, "\n");
            }
            break;
		      }
	      }

	    }	
	    //ind++;
    }else{
      printf("\nReceived: %d %d\n", e, length);  
      //return -1; 	
	  }
	  //sleep(sleeptime);

	  //if (ind>=tsec) break;        
  //}  //end of while loop 

  if(savefile){
      fclose(fptr);
  }
  return currents_out;

}


vector<float> communicate_pAs_v(libusb_device_handle *handle, double *amp, double *bias){
  //   Communicate     
  //printf("Communicate \n");

  //std::stringstream ss;
  vector<float> currents_out;
  unsigned char *my_string;
  unsigned short *data_in;
  int *channels;

  int transferred = 0;    
  int received = 0;    
  int length = 0; 
  int temp=0;
  int flaga=0,flagb=0,sync_a=0,k=0;   
  my_string = (unsigned char *)malloc(512);    
  channels = (int *)malloc(64);    
  //int ind = 0;    
  length=2;
  sync_a=0;
  //for(i = 0; i < length; i++) 
  //while(1){
    currents_out.clear();
    int e = libusb_bulk_transfer(handle,BULK_EP_OUT,my_string,512,&received,0);     
    if(e==0){    
      data_in = ((unsigned short*) my_string);	
      for(int j=0; j<256; j++){
        if(data_in[j]==0xFEFE && sync_a==0){
          length=j;
	        sync_a=1;
	        k=0;
          char c_tmp[20];
	        printf(c_tmp,"%lld ", current_timestamp());
          //currents_out.append(c_tmp);
          
        }else if(sync_a==1){
		      temp=(int)(data_in[j]<<8|data_in[j]>>8)&0xffff;
		      channels[k]=temp-0x8000;
          char c_tmp[20];
		      sprintf(c_tmp,"%5.4f ",amp[k]*(float)channels[k]-bias[k]);
          currents_out.push_back(amp[k]*(float)channels[k]-bias[k]);
		      
          k++;
		      if(k==12){            //number of channels
		        sync_a=0;
		        //if(i%100==0)

            
            break;
		      }
	      }

	    }	
	    //ind++;
    }else{
      printf("\nReceived: %d %d\n", e, length);  
      //return -1; 	
	  }

  return currents_out;

}


int main( int argc, char *argv[])
{

    int tsec=100;

  printf("Setting parameters for the readout");

  printf("\n");

  double amp[12];

  double bias[12];

  readConfiguration(amp,bias);

  //for(int n=0; n<12;n++){

    //printf("%d %f | %f \n",n,amp[n],bias[n]);

  //}





    

  int r = 1;    

  struct libusb_device **devs;    

  struct libusb_device_handle *handle = NULL, *hDevice_expected = NULL;    

  struct libusb_device *dev,*dev_expected;    



  struct libusb_device_descriptor desc;    

  struct libusb_endpoint_descriptor *epdesc;    

  struct libusb_interface_descriptor *intdesc;    

  

  ssize_t cnt;    

  int e = 0,config2;    

  int ind = 0,index,j=0;    

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

    printf("\nInit Successful!\n");    



  // Get a list os USB devices    

  cnt = libusb_get_device_list(NULL, &devs);    

  if (cnt < 0)    

  {    

      printf("\nThere are no USB devices on bus\n");    

      return -1;    

  }    

  printf("\nDevice Count : %d\n-------------------------------\n",cnt);    



  //while ((dev = devs[i++]) != NULL)    

  for (int k=0;k<cnt;k++)

  {    

      dev = devs[k];

      r = libusb_get_device_descriptor(dev, &desc);    

      if (r < 0){

        printf("failed to get device descriptor\n");    

        libusb_free_device_list(devs,1);    

        libusb_close(handle);    

        break;    

      }    



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

        

      if(desc.idVendor == 0x04b4 && desc.idProduct == 0x1003){    

        found = 1; 

        e = libusb_open(dev,&handle);    

        printf("return value of openning : %d %s \n", e,libusb_error_name(e));

      

        if (e < 0){    

          printf("error opening device\n");    

          libusb_free_device_list(devs,1);    

          libusb_close(handle);    

          break;    

        }  



        e = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));    

        if (e < 0){

          libusb_free_device_list(devs,1);    

          libusb_close(handle);    

          continue;    

        }    

        printf("\nManufactured : %s",str1);    



        e = libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));    

        if(e < 0){    

          libusb_free_device_list(devs,1);    

          libusb_close(handle);    

          continue;    

        }    

      

        printf("\nProduct : %s",str2);    

        printf("\n----------------------------------------");    

 

        break;    

      }else{

        continue;

      }   

  }//end of for loop

    

  if(found == 0){    

    printf("\nDevice NOT found\n");    

    libusb_free_device_list(devs,1);    

    libusb_close(handle);    

    return 1;    

  }else{    

    printf("\nDevice found\n");    

    dev_expected = dev;    

    hDevice_expected = handle;    

  }    



  e = libusb_get_configuration(handle,&config2);    

  if(e!=0){    

    printf("\n***Error in libusb_get_configuration\n");    

    libusb_free_device_list(devs,1);    

    libusb_close(handle);    

    return -1;    

  }    

  printf("\nConfigured value : %d",config2);    



  if(config2 != 1){    

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



  e = libusb_claim_interface(handle, 0);    

  if(e < 0){    

    printf("\nCannot Claim Interface");    

    libusb_free_device_list(devs,1);    

    libusb_close(handle);    

    return -1;    

  }    

  else    

    printf("\nClaimed Interface\n");    
  //Open server
  int sockfd;
  int in_fd;
  int i;
  char buffer[MAXLINE];
  struct sockaddr_in servaddr, cliaddr;

  char *reply;
  
  // Creating socket file descriptor
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
      perror("socket creation failed");
      exit(EXIT_FAILURE);
    }
  
  memset(&servaddr, 0, sizeof(servaddr));
  
  // Filling server information
  servaddr.sin_family = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(PORT);

  // Bind the socket with the server address
  if ( bind(sockfd, (const struct sockaddr *)&servaddr,
	    sizeof(servaddr)) < 0 )
    {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
  
  listen(sockfd, 1);
  
  

	int ax_n = 100;
	std::vector<double> x, y, z;
  std::vector<double> currents[12];
  int ix=0;
  float x_min=0;
  float x_max=ax_n;
  int step=0;

  //Waiting commands from the client 
  unsigned int len, n;

  while (sockfd > 0){
    vector<float> out_currents;// = communicate_pAs_v(handle, amp, bias);
    //bool plot_f=true;
    //while (plot_f){
//
    out_currents = communicate_pAs_v(handle, amp, bias);
    ix++;
//    //  sleep(sleeptime);
//
    if(x.size()>1000){
      x.clear();
      for(int c=0;c<12;c++){
        currents[c].clear();
      }
    }
    x.push_back(ix);
//// Clear previous plot
    plt::clf();  
    int shift = ix/ax_n;
    if(step!=shift){
      x_min+=10*shift;
      x_max+=10*shift;   
      step=shift;
    }
    //  //cout<<shift<<" "<<x_min<<" : "<<x_max<<endl; 
	  // Plot lines. Color is selected automatically.
    for(int c=0;c<12;c++){
      currents[c].push_back(out_currents[c]);
      char name[10];
      snprintf(name, sizeof(name), "%d", c);
      plt::named_plot(name,x, currents[c]);
      // Set x-axis to interval [0,1000000]
      plt::xlim(x_min, x_max);
    }
    
//
//
  	//  // Add graph title
		//  //plt::title("Sample figure");
		//  // Enable legend.
		plt::legend();
		//  // Display plot continuously
		plt::pause(0.01);
    //}

    len = sizeof(cliaddr); 
    memset(&cliaddr, 0, sizeof(cliaddr));

    in_fd = AcceptClient(sockfd, 1);
    //in_fd = accept(sockfd,  (struct sockaddr *) &cliaddr, &len);
    if ( in_fd <= 0){
	    continue;
	  }
	
    n = read ( in_fd, buffer, MAXLINE);
    for ( int i = 0; i < n; i++){
      cout << i << " " << hex << setw(3) << int(buffer[i]) << dec;
	    if ( buffer[i] >= 32 ) cout << "  " << buffer[i];  // printable char
        cout << endl;
	  }
            
    string buff_str = convertToString(buffer,sizeof(buffer) / sizeof(char));
    //cout<<"Sir, this is your buffer string" << buff_str <<endl;
    if (buff_str.find("current")==true){
      string str = communicate_pAs(handle, amp, bias);
      reply = &str[0];   
      write (in_fd, reply, strlen(reply) +1);
    }
    string s_file = "file:";
    if (buff_str.find(s_file)==true){
      string file_name = nextWord(buff_str, s_file);
      filename = file_name;
      string str = "File: " + filename;
      reply = &str[0];   
      write (in_fd, reply, strlen(reply) +1);
    }
    string s_time = "time:";
    if (buff_str.find(s_time)==true){
      cout << "Dude, you typed time" << endl;
      string time_sec = nextWord(buff_str, s_time);
      string str = "t= " + time_sec;
      tsec = stoi(time_sec);
      reply = &str[0];   
      write (in_fd, reply, strlen(reply) +1);        
    }
    string s_write = "write";
    if (buff_str.find(s_write)==true){
      ofstream out_file;
      out_file.open (filename);
      for (int t=0;t<tsec;t++){
        string current_str = communicate_pAs(handle, amp, bias);
        out_file << current_str;
        sleep(sleeptime);
      }
      out_file.close();
    }
    close (in_fd);
    in_fd = 0;
  }
  //swapped = (num>>8) | (num<<8);
  //e = libusb_release_interface(handle, 0);    

  //libusb_close(handle);    
  //libusb_exit(NULL);    

  printf("\n");    


  return 0;
}
