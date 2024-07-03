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

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define PORT  50003
#define MAXLINE (1024*2)

bool savefile = false;
//char filename[] = "data.txt";
float sleeptime = 1.0; // unit in seconds

int interface_ref = 0;    
int alt_interface,interface_number;    

#include <fstream>
#include <iostream>
#include <iomanip>

#include <libusb-1.0/libusb.h>
#include "pa_utilities.h"

using namespace std;

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


int main( int argc, char *argv[])
{
  // Fetch the calibration constants
  double amp[12];
  double bias[12];
  readCalibration(amp,bias);

  for (int i=0; i<12; i++)
    {
      cout << " i: " << i;
      cout << " amp: " << amp[i];
      cout << " bias: " << bias[i];
      cout << endl;
    }
  
  // Open the pAmmeter USB connection.
  pa_connect();

  //Open tcp server
  int sockfd;
  int in_fd;
  int i;
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
  while (sockfd > 0)
    {
      //  Accept a connection from the client...
      in_fd = AcceptClient(sockfd, 1);
      if ( in_fd <= 0)
	{
	  continue;
	}
	
      //  Read the message from the client.
      char buffer[MAXLINE];
      int n = read ( in_fd, buffer, MAXLINE);
      buffer[n] = '\0';  // terminate string for good measure
      //cout << __FILE__ << " " <<  __LINE__ << " received: " << buffer << endl;

      string str1(buffer);      
      if (str1.find("currents") == 0)
	{
	  string reply = communicate_pAs_v(amp, bias, 5120, 0);	  
	  write (in_fd, reply.c_str(), reply.size());
	}
      else if (str1.find("bulk") == 0)
	{
	  string reply = communicate_pAs_v(amp, bias, 5120, 1);	  
	  write (in_fd, reply.c_str(), reply.size());
	}
      else if (str1.find("raw") == 0)
	{
	  string reply = communicate_pAs_v(amp, bias, 5120, 2);	  
	  write (in_fd, reply.c_str(), reply.size());
	}
      else
	{
	  cout << __FILE__ << " " << __LINE__ << " unrecognized command: " << buffer << endl;
	}

      close (in_fd);
      in_fd = 0;
    }

  return 0;
}
