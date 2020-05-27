// thread example
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
 
#define PORT	4464 
#define MAXLINE 1024 

int serverIndex = 0;
int nameIndex = 0;

char name[128] = "client";
char serverAddress[128] = "127.0.0.1";

void printUsage(char * prog) {
   printf("Usage: \r\n");
   printf("\t%s -s <server address> -n <name>", prog);
   printf("where:\r\n");
   printf("\t <server address> is the server to connect to\r\n");
   printf("\t <name> is a text string to send to the server\r\n");
   printf("example:\r\n");
   printf("\t%s -s 127.0.0.1 -n rob\r\n", prog);
}

int processArgs(int argc, char ** argv) {
   int i;

   if (argc < 5) {
      printUsage(argv[0]);
      return -1;
   }

   for(i=1; i<argc; i++) {
      if (strcmp(argv[i],"-h") == 0) {
         printUsage(argv[0]);
         return -1;
      }
      if (strcmp(argv[i],"-s") == 0) {
         i++;
         serverIndex = i;
         strcpy(serverAddress, argv[i]);
      }
      if (strcmp(argv[i],"-n") == 0) {
         i++;
         nameIndex = i;
         strcpy(name, argv[i]);
      }
   }

   if (nameIndex == 0 || serverIndex == 0) {
      printUsage(argv[0]);
      return -1;
   }

   return 0;
}

void clientHandler() 
{
   int sockfd; 
   struct sockaddr_in servaddr; 

   // Creating socket file descriptor 
   if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
      perror("socket creation failed"); 
      exit(EXIT_FAILURE); 
   } 

   memset(&servaddr, 0, sizeof(servaddr)); 

   // Filling server information 
   servaddr.sin_family = AF_INET; 
   inet_pton(AF_INET, serverAddress, &servaddr.sin_addr);
   servaddr.sin_port = htons(PORT); 

   puts("Client ready to send.");

   while (1) {
      sendto(sockfd, name, strlen(name), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
            sizeof(servaddr)); 
      sleep(1);
   }
}

void serverHandler()
{
	int sockfd; 
	char buffer[MAXLINE]; 
	struct sockaddr_in servaddr, cliaddr; 

   // Creating socket file descriptor 
   if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
      perror("socket creation failed"); 
      exit(EXIT_FAILURE); 
   } 

   memset(&servaddr, 0, sizeof(servaddr)); 
   memset(&cliaddr, 0, sizeof(cliaddr)); 

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

   socklen_t len; 
   int n; 

   len = sizeof(cliaddr); //len is value/resuslt 

   puts("Server ready to receive.");

   while(1) {
      n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
            MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
            &len); 
      buffer[n] = '\0'; 
      printf("Client sent: %s\n", buffer); 
   }
}

int main(int argc, char ** argv) 
{
  if(processArgs(argc, argv) != 0) {
     return -1;
  }

  printf("Server address: %s\r\n", serverAddress);
  printf("Name: %s\r\n", name);

  std::thread clientThread (clientHandler);     // spawn new thread that calls foo()
  std::thread serverThread (serverHandler);  // spawn new thread that calls bar(0)

  while (1) {
   puts("Main tick.");
   sleep(5);
  }

  return 0;
}

