/*
   rjam_api.h

   An API for controlling Raspberry Jam from various UIs
*/

#ifndef RJAM_API_H
#define RJAM_API_H

#include "sessionInfo.h"

void RjamApi_SetOutputLevel(int level);
void RjamApi_SetInputLevel(int level);
void RjamApi_InitializeSoundCard();
void RjamApi_InputSelect(int input);
void RjamApi_ChangeMicGain();
const char * RjamApi_GetMicGain();

extern SessionInfo_T sessionInfo;

#endif // RJAM_API_H
