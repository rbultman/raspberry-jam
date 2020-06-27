#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userSpec.h"
#include "readUsers.h"

#define USER_FILE_NAME "../settings/users.csv"

static char * strip(char *p)
{
   char *pEOL;

   if (strlen(p) == 0) return p;

   // strip front whitespace
   while(*p == ' ') p++;

   // strip EOL characters
   pEOL = strchr(p, '\r');
   if (pEOL != NULL) *pEOL = 0;
   pEOL = strchr(p, '\n');
   if (pEOL != NULL) *pEOL = 0;

   if (strlen(p) != 0)
   {
      // strip ending whitespace
      pEOL = &p[strlen(p)-1];
      while(*pEOL == ' ') 
      {
         *pEOL = 0;
         pEOL--;
         if (strlen(p) == 0) break;
      }
   }

   return p;
}

static void parseLine(UserRecord_T *users, char *line)
{
   char *p;

   // find name
   p = strtok(line, ",");
   if (p == NULL) return;
   p = strip(p);
   strcpy(users->name, p);

   p = strtok(NULL, ",");
   if (p == NULL) return;
   p = strip(p);
   strcpy(users->ipAddr, p);

   p = strtok(NULL, ",");
   if (p == NULL) return;
   p = strip(p);
   strcpy(users->port, p);

   printf("Username: <%s>, ip address: <%s>, port: %s\r\n", users->name, users->ipAddr, users->port);
}

int readUsers(UserRecord_T *users)
{
   FILE *fp;
   char *pBuf = NULL;
   size_t len = 0;
   ssize_t numread;
   int lines = 0;

   fp = fopen(USER_FILE_NAME, "r");

   if (fp != NULL)
   {
      puts("User file successfully opened.");
      while((numread = getline(&pBuf, &len, fp)) != -1)
      {
         if(lines > 0) parseLine(&users[lines-1], pBuf);
         if(lines > 2) break;
         lines++;
      }
   }
   else
   {
      return 1;
      puts("ERROR: Unable to open user file.");
   }

   fclose(fp);
   if(pBuf) free(pBuf);

   return 0;
}
