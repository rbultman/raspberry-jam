#include <stdio.h>
#include <sqlite3.h> 
#include <string.h>
#include <stdlib.h>
#include "session_db.h"

/*
CREATE TABLE session(name varchar(64) primary key, inputLevel int, outputLevel int, sampleRate int, monitorGain int, inputSelect int, micBoost int);
CREATE TABLE connection(session_name varchar(64), slot int, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int, PRIMARY KEY (session_name, slot));
*/

static const char dbFile[] = "../settings/sessions.db";
static sqlite3 *db;
static sqlite3_stmt *stmt;
static int connectionIndex = 0;

#define MAX_CONNECTIONS 3

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
      puts("Some error occurred preparing the statement.");
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

static int SessionCallback(void *data, int argc, char **argv, char **azColName){
   int i;
   SessionInfo_T * pSessionInfo = (SessionInfo_T *)data;

   for(i = 0; i<argc; i++){
      if (0 == strcmp("name", azColName[i])) {
         strcpy(pSessionInfo->name, argv[i]);
      } else if (0 == strcmp("inputLevel", azColName[i])) {
         pSessionInfo->inputLevel = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("outputLevel", azColName[i])) {
         pSessionInfo->outputLevel = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("sampleRate", azColName[i])) {
         pSessionInfo->sampleRate = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("monitorGain", azColName[i])) {
         pSessionInfo->monitorGain = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("inputSelect", azColName[i])) {
         pSessionInfo->inputSelect = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("micBoost", azColName[i])) {
         pSessionInfo->micBoost = strtol(argv[i], NULL, 10);
      } else {
         printf("ERROR: Unknown column name: <%s>\r\n", azColName[i]);
         return 1;
      }
   }

   return 0;
}

static int GetSessionInfo(const char * sessionName, SessionInfo_T * pSessionInfo) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   int retval = 0;

   /* Open database */
   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }

   /* Create SQL statement */
   sprintf(sql, "SELECT * from session where name = '%s'", sessionName);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, SessionCallback, (void*)pSessionInfo, &zErrMsg);

   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      retval = 1;
   }

   sqlite3_close(db);

   return retval;
}

//CREATE TABLE connection(session_name varchar(64), slot int, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int, PRIMARY KEY (session_name, slot));
static int ConnectionCallback(void *data, int argc, char **argv, char **azColName){
   int i;
   ConnectionInfo_T * pConnection = (ConnectionInfo_T *)data;

   if (connectionIndex >= MAX_CONNECTIONS) {
      return 1;
   }

   for(i = 0; i<argc; i++){
      if (0 == strcmp("name", azColName[i])) {
         strcpy(pConnection[connectionIndex].name, argv[i]);
      } else if (0 == strcmp("ipAddr", azColName[i])) {
         strcpy(pConnection[connectionIndex].ipAddr, argv[i]);
      } else if (0 == strcmp("port", azColName[i])) {
         pConnection[connectionIndex].portOffset = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("role", azColName[i])) {
         pConnection[connectionIndex].role = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("latency", azColName[i])) {
         pConnection[connectionIndex].latency = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("gain", azColName[i])) {
         pConnection[connectionIndex].gain = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("slot", azColName[i])) {
         pConnection[connectionIndex].slot = strtol(argv[i], NULL, 10);
      } else if (0 == strcmp("session_name", azColName[i])) {
      } else {
         printf("ERROR: Unknown connection column name: <%s>\r\n", azColName[i]);
         return 1;
      }
   }

   connectionIndex++;

   return 0;
}

static int GetConnectionInfoForSession(const char * sessionName, ConnectionInfo_T * pConnections) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   int retval = 0;

   connectionIndex = 0;

   /* Open database */
   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }

   /* Create SQL statement */
   sprintf(sql, "SELECT * from connection where session_name = '%s'", sessionName);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, ConnectionCallback, (void*)pConnections, &zErrMsg);

   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      retval = 1;
   }

   sqlite3_close(db);

   return retval;
}

int GetAllSessionInfo(const char *sessionName, SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections) {
   int retval = 0;

   retval = GetSessionInfo(sessionName, pSessionInfo);
   if (retval == 0) {
      GetConnectionInfoForSession(sessionName, pConnections);
   }

   return retval;
}

//CREATE TABLE connection(session_name varchar(64), slot int, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int, PRIMARY KEY (session_name, slot));
static int SaveConnectionInfo(SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   int retval = 0;
   int i;

   /* Open database */
   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }

   for(i=0; i<MAX_CONNECTIONS; i++) {
      sprintf(sql, "REPLACE into connection(session_name, slot, name, ipAddr, port, role, latency, gain) " \
            "VALUES('%s', %d, '%s', '%s', %d, %d, %d, %d)", 
            pSessionInfo->name, pConnections[i].slot, pConnections[i].name, pConnections[i].ipAddr, pConnections[i].portOffset, 
            pConnections[i].role, pConnections[i].latency, pConnections[i].gain);

      /* Execute SQL statement */
      rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);

      if( rc != SQLITE_OK ) {
         fprintf(stderr, "SQL error: %s\n", zErrMsg);
         sqlite3_free(zErrMsg);
         retval = 1;
      }
   }

   sqlite3_close(db);

   return retval;
}

//CREATE TABLE session(sessionId INTEGER primary key autoincrement, name varchar(64), inputLevel int, outputLevel int, sampleRate int, monitorGain int, inputSelect int, micBoost int);
static int SaveSessionInfo(SessionInfo_T * pSessionInfo) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   int retval = 0;

   /* Open database */
   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }

   /* Create SQL statement */
   sprintf(sql, "REPLACE into session(name, inputLevel, outputLevel, sampleRate, monitorGain, inputSelect, micBoost) " \
         "VALUES('%s', %d, %d, %d, %d, %d, %d)", \
         pSessionInfo->name, pSessionInfo->inputLevel, pSessionInfo->outputLevel, pSessionInfo->sampleRate, \
         pSessionInfo->monitorGain, pSessionInfo->inputSelect, pSessionInfo->micBoost);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);

   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      retval = 1;
   }

   sqlite3_close(db);

   return retval;
}

int SaveAllSessionInfo(SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections) {
   int retval = 0;

   SaveSessionInfo(pSessionInfo);
   SaveConnectionInfo(pSessionInfo, pConnections);

   return retval;
}

int DeleteSessionInfo(const char * sessionName) {
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256];
   int retval = 0;

   /* Open database */
   rc = sqlite3_open(dbFile, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(1);
   }

   /* Create SQL statement */
   sprintf(sql, "DELETE FROM session WHERE name = '%s'", sessionName);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);

   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error deleting from session table: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      retval = 1;
   }

   /* Create SQL statement */
   sprintf(sql, "DELETE FROM connection WHERE session_name = '%s'", sessionName);

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);

   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error deleting from connection table: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      retval = 1;
   }

   sqlite3_close(db);

   return retval;
}

