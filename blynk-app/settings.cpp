#include <stdio.h>
#include <sqlite3.h> 
#include <string.h>
#include <stdlib.h>
#include "settings.h"

static const char dbFile[] = "../settings/sessions.db";
static sqlite3 *db;
static sqlite3_stmt *stmt;

unsigned int GetSelectedSoundcard() {
   int rc;
   char sql[] = "SELECT selectedCard FROM settings where id=1;";
   int card = -1;

   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\r\n", sqlite3_errmsg(db));
      return -1;
   }

   rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   if (SQLITE_OK != rc) {
      puts("Some error occurred preparing the statement.");
   } else {
      rc = sqlite3_step(stmt);

      if (rc == SQLITE_ROW) {
         card = sqlite3_column_int(stmt, 0);
         printf("The card is %d\r\n", card);
      }

      sqlite3_finalize(stmt);  
   }

   sqlite3_close(db);

   return card;
}

