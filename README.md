# pA Reader
This repository contain scripts for reading out PicoLogic PA125-24 12 channel floating-point pico-ampermeters for the sPHENIX TPC readout modules test stand.
## Installation process
- Install the usb drivers:
`sudo apt-get install libusb-1.0.0`
The library is in: `<libusb-1.0/libusb.h>`
- Build the code:
`gcc -o test.o -Wformat=0 -Wno-incompatible-pointer-types picologic.c -lusb-1.0`
- Run the code:
`sudo ./test.o DATA.txt 10`
it takes as an input:
    - Name of the output file;
    - Time for recording in seconds
## ToDo List:
1. Set correct amplification and bias for each pA;
2. Develop GUI like PicoLogic has in LabVIEW:
    - On the fly plotting of the currents;
    - Set output file name;
    - Set time for recording
