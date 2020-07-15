
#ifndef SESSION_DB_H
#define SESSION_DB_H

#include "sessionInfo.h"

const unsigned char * GetFirstSessionName();
const unsigned char * GetNextSessionName();
int GetAllSessionInfo(const char *sessionName, SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections);
int SaveAllSessionInfo(SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections);
int DeleteSessionInfo(const char * sessionName);

#endif // SESSION_DB_H
