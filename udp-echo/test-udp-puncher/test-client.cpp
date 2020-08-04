#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define MAXLINE 1024
#define REREGISTER_CLOCK 30

int serverIndex = 0;
int nameIndex = 0;
int portIndex = 0;
int contactIndex = 0;

char name[128] = "client";
char contact[128] = "contact";
char serverAddress[128] = "127.0.0.1";
int serverPort = 8080;

void printUsage(char *prog)
{
   printf("Usage: \r\n");
   printf("\t%s -s <server address> -p <server port> -n <my name> -c <name to contact>\n", prog);
   printf("where:\r\n");
   printf("\t <server address> is the STAGE MANAGER server to connect to\r\n");
   printf("\t <server port> is the STAGE MANAGER port to connect to\r\n");
   printf("\t <name> is a text string to send to the server\r\n");
   printf("example:\r\n\t(alice would do this, if she is trying to reach bob)\r\n");
   printf("\t%s -s 127.0.0.1 -p 8080 -n alice -c bob\r\n", prog);
   printf("\t(bob would do this, if he is trying to reach alice)\r\n");
   printf("\t%s -s 127.0.0.1 -p 8080 -n bob -c alice\r\n", prog);
}

int processArgs(int argc, char **argv)
{
   int i;

   if (argc < 9)
   {
      printUsage(argv[0]);
      return -1;
   }

   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-h") == 0)
      {
         printUsage(argv[0]);
         return -1;
      }

      if (strcmp(argv[i], "-p") == 0)
      {
         i++;
         portIndex = i;
         serverPort = atoi(argv[i]);
      }

      if (strcmp(argv[i], "-s") == 0)
      {
         i++;
         serverIndex = i;
         strcpy(serverAddress, argv[i]);
      }

      if (strcmp(argv[i], "-n") == 0)
      {
         i++;
         nameIndex = i;
         strcpy(name, argv[i]);
      }

      if (strcmp(argv[i], "-c") == 0)
      {
         i++;
         contactIndex = i;
         strcpy(contact, argv[i]);
      }
   }

   if (portIndex == 0 || nameIndex == 0 || serverIndex == 0 || contactIndex == 0)
   {
      printUsage(argv[0]);
      return -1;
   }

   return 0;
}

void stageManagerHandler()
{
   int sockfd;
   struct sockaddr_in servaddr, cliaddr, contactaddr;
   struct timeval timeout;
   char buffer[MAXLINE];
   int reregister_clock = REREGISTER_CLOCK;

   // Creating socket file descriptor
   if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("socket creation failed");
      exit(EXIT_FAILURE);
   }

   timeout.tv_sec = 1; // set the read timeout to a seconds so we can keep alive connections
   timeout.tv_usec = 0;
   setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout, sizeof(struct timeval));

   memset(&servaddr, 0, sizeof(servaddr));
   memset(&contactaddr, 0, sizeof(contactaddr));

   // Filling server information
   servaddr.sin_family = AF_INET;
   inet_pton(AF_INET, serverAddress, &servaddr.sin_addr);
   servaddr.sin_port = htons(serverPort);

   socklen_t len;
   int n;
   char ipaddr[INET_ADDRSTRLEN];

   while (1)
   {
      puts("Registering with Stage Manager.");
      sendto(sockfd, name, strlen(name),
             0, (const struct sockaddr *)&servaddr,
             sizeof(servaddr));
      puts("Retrieving incoming messages.");
      while (1)
      {
         n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                      MSG_WAITALL, (struct sockaddr *)&cliaddr,
                      &len);
         printf(".");
         fflush(stdout);
         reregister_clock--;
         if (reregister_clock <= 0)
         {
            reregister_clock = REREGISTER_CLOCK;
            break;
         }
         if (n < 0)
            continue; // just wait some more

         buffer[n] = '\0';
         // check who it was that just sent the message
         inet_ntop(AF_INET, &(cliaddr.sin_addr), ipaddr, INET_ADDRSTRLEN);
         // This is not really a solid test, but should be good enough in most cases
         if (strcmp(ipaddr, serverAddress) == 0 && cliaddr.sin_port == servaddr.sin_port)
         {
            printf("Stage Manager sent us a Phonebook entry: %s", buffer);
            char delims[8] = ",\n\r ";
            int slotnum = atoi(strtok(buffer, delims));
            int nslots = atoi(strtok(NULL, delims));
            char peer_name[256];
            strncpy(peer_name, strtok(NULL, delims), 256);
            char peer_addr[256];
            strncpy(peer_addr, strtok(NULL, delims), 256);
            int peer_port = atoi(strtok(NULL, delims));
            printf("Peer: %d/%d %s@%s:%d\n", slotnum, nslots, peer_name, peer_addr, peer_port);
            if (strcmp(peer_name, contact) == 0)
            {
               // we found the contact we are looking for!
               printf("Located %s!\n", peer_name);
               // Filling the contact server information
               contactaddr.sin_family = AF_INET;
               inet_pton(AF_INET, peer_addr, &contactaddr.sin_addr);
               contactaddr.sin_port = htons(peer_port);
            }
         }
         else
         {
            printf("We received a peer message of len %d from %s:%d ->>\n%s\n", n, ipaddr, cliaddr.sin_port, buffer);
         }
         if (contactaddr.sin_port != 0)
         {
            // We have contact info, so keep trying to talk to them :-)
            sleep(1); // just to slow things down a bit
            sprintf(buffer, "This is a direct P2P message from %s to %s", name, contact);
            sendto(sockfd, buffer, strlen(buffer),
                   0, (const struct sockaddr *)&contactaddr,
                   sizeof(contactaddr));
         }
      }
   }
}

int main(int argc, char **argv)
{
   if (processArgs(argc, argv) != 0)
   {
      return -1;
   }

   printf("Stage Manager Server address: %s\r\n", serverAddress);
   printf("My Name: %s\r\n", name);

   stageManagerHandler();
   // never returns

   return 0;
}
