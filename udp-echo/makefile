#PROGS=udpclient udpserver udp-puncher
PROGS=udpclient udpserver 

all: $(PROGS)

udpclient: udpclient.c
	gcc -o udpclient udpclient.c

udpserver: udpserver.c
	gcc -o udpserver udpserver.c

#udp-puncher: udp-puncher.cpp
#	g++ -pthread -o udp-puncher udp-puncher.cpp

clean:
	rm $(PROGS)

