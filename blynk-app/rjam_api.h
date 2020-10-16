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
void RjamApi_KillSlot(int slot);
void RjamApi_RouteSlot(int slot);
void RjamApi_OpenSlot(int slot);
bool RjamApi_SetLatencyForSlot(uint8_t slot, uint8_t latency);
bool RjamApi_SetSlotIpAddress(uint8_t slot, const char *newIp);
void RjamApi_SetSlotRole(uint8_t slot, uint8_t role);

extern SessionInfo_T sessionInfo;
extern ConnectionInfo_T connections[TOTAL_SLOTS];

#define SAMPLE_RATE_COUNT 3
extern const char *sampleRate[SAMPLE_RATE_COUNT];

typedef struct JacktripParams {
   char connectionType[32];
   bool isConnected;
   bool volumeIsEnabled;
   bool pollForClient;
} JacktripParams;
extern JacktripParams connectionParams[TOTAL_SLOTS];

#endif // RJAM_API_H
