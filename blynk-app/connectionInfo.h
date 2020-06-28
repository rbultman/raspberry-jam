#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

typedef struct ConnectionInfo_T {
   char name[64];
   char ipAddr[20];
   int port;
   int role; 
   int latency;
   int gain;
} ConnectionInfo_T;

#endif // CONNECTION_INFO_H
