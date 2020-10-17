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
void RjamApi_SetOutputLevel(int level);
void RjamApi_SetInputLevel(int level);
void RjamApi_InitializeSoundCard();
void RjamApi_InputSelect(int input);
void RjamApi_ChangeMicGain();
const char * RjamApi_GetMicGain();
void RjamApi_SetSlotGain(int slot, int gain);
void RjamApi_EcaSetup();
void RjamApi_EcaConnect(uint8_t slot);
bool RjamApi_SetSampleRate(int newRate);
void RjamApi_KillSlot(uint8_t slot);
void RjamApi_RouteSlot(uint8_t slot);
void RjamApi_OpenSlot(uint8_t slot);
bool RjamApi_SetLatencyForSlot(uint8_t slot, uint8_t latency);
bool RjamApi_SetSlotIpAddress(uint8_t slot, const char *newIp);
void RjamApi_SetSlotRole(uint8_t slot, uint8_t role);
uint8_t RjamApi_GetSlotRole(uint8_t slot);
void RjamApi_CreateNewSession();
const char * RjamApi_GetConnectionType(uint8_t slot);
bool RjamApi_isConnected(uint8_t slot);
bool RjamApi_VolumeIsEnabled(uint8_t slot);
const char * RjamApi_GetSlotName(uint8_t slot);
void RjamApi_SetSlotName(uint8_t slot, const char * pName);
int RjamApi_GetSlotPortOffset(uint8_t slot);
void RjamApi_SetSlotPortOffset(uint8_t slot, int offset);

extern SessionInfo_T sessionInfo;
extern ConnectionInfo_T connections[TOTAL_SLOTS];

#define SAMPLE_RATE_COUNT 3
extern const char *sampleRate[SAMPLE_RATE_COUNT];

#endif // RJAM_API_H
