
#ifndef SESSION_DB_H
#define SESSION_DB_H

#include "sessionInfo.h"

const unsigned char * GetFirstSessionName();
const unsigned char * GetNextSessionName();
int GetAllSessionInfo(const char *sessionName, SessionInfo_T * pSessionInfo, ConnectionInfo_T * pConnections);

#endif // SESSION_DB_H
