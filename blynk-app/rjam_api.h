/*
   rjam_api.h

   An API for controlling Raspberry Jam from various UIs
*/

#ifndef RJAM_API_H
#define RJAM_API_H

#include <stdint.h>
#include "config.h"
#include "sessionInfo.h"

typedef void (*SlotConnectedCallback)(int slot);

enum {
   ServerRole,
   ClientRole
};

void RjamApi_Initialize(SlotConnectedCallback);
void RjamApi_InitializeSoundCard();
// Session info
void RjamApi_CreateNewSession();
void RjamApi_SaveAllSessionInfo();
const char * RjamApi_GetSessionName();
int RjamApi_GetAllSessionInfo(const char * pSessionName);
const char * RjamApi_GetSessionName();
void RjamApi_SetSessionName(const char * pName);
int RjamApi_GetSessionOutputLevel();
void RjamApi_SetSessionOutputLevel(int level);
int RjamApi_GetSessionInputLevel();
void RjamApi_SetSessionInputLevel(int level);
int RjamApi_GetSessionInputSelect();
void RjamApi_SetSessionInputSelect(int input);
const char * RjamApi_GetSessionMicGain();
void RjamApi_ChangeSessionMicGain();
int RjamApi_GetSessionMonitorGain();
void RjamApi_SetSessionMonitorGain(int gain);
int RjamApi_GetSessionSampleRate();
bool RjamApi_SetSessionSampleRate(int newRate);
// Slot info
void RjamApi_SetSlotGain(uint8_t slot, int gain);
int RjamApi_GetSlotGain(uint8_t slot);
void RjamApi_KillSlot(uint8_t slot);
void RjamApi_RouteSlot(uint8_t slot);
void RjamApi_OpenSlot(uint8_t slot);
bool RjamApi_SetSlotLatency(uint8_t slot, uint8_t latency);
uint8_t RjamApi_GetSlotLatency(uint8_t slot);
bool RjamApi_SetSlotIpAddress(uint8_t slot, const char *newIp);
const char * RjamApi_GetSlotIpAddress(uint8_t slot);
void RjamApi_SetSlotRole(uint8_t slot, uint8_t role);
uint8_t RjamApi_GetSlotRole(uint8_t slot);
const char * RjamApi_GetConnectionType(uint8_t slot);
bool RjamApi_isConnected(uint8_t slot);
bool RjamApi_VolumeIsEnabled(uint8_t slot);
const char * RjamApi_GetSlotName(uint8_t slot);
void RjamApi_SetSlotName(uint8_t slot, const char * pName);
int RjamApi_GetSlotPortOffset(uint8_t slot);
void RjamApi_SetSlotPortOffset(uint8_t slot, int offset);
uint8_t RjamApi_GetSlotOfSlot(uint8_t slot);
void RjamApi_SetSlotOfSlot(uint8_t slot, uint8_t dBslot);

#endif // RJAM_API_H
