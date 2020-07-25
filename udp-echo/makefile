#PROGS=udpclient udpserver udp-puncher
PROGS=udpclient udpserver 

all: $(PROGS)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CCFLAGS += -D MACH_LINUX
endif
ifeq ($(UNAME_S),Darwin)
	CCFLAGS += -D MACH_OSX
endif

udpclient: udpclient.c
	gcc $(CCFLAGS) -o udpclient udpclient.c

udpserver: udpserver.c
	gcc $(CCFLAGS) -o udpserver udpserver.c

#udp-puncher: udp-puncher.cpp
#	g++ -pthread -o udp-puncher udp-puncher.cpp

clean:
	rm $(PROGS)

