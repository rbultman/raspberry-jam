#include <stdio.h>
#include <sqlite3.h> 
#include <string.h>
#include <stdlib.h>
#include "session_db.h"

/*
CREATE TABLE session(sessionId INTEGER primary key autoincrement, name varchar(64), inputLevel int, outputLevel int, sampleRate int, monitorGain int, inputSelect int, micBoost int);
CREATE TABLE connection(id INTEGER primary key autoincrement, sessionId INTEGER, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int);
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

static int SessionCallback(void *data, int argc, char **argv, char **azColName){
   int i;
   SessionInfo_T * pSessionInfo = (SessionInfo_T *)data;

   for(i = 0; i<argc; i++){
      if (0 == strcmp("name", azColName[i])) {
         strcpy(pSessionInfo->name, argv[i]);
         printf("Session name: %s\r\n", pSessionInfo->name);
      } else if (0 == strcmp("inputLevel", azColName[i])) {
         pSessionInfo->inputLevel = strtol(argv[i], NULL, 10);
         printf("Input Level: %d\r\n", pSessionInfo->inputLevel);
      } else if (0 == strcmp("outputLevel", azColName[i])) {
         pSessionInfo->outputLevel = strtol(argv[i], NULL, 10);
         printf("Output Level: %d\r\n", pSessionInfo->outputLevel);
      } else if (0 == strcmp("sampleRate", azColName[i])) {
         pSessionInfo->sampleRate = strtol(argv[i], NULL, 10);
         printf("Sample Rate: %d\r\n", pSessionInfo->sampleRate);
      } else if (0 == strcmp("monitorGain", azColName[i])) {
         pSessionInfo->monitorGain = strtol(argv[i], NULL, 10);
         printf("Monitor Gain: %d\r\n", pSessionInfo->monitorGain);
      } else if (0 == strcmp("inputSelect", azColName[i])) {
         pSessionInfo->inputSelect = strtol(argv[i], NULL, 10);
         printf("Input Select: %d\r\n", pSessionInfo->inputSelect);
      } else if (0 == strcmp("micBoost", azColName[i])) {
         pSessionInfo->micBoost = strtol(argv[i], NULL, 10);
         printf("Mic Boost: %d\r\n", pSessionInfo->micBoost);
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

//CREATE TABLE connection(id INTEGER primary key autoincrement, sessionId INTEGER, name varchar(64), ipAddr varchar(20), port int, role int, latency int, gain int);
static int ConnectionCallback(void *data, int argc, char **argv, char **azColName){
   int i;
   ConnectionInfo_T * pConnection = (ConnectionInfo_T *)data;

   if (connectionIndex >= MAX_CONNECTIONS) {
      return 1;
   }

   for(i = 0; i<argc; i++){
      if (0 == strcmp("name", azColName[i])) {
         strcpy(pConnection[connectionIndex].name, argv[i]);
         printf("Connection name: %s\r\n", pConnection[connectionIndex].name);
      } else if (0 == strcmp("ipAddr", azColName[i])) {
         strcpy(pConnection[connectionIndex].ipAddr, argv[i]);
         printf("IP Address: %s\r\n", pConnection[connectionIndex].ipAddr);
      } else if (0 == strcmp("port", azColName[i])) {
         pConnection[connectionIndex].port = strtol(argv[i], NULL, 10);
         printf("Port: %d\r\n", pConnection[connectionIndex].port);
      } else if (0 == strcmp("role", azColName[i])) {
         pConnection[connectionIndex].role = strtol(argv[i], NULL, 10);
         printf("Role: %d\r\n", pConnection[connectionIndex].role);
      } else if (0 == strcmp("latency", azColName[i])) {
         pConnection[connectionIndex].latency = strtol(argv[i], NULL, 10);
         printf("Latency: %d\r\n", pConnection[connectionIndex].latency);
      } else if (0 == strcmp("gain", azColName[i])) {
         pConnection[connectionIndex].gain = strtol(argv[i], NULL, 10);
         printf("Gain: %d\r\n", pConnection[connectionIndex].gain);
      } else if (0 == strcmp("id", azColName[i])) {
         printf("ID: %s\r\n", argv[i]);
      } else if (0 == strcmp("session_name", azColName[i])) {
         printf("Session: %s\r\n", argv[i]);
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

