/* 
 * Client side implementation of UDP client-server model 
 *
 * This was developed in order to understand round-trip UDP latency.
 * This work is int the public domain.
 *
 * This is a derivative work combining concepts from these two pages:
 * - https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 * - https://gist.github.com/suyash/0f100b1518334fcf650bbefd54556df9
*/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>

#define PORT	 4464 
#define MAXLINE 1024 

void printUsage() {
   printf("Usage: \r\n");
   printf("\tudpclient -s <server address> -n <name>");
   printf("where:\r\n");
   printf("\t <server address> is the server to connect to\r\n");
   printf("\t <name> is a text string to send to the server\r\n");
   printf("example:\r\n");
   printf("\tudpclient -s 127.0.0.1 -n joe\r\n");
}

int serverIndex = 0;
int nameIndex = 0;

int processArgs(int argc, char ** argv) {
   int i;

   if (argc < 5) {
      printUsage();
      return -1;
   }

   for(i=1; i<argc; i++) {
      if (strcmp(argv[i],"-h") == 0) {
         printUsage();
         return -1;
      }
      if (strcmp(argv[i],"-s") == 0) {
         i++;
         serverIndex = i;
      }
      if (strcmp(argv[i],"-n") == 0) {
         i++;
         nameIndex = i;
      }
   }

   if (nameIndex == 0 || serverIndex == 0) {
      printUsage();
      return -1;
   }

   return 0;
}

// Driver code 
int main(int argc, char ** argv) {
	int sockfd; 
	char buffer[MAXLINE]; 
	struct sockaddr_in	 servaddr; 
   struct timespec startTime;
   struct timespec endTime;
   double startMillis;
   double endMillis;
   double latency;
   double maxLatency = 0.0;
   double minLatency = 100000000000.0;
   double avgLatency = -0.1;

   if(processArgs(argc, argv) != 0) {
      return -1;
   }

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	inet_pton(AF_INET, argv[serverIndex], &servaddr.sin_addr);
	servaddr.sin_port = htons(PORT); 
	
	int n, len; 
	
   while(1) {
      clock_gettime(CLOCK_REALTIME, &startTime);
      sendto(sockfd, argv[nameIndex], strlen(argv[nameIndex]), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
            sizeof(servaddr)); 
      printf("Hello message sent.\n"); 

      n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
            MSG_WAITALL, (struct sockaddr *) &servaddr, 
            &len); 
      clock_gettime(CLOCK_REALTIME, &endTime);
      buffer[n] = '\0'; 
      printf("Server : %s\n", buffer); 
      startMillis = startTime.tv_sec * 1000.0 + startTime.tv_nsec / 1.0e6;
      endMillis = endTime.tv_sec * 1000.0 + endTime.tv_nsec / 1.0e6;
      latency = endMillis - startMillis;
      if (latency > maxLatency) maxLatency = latency;
      if (latency < minLatency) minLatency = latency;
      if (avgLatency < 0.0) {
         avgLatency = latency;
      } else {
         avgLatency = 0.9 * avgLatency + 0.1 * latency;
      }
      printf("Latency: %.3f, min: %.3f, max: %.3f, avg: %.3f\r\n", latency, minLatency, maxLatency, avgLatency);
      sleep(2);
   }

	close(sockfd); 
	return 0; 
} 

