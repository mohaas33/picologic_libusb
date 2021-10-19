g++ picologic_server_tcp_mpl.cc -std=c++14 \
 -I /usr/local/lib/python3.7/dist-packages \
 -I /usr/include/python3.7 \
 -I /usr/lib/python3.7 \
 -lpython3.7m \
 -Wformat=0 \
 -lusb-1.0 \
 -o picologic_server_tcp_mpl 
 