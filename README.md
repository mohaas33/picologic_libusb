(0) In the picologic.c file, the header <libusb.h> is in fact <libusb-1.0/libusb.h>

the file can be found in /usr/include/libusb-1.0 after I do 
sudo apt-get install libusb-1.0.0

"sudo apt install libusb-dev" this one seems did not work in my case.

(1) Notes for compiling the code

sudo apt-get install libusb-1.0.0-dev

gcc -o test.o -Wformat=0 -Wno-incompatible-pointer-types picologic.c -lusb-1.0

(2) After successfully compiled, I run ./test.o and came to error of not opening the device

I realized it is permission to access to the usb port, in this case it is at /dev/bus/001/004

sudo ./test.o worked!!!
