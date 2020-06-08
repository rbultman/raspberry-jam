/**
 * @file       main.cpp
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 */

//#define BLYNK_DEBUG
#define BLYNK_PRINT stdout
#ifdef RASPBERRY
#include <BlynkApiWiringPi.h>
#else
#include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "userSpec.h"
#include "readUsers.h"

// convenience macros
#define USER1_ROLE V3
#define USER1_IPADDR V2
#define USER1_LATENCY V5
#define USER1_CONNECT V6 

#define USER2_ROLE V8
#define USER2_IPADDR V9
#define USER2_LATENCY V10
#define USER2_CONNECT V11

#define USER3_ROLE V12
#define USER3_IPADDR V13
#define USER3_LATENCY V14
#define USER3_CONNECT V15

#define ADDRESS_BOOK V16
#define SAMPLE_RATE V7
#define START_JACK V4
#define ROUTING V18
#define INPUT_LEVEL V17
#define OUTPUT_LEVEL V1
#define SOUNDCARD V19
#define INPUT_SELECT V20
#define MIC_GAIN V21

#define TOTAL_SLOTS 3

int ConnectButton[TOTAL_SLOTS] = {
   USER1_CONNECT,
   USER2_CONNECT,
   USER3_CONNECT,
};

int roleButton[TOTAL_SLOTS] = {
   USER1_ROLE,
   USER2_ROLE,
   USER3_ROLE,
};

int ipAddrBox[TOTAL_SLOTS] = {
   USER1_IPADDR,
   USER2_IPADDR,
   USER3_IPADDR,
};

int latencySlider[TOTAL_SLOTS] = {
   USER1_LATENCY, 
   USER2_LATENCY, 
   USER3_LATENCY, 
};

#define TOTAL_USERS 4
UserRecord_T users[TOTAL_USERS] = {
   {"User1", "127.0.0.1"},
   {"User2", "127.0.0.2"},
   {"User3", "127.0.0.3"},
   {"User4", "127.0.0.4"},
};

static const uint8_t defaultRoles[TOTAL_USERS][TOTAL_SLOTS] = {
   {0, 0, 0},
   {1, 0, 0},
   {1, 1, 0},
   {1, 1, 1}
};

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);

#include <BlynkWidgets.h>

static const char *auth, *serv;
static uint16_t port;
char mixMasterCommand[32] = "amixer ";
char mixCaptureCommand[32] = "amixer";
char mixCommand[100] = "amixer";
char sampleRate[6] = "96000";
char inputMicCommand[100] = "amixer ";
char inputLineCommand[100] = "amixer ";
char micGainCommand[50];
int micGainIndex =0;
bool audioInjector = false;
bool connectionState[TOTAL_SLOTS] = {false, false, false};
int currentUser = 1;

int longPressMillis = 2000;    // time in millis needed for longpress
int longPressTimer;
int longPressTimer2;
bool buttonState = false;

#define BUFFER_SIZES 10
char *rxBufferSize[BUFFER_SIZES] = {
   "2",
   "4",
   "8",
   "12",
   "16",
   "24",
   "32",
   "48",
   "64",
   "128"
};

typedef struct JacktripParams {
   char connectionType[32];
   char clientIP[20];
   char rxBufferSize[10];
   int  user;
   int  serverOffset;
   int  clientOffset;
   int  role;
} JacktripParams;

JacktripParams connectionParams[TOTAL_SLOTS] = {
   {"s"," ","8", 0, 1, 10, 0},
   {"s"," ","8", 1, 2, 20, 0},
   {"s"," ","8", 2, 3, 30, 0}
};

BlynkTimer tmr;

// function prototypes
void SetSlotRole(uint8_t slot, uint8_t role);
int GetCurrentOffset(int slot);
void KillSlot(int slot);
void KillAllSlots();
void OpenSlot(int slot);

static void sleep_millis(int millis)
{
   struct timespec req = {0};
   req.tv_sec = 0;
   req.tv_nsec = millis * 1000000L;
   nanosleep(&req, (struct timespec *)NULL);
}

int checkIpFormat(const char *ip) {
   uint16_t quad[4];
   uint8_t numread;
   uint8_t i;

   printf("Checking: %s\r\n", ip);

   numread = sscanf(ip, "%hu.%hu.%hu.%hu", &quad[0], &quad[1], &quad[2], &quad[3]);

   if (numread != 4) {
      puts("Not enough numbers entered.");
      return 1;
   }

   for(i=0; i<4; i++) {
      if (quad[i] > 255) {
         printf("Number out of range: %hu\r\n", quad[i]);
         return 2;
      } 
   }

   return 0;
}

BLYNK_CONNECTED() {
   Blynk.syncVirtual(SAMPLE_RATE); //sync sample rate on connection
   Blynk.syncVirtual(INPUT_LEVEL); //sync input level on connection
   Blynk.syncVirtual(OUTPUT_LEVEL); //sync output level on connection
   Blynk.syncVirtual(USER1_IPADDR); //sync Client 1 IP
   //	Blynk.syncVirtual(USER2_IPADDR); //sync Client 2 IP
   //	Blynk.syncVirtual(USER3_IPADDR); //sync Client 3 IP
   Blynk.syncVirtual(SOUNDCARD); //sync Soundcard selection
   Blynk.syncVirtual(INPUT_SELECT); //sync Input selection
   //Blynk.syncVirtual(ADDRESS_BOOK); 

   // initialize menu items
   //Blynk.setProperty(ADDRESS_BOOK, "labels", "Menu Item 1", "Menu Item 2", "Menu Item 3");
   Blynk.setProperty(ADDRESS_BOOK, "labels", "New Session", users[0].name, users[1].name, users[2].name, users[3].name);
   Blynk.syncVirtual(ADDRESS_BOOK); 
}

BLYNK_WRITE(OUTPUT_LEVEL) //Output Level Slider
{
   printf("New output level: %s\n", param[0].asStr());
   sprintf(mixCommand, "amixer -M set %s %s%%", mixMasterCommand, param[0].asStr());
   system(mixCommand);
   printf("%s\r\n",mixCommand);
}

BLYNK_WRITE(INPUT_LEVEL)  //Input Level Slider
{
   printf("New input level: %s\n", param[0].asStr());
   sprintf(mixCommand, "amixer -M set %s %s%%", mixCaptureCommand, param[0].asStr());
   system(mixCommand);

}

void SetLatencyForSlot(uint8_t slot, uint8_t latency)
{
   if (latency >= BUFFER_SIZES) {
      printf("Buffer size inxex out of range.\r\n");
   } else {
      printf("New latency for slot %d: %s \r\n", slot, rxBufferSize[latency]);
      strcpy(connectionParams[slot].rxBufferSize, rxBufferSize[latency]);
      if (connectionState[slot]) 
      {
         KillSlot(slot);
         OpenSlot(slot);
      }
   }
}

BLYNK_WRITE(USER1_LATENCY) // Connection 1 buffer adjustment
{
   SetLatencyForSlot(0, param.asInt() - 1);
}

BLYNK_WRITE(USER2_LATENCY) // Connection 2 buffer adjustment
{
   SetLatencyForSlot(1, param.asInt() - 1);
}

BLYNK_WRITE(USER3_LATENCY) // Connection 3 buffer adjustment
{
   SetLatencyForSlot(2, param.asInt() - 1);
}

void UserSelected(uint8_t user)
{
   uint8_t i;
   uint8_t slot=0;

   printf("The select user is index %d.\r\n", user);

   KillAllSlots();

   currentUser = user;

   for (i=0; i<4; i++) 
   {
      if (i == user) 
      {
         continue;
      }
      // set parameters
      connectionParams[slot].serverOffset = user * 10 + i;
      connectionParams[slot].clientOffset = i * 10 + user;
      connectionParams[slot].user = i;
      strcpy(connectionParams[slot].clientIP, users[i].ipAddr);
      SetSlotRole(slot, defaultRoles[user][slot]);

      Blynk.setProperty(roleButton[slot], "label", users[i].name);
      Blynk.virtualWrite(ipAddrBox[slot], connectionParams[slot].clientIP);
      Blynk.virtualWrite(latencySlider[slot],4);
      Blynk.virtualWrite(roleButton[slot], connectionParams[slot].role);
      slot++;
   }
}

BLYNK_WRITE(ADDRESS_BOOK) //Address Book
{
   printf("Got address book value write: %d \r\n", param.asInt());
   int user = param.asInt();
   switch (user)
   {
      case 1: // New Session
         Blynk.virtualWrite(USER1_IPADDR,"\r");
         Blynk.syncVirtual(USER1_IPADDR);

         Blynk.virtualWrite(USER2_IPADDR,"\r");
         Blynk.syncVirtual(USER2_IPADDR);

         Blynk.virtualWrite(USER3_IPADDR,"\r");
         Blynk.syncVirtual(USER3_IPADDR);

         Blynk.virtualWrite(USER1_ROLE,LOW);
         Blynk.syncVirtual(USER1_ROLE);
         Blynk.setProperty(USER1_ROLE,"label","Connection 1");

         Blynk.virtualWrite(USER2_ROLE,LOW);
         Blynk.syncVirtual(USER2_ROLE);
         Blynk.setProperty(USER2_ROLE,"label","Connection 2");

         Blynk.virtualWrite(USER3_ROLE,LOW);
         Blynk.syncVirtual(USER3_ROLE);
         Blynk.setProperty(USER3_ROLE,"label","Connection 3");

         Blynk.virtualWrite(USER1_LATENCY,4);
         Blynk.syncVirtual(USER1_LATENCY);

         Blynk.virtualWrite(USER2_LATENCY,4);
         Blynk.syncVirtual(USER2_LATENCY);

         Blynk.virtualWrite(USER3_LATENCY,4);
         Blynk.syncVirtual(USER3_LATENCY);	
         break;

      case 2:
      case 3:
      case 4:
      case 5:
         UserSelected(user-2);
         break;
      default:
         printf("Unknown item selected \r\n");
   }
}

BLYNK_WRITE(SAMPLE_RATE) // Sampe Rate setting
{
   char jackCommand[128] = "jackd";

   printf("New sample rate: %d\r\n", param.asInt());
   switch (param.asInt())
   {
      case 1: // Item 1
         sprintf(sampleRate,"44100");
         break;
      case 2: // Item 2
         sprintf(sampleRate,"48000");
         break;
      case 3: // Item 3
         sprintf(sampleRate,"96000");
         break;
      default:
         printf("Unknown item selected \r\n");
   }
   printf("%s \r\n",sampleRate);
   KillAllSlots();
   sprintf(jackCommand,"sh /home/pi/start_jack.sh -r%s &",sampleRate);
   system(jackCommand);	
}

BLYNK_WRITE(ROUTING)  //Route to both ears button
{
   char routeCommand[128] = "jack_connect";
   printf("New routing: %s\n", param[0].asStr());

   sprintf(routeCommand,"jack_connect %s:receive_1 system:playback_2",connectionParams[0].clientIP);
   printf("%s\r\n",routeCommand);
   system(routeCommand);
   sprintf(routeCommand,"jack_connect %s:receive_1 system:playback_2",connectionParams[1].clientIP);
   printf("%s\r\n",routeCommand);
   system(routeCommand);
   sprintf(routeCommand,"jack_connect %s:receive_1 system:playback_2",connectionParams[2].clientIP);
   printf("%s\r\n",routeCommand);
   system(routeCommand);

   printf("jack_connect JackTrip:receive_1 system:playback_2");
   system("jack_connect JackTrip:receive_1 system:playback_2");
   printf("jack_connect JackTrip-01:receive_1 system:playback_2");
   system("jack_connect JackTrip-01:receive_1 system:playback_2");
   printf("jack_connect JackTrip-02:receive_1 system:playback_2");
   system("jack_connect JackTrip-02:receive_1 system:playback_2");

   //Connect/Disconnect local monitoring
   if (param[0])
   {
      system("jack_connect system:capture_1 system:playback_1 && jack_connect system:capture_2 system:playback_2");
   }
   else
   {
      system("jack_disconnect system:capture_1 system:playback_1 && jack_disconnect system:capture_2 system:playback_2");
   }
}


int GetCurrentOffset(int slot)
{
   int offset;

   if (connectionParams[slot].role == 1)
   {
      offset = connectionParams[slot].clientOffset;
   }
   else
   {
      offset = connectionParams[slot].serverOffset;
   }

   return offset;
}

void OpenSlot(int slot)
{
   char jacktripCommand[128];
   int offset = GetCurrentOffset(slot);

   Blynk.virtualWrite(ConnectButton[slot] ,HIGH);
   sprintf(jacktripCommand, "jacktrip -o%d -n1 -z -%s -q%s &", offset, connectionParams[slot].connectionType, connectionParams[slot].rxBufferSize);
   printf("%s\r\n",jacktripCommand);
   system(jacktripCommand);
   connectionState[slot] = true;
}

void KillSlot(int slot)
{
   char killcmd[128];
   int offset = GetCurrentOffset(slot);

   Blynk.virtualWrite(ConnectButton[slot] ,LOW);
   //send kill command
   sprintf(killcmd, "kill `ps aux | grep \"[j]acktrip -o%d\" | awk '{print $2}'`", offset);
   system(killcmd);
   connectionState[slot] = false;
   // sleep a little so the process dies and jackd recovers
   sleep_millis(250);
}

void KillAllSlots()
{
   int i;

   for(i=0; i<TOTAL_SLOTS; i++)
   {
      if (connectionState[i])
      {
         KillSlot(i);
      }
   }
}

void DisconnectSlot(void *pSlot)
{
   uint32_t slot = (uint32_t)pSlot;

   printf("Disconnecting slot: %d\r\n", slot);
   KillSlot(slot);
   printf("Slot %d disconnected\r\n", slot+1);
}

void ConnectSlot(int slot, int pressed)
{
   if (pressed)
   {
      KillSlot(slot);
      OpenSlot(slot);
      longPressTimer = tmr.setTimeout(2000, DisconnectSlot, (void *)slot);
   }
   else
   {	
      tmr.deleteTimer(longPressTimer);
   }
}

BLYNK_WRITE(USER1_CONNECT ) //Connection 1 Connect button
{
   ConnectSlot(0, param[0].asInt());
}


BLYNK_WRITE(USER2_CONNECT) //Connection 2 Connect button
{
   ConnectSlot(1, param[0].asInt());
}

BLYNK_WRITE(USER3_CONNECT) //Connection 3 Connect button
{
   ConnectSlot(2, param[0].asInt());
}

void SetSlotIpAddress(uint8_t slot, const char *newIp) {
   int ipCheck = 1;
   printf("New IP address for slot 1: %s\n", newIp);
   ipCheck=checkIpFormat(newIp);
   printf("ipCheck %d\r\n", ipCheck);
   if (ipCheck == 0)
   {	
      strcpy(connectionParams[slot].clientIP, newIp);
      Blynk.virtualWrite(roleButton[slot], HIGH);
      connectionParams[slot].role = 1;
      sprintf(connectionParams[slot].connectionType, "c%s", connectionParams[slot].clientIP);
      printf("<%s>\r\n",connectionParams[slot].clientIP);
   }
   else
   {
      Blynk.virtualWrite(ipAddrBox[slot],"\r");
   }
}

BLYNK_WRITE(USER1_IPADDR) //Connection 1 Client IP
{
   if(param[0].isEmpty()) {puts("param was empty"); return;}
   SetSlotIpAddress(0, param[0].asStr());
}

BLYNK_WRITE(USER2_IPADDR) //Connection 2 Client IP
{
   if(param[0].isEmpty()) {puts("param was empty"); return;}
   SetSlotIpAddress(1, param[0].asStr());
}

BLYNK_WRITE(USER3_IPADDR) //Connection 3 Client IP
{
   if(param[0].isEmpty()) {puts("param was empty"); return;}
   SetSlotIpAddress(2, param[0].asStr());
}

void SetSlotRole(uint8_t slot, uint8_t role)
{
   bool restartNeeded = false;

   printf("New role for slot %d: %d\n", slot, role);
   if (connectionState[slot])
   {
      KillSlot(slot);
      restartNeeded = true;
   }
   if (role == 1)
   {
      sprintf(connectionParams[slot].connectionType,"c%s", connectionParams[slot].clientIP);
   }
   else
   {
      sprintf(connectionParams[slot].connectionType,"s");
      Blynk.setProperty(START_JACK, "color", "D3435C");
   }
   connectionParams[slot].role = role;
   printf("Slot %d connection type is now: %s\r\n", slot, connectionParams[slot].connectionType);
   if (restartNeeded)
   {
      OpenSlot(slot);
   }
}

BLYNK_WRITE(USER1_ROLE) //Connection 1 Connection Type
{
   SetSlotRole(0, param[0].asInt());
}

BLYNK_WRITE(USER2_ROLE) //Connection 2 Connection Type
{
   SetSlotRole(1, param[0].asInt());
}

BLYNK_WRITE(USER3_ROLE) //Connection 3 Connection Type
{
   SetSlotRole(2, param[0].asInt());
}

BLYNK_WRITE(START_JACK) //Start Jack button
{
   char jackCommand[128] = "jackd";

   if (param[0])
   {
      printf("Start_Jam \r\n");
      KillAllSlots();
      sprintf(jackCommand,"sh /home/pi/start_jack.sh -r%s &",sampleRate);
      system(jackCommand);
   }
}

BLYNK_WRITE(SOUNDCARD) //Soundcard Selection
{
   switch (param.asInt())
   {
      case 1: // Audio Injector Stereo
         system("amixer set 'Output Mixer HiFi' on");
         sprintf(inputMicCommand,"amixer set 'Input Mux' 'Mic' && amixer set 'Mic' cap && amixer set 'Line' nocap");
         sprintf(inputLineCommand,"amixer set 'Input Mux' 'Line In' && amixer set 'Line' cap && amixer set 'Mic' nocap");
         sprintf(mixMasterCommand,"'Master'");
         sprintf(mixCaptureCommand,"'Capture'");
         sprintf(micGainCommand,"amixer set 'Mic Boost' ");
         audioInjector=true;
         break;
      case 2: // Fe-Pi
         audioInjector=false;
         system("amixer -M set 'Lineout' 100%");
         system("amixer -M set 'Headphone' 100%");
         system("amixer set 'Headphone Mux' 'DAC'");
         sprintf(inputMicCommand,"amixer set 'Capture Mux' 'MIC_IN'");
         sprintf(inputLineCommand,"amixer set 'Capture Mux' 'LINE_IN'");
         sprintf(mixMasterCommand,"'PCM'");
         sprintf(mixCaptureCommand,"'Capture'");
         sprintf(micGainCommand,"amixer set 'Mic' ");
         break;
      case 3: // Hifiberry DAC +ADC

         break;
      default:
         printf("Unknown item selected \r\n");
   }

}

BLYNK_WRITE(INPUT_SELECT) //Input Selection
{
   printf("New input selected: %s\n", param[0].asStr());
   if (param[0])
   {
      system(inputMicCommand);
   }
   else
   {
      system(inputLineCommand);
   }

}

BLYNK_WRITE(MIC_GAIN) //Mic Gain
{
   printf("Mic gain changed: %s\n", param[0].asStr());
   if (param[0])
   {
      micGainIndex++;
      switch (micGainIndex)
      {
         case 1: // +20dB
            Blynk.setProperty(MIC_GAIN,"offLabel", "+20dB");
            sprintf(mixCommand,"%s %u",micGainCommand,micGainIndex);
            printf("%s\r\n",mixCommand);
            system(mixCommand);
            break;
         case 2: // +30dB
            if (audioInjector) 
            {
               micGainIndex=0;
               Blynk.setProperty(MIC_GAIN,"offLabel", "0dB");
            }
            else
            {	
               Blynk.setProperty(MIC_GAIN,"offLabel", "+30dB");
            }
            sprintf(mixCommand,"%s %u",micGainCommand,micGainIndex);
            printf("%s\r\n",mixCommand);
            system(mixCommand);
            break;
         case 3: // +40dB
            Blynk.setProperty(MIC_GAIN,"offLabel", "+40dB");
            sprintf(mixCommand,"%s %u",micGainCommand,micGainIndex);
            printf("%s\r\n",mixCommand);
            system(mixCommand);
            break;
         case 4: // Reset to 0 dB
            Blynk.setProperty(MIC_GAIN,"offLabel", "0dB");
            micGainIndex=0;
            sprintf(mixCommand,"%s %u",micGainCommand,micGainIndex);
            printf("%s\r\n",mixCommand);
            system(mixCommand);
            break;
         default:
            printf("Unknown item selected \r\n");
      }
   }
}


void setup()
{
   Blynk.begin(auth, serv, port);
   tmr.setInterval(1000, [](){
         Blynk.virtualWrite(V0, BlynkMillis()/1000);
         });
}

void loop()
{
   Blynk.run();
   tmr.run();
}


int main(int argc, char* argv[])
{
   readUsers(&users[0]);

   puts("User info read, continuing...");

   parse_options(argc, argv, auth, serv, port);

   setup();

   while(true) {
      loop();
   }

   return 0;
}
