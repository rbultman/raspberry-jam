/*
 * Some utility functions
 */
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

int checkIpFormat(const char *ip) {
   uint16_t quad[4];
   uint8_t numread;
   uint8_t i;

   printf("Checking: %s\r\n", ip);

   numread = sscanf(ip, "%hu.%hu.%hu.%hu", &quad[0], &quad[1], &quad[2], &quad[3]);

   if (numread != 4) {
      puts("Not enough numbers entered.");
      return 1;
   }

   for(i=0; i<4; i++) {
      if (quad[i] > 255) {
         printf("Number out of range: %hu\r\n", quad[i]);
         return 2;
      } 
   }

   return 0;
}

void sleep_millis(int millis)
{
   struct timespec req = {0};
   req.tv_sec = 0;
   req.tv_nsec = millis * 1000000L;
   nanosleep(&req, (struct timespec *)NULL);
}

void TrimWhitespace(char *pBuf)
{
   char *p = pBuf;
   int i;

   // strip leading whitepace
   while (isspace(*p)) p++;

   // copy remaining
   strcpy(pBuf, p);

   // trim trailing white space
   for(i=strlen(pBuf)-1; i>0; i--) {
      if(isspace(pBuf[i])) {
         pBuf[i] = 0;
      } else {
         break;
      }
   }
}

