#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define DEFAULT_PORT 8080
#define MAXLINE 1024
#define MAXCLIENTS 16
#define MAXSTRING 256

int port = DEFAULT_PORT;

void printUsage(char *prog)
{
   printf("Usage: \r\n");
   printf("\t%s [-p PORT]\r\n", prog);
   printf("where:\r\n");
   printf("\t PORT is the port number to listen on (default 8080) and should be reachable from the internet\r\n");
   printf("example:\r\n");
   printf("\t%s -p 8888\r\n", prog);
}

int processArgs(int argc, char **argv)
{

   if (argc == 1)
      return 0;

   if (argc != 3)
   {
      printUsage(argv[0]);
      return -1;
   }

   if (strcmp(argv[1], "-p") != 0)
   {
      printUsage(argv[0]);
      return -1;
   }

   if (argc == 3)
   {
      int p = atoi(argv[2]);
      if (p > 0)
      {
         port = p;
         printf("Port number set to %d\n", port);
      }
      else
      {
         printf("Malformed port number?\r\n");
         printUsage(argv[0]);
         return -1;
      }
   }
   return 0;
}

void serverHandler()
{
   int sockfd;
   char buffer[MAXLINE];
   struct sockaddr_in servaddr, cliaddr;
   static char names[MAXCLIENTS][MAXSTRING], addrs[MAXCLIENTS][MAXSTRING], ports[MAXCLIENTS][MAXSTRING];
   int next_client_num = 0;
   int num_clients = 0;

   // Creating socket file descriptor
   if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("socket creation failed");
      exit(EXIT_FAILURE);
   }

   memset(&servaddr, 0, sizeof(servaddr));
   memset(&cliaddr, 0, sizeof(cliaddr));

   // Filling server information
   servaddr.sin_family = AF_INET; // IPv4
   servaddr.sin_addr.s_addr = INADDR_ANY;
   servaddr.sin_port = htons(port);

   // Bind the socket with the server address
   if (bind(sockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0)
   {
      perror("bind failed");
      exit(EXIT_FAILURE);
   }

   socklen_t len;
   int n;

   len = sizeof(cliaddr); //len is value/resuslt

   printf("Connection Server ready to receive on port %d.\nMake sure this is reachable from the internet.\n", port);

   while (1)
   {
      char *client_name;
      int client_num = -1;

      n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                   MSG_WAITALL, (struct sockaddr *)&cliaddr,
                   &len);
      buffer[n] = '\0';
      printf("Client from %s : %d sent: %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buffer);
      client_name = strtok(buffer, "\r\n ");
      // is this a new name?
      for (int i = 0; i < num_clients; i++)
      {
         if (strcmp(names[i], client_name) == 0)
         {
            // found it, not a new name
            client_num = i;
         }
      }
      if (client_num < 0)
      {
         // new name, which slot should we give?
         client_num = next_client_num;
         next_client_num++;
         if (next_client_num >= MAXCLIENTS)
         {
            next_client_num = 0;
            printf("**WARNING** TOO MANY CLIENTS, STARTING TO DROP THE OLDEST ONES**\n");
         }
         if (num_clients < MAXCLIENTS)
            num_clients++;
         printf("Adding client #%d to table of clients.\n", client_num);
      }
      else
      {
         printf("Updating client #%d in table of clients.\n", client_num);
      }
      snprintf(names[client_num], MAXSTRING, "%s", client_name);
      snprintf(addrs[client_num], MAXSTRING, "%s", inet_ntoa(cliaddr.sin_addr));
      snprintf(ports[client_num], MAXSTRING, "%d", ntohs(cliaddr.sin_port));
      // Send the current list of clients back to this client that just connected
      for (int i = 0; i < num_clients; i++)
      {
         snprintf(buffer, MAXLINE, "%d,%d,%s,%s,%s\n", i, num_clients, names[i], addrs[i], ports[i]);
         printf("%s", buffer);
         sendto(sockfd, buffer, strlen(buffer),
                MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
                sizeof(cliaddr));
      }
   }
}

int main(int argc, char **argv)
{
   if (processArgs(argc, argv) != 0)
   {
      return -1;
   }

   serverHandler();
   // never returns
   return 0;
}
