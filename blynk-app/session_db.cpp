#include <stdio.h>
#include <sqlite3.h> 
#include "session_db.h"

/*
CREATE TABLE session(sessionId INTEGER primary key autoincrement, name varchar(64), inputLevel int, outputLevel int, sampleRate int, monitorGain int, inputSelect int, micBoost int);
CREATE TABLE connection(id INTEGER primary key autoincrement, sessionId INTEGER, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int);
 */

static const char dbFile[] = "../settings/sessions.db";
static sqlite3 *db;
sqlite3_stmt *stmt;

const unsigned char * GetFirstSessionName() {
   int rc;
   char sql[] = "SELECT name FROM session;";
   const unsigned char* name;

   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\r\n", sqlite3_errmsg(db));
      return NULL;
   }

   name = NULL;

   rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
   if (SQLITE_OK != rc) {
      puts("Some error occurred.");
   } else {
      name = GetNextSessionName();
   }

   sqlite3_close(db);

   return name;
}

const unsigned char * GetNextSessionName() {
   int rc;
   const unsigned char* name = NULL;

   rc = sqlite3_step(stmt);

   if (rc == SQLITE_ROW) {
      name = sqlite3_column_text(stmt, 0);
   } else {
      sqlite3_finalize(stmt);  
      sqlite3_close(db);
   }

   return name;
}

