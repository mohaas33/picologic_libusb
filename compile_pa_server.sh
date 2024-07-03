g++ pa_server.cc pa_utilities.cc -std=c++14 \
 -I /usr/local/lib/python3.7/dist-packages \
 -I /usr/include/python3.7 \
 -I /usr/lib/python3.7 \
 -Wformat=0 \
 -lusb-1.0 \
 -o pa_server
 
