
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
#include <libecasoundc/ecasoundc.h>
#include "userSpec.h"
#include "readUsers.h"

#define TOTAL_SLOTS 3

// define the controls
#define SLOT1_EDIT_BUTTON     V28
#define SLOT1_ROLE_BUTTON     V3
#define SLOT1_LATENCY         V5
#define SLOT1_GAIN_SLIDER     V22
#define SLOT1_START_BUTTON    V6

#define SLOT2_EDIT_BUTTON     V31
#define SLOT2_ROLE_BUTTON     V8
#define SLOT2_LATENCY V9
#define SLOT2_GAIN_SLIDER     V24
#define SLOT2_START_BUTTON    V11

#define SLOT3_EDIT_BUTTON     V32
#define SLOT3_ROLE_BUTTON     V12
#define SLOT3_LATENCY V14
#define SLOT3_GAIN_SLIDER     V25
#define SLOT3_START_BUTTON    V15

#define LEFT_EDIT_FIELD_TEXT_BOX  V27
#define RIGHT_EDIT_FIELD_TEXT_BOX V29
#define SESSION_DROP_DOWN         V16
#define SESSION_SAVE_BUTTON       V30
#define ROUTING                   V18
#define SAMPLE_RATE               V7
#define MONITOR_GAIN_SLIDER       V26
#define INPUT_LEVEL               V17
#define OUTPUT_LEVEL              V1
#define SOUNDCARD                 V19
#define INPUT_SELECT              V20
#define MIC_GAIN                  V21

int connectButton[TOTAL_SLOTS] = {
   SLOT1_START_BUTTON,
   SLOT2_START_BUTTON,
   SLOT3_START_BUTTON,
};

int roleButton[TOTAL_SLOTS] = {
   SLOT1_ROLE_BUTTON,
   SLOT2_ROLE_BUTTON,
   SLOT3_ROLE_BUTTON,
};

int editButton[TOTAL_SLOTS] = {
   SLOT1_EDIT_BUTTON,
   SLOT2_EDIT_BUTTON,
   SLOT3_EDIT_BUTTON,
};

int latencySlider[TOTAL_SLOTS] = {
   SLOT1_LATENCY,
   SLOT2_LATENCY,
   SLOT3_LATENCY,
};

int gainSlider[TOTAL_SLOTS] = {
   SLOT1_GAIN_SLIDER,
   SLOT2_GAIN_SLIDER,
   SLOT3_GAIN_SLIDER,
};

UserRecord_T users[TOTAL_SLOTS] = {
   {"User1", "127.0.0.1", "10"},
   {"User2", "127.0.0.2", "10"},
   {"User3", "127.0.0.3", "10"},
};

static const uint8_t defaultRoles[TOTAL_SLOTS][TOTAL_SLOTS] = {
   {0, 0, 0},
   {1, 0, 0},
   {1, 1, 0},
};

int slotBeingEdited = -1;

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
char ecaCommand[100];
char connectionName[50];
char connectedSlots[64];
int micGainIndex =0;
bool audioInjector = false;
bool connectionState[TOTAL_SLOTS] = {false, false, false};
bool enableVolume[TOTAL_SLOTS] = {false, false, false};
bool myVolumeIsEnabled = false;

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
   {"s --clientname slot0"," ","8", 0, 1, 10, 0},
   {"s --clientname slot1"," ","8", 1, 2, 20, 0},
   {"s --clientname slot2"," ","8", 2, 3, 30, 0}
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
   int i;

   Blynk.syncVirtual(SAMPLE_RATE); //sync sample rate on connection
   Blynk.syncVirtual(INPUT_LEVEL); //sync input level on connection
   Blynk.syncVirtual(OUTPUT_LEVEL); //sync output level on connection
   Blynk.syncVirtual(SOUNDCARD); //sync Soundcard selection
   Blynk.syncVirtual(INPUT_SELECT); //sync Input selection

   for (i=0; i<TOTAL_SLOTS; i++) {
      Blynk.syncVirtual(roleButton[i]); //sync Client 1 IP
   }

   // initialize menu items
   Blynk.setProperty(SESSION_DROP_DOWN, "labels", "New Session", users[0].name, users[1].name, users[2].name);
   Blynk.syncVirtual(SESSION_DROP_DOWN); 

   // set connection states to off
   for (i=0; i<TOTAL_SLOTS; i++) {
      Blynk.virtualWrite(connectButton[i], LOW);
   }

   Blynk.virtualWrite(SLOT1_GAIN_SLIDER, 0);
   Blynk.virtualWrite(SLOT2_GAIN_SLIDER, 0);
   Blynk.virtualWrite(SLOT3_GAIN_SLIDER, 0);
   Blynk.virtualWrite(MONITOR_GAIN_SLIDER, 0);
   Blynk.virtualWrite(ROUTING, LOW);
   Blynk.syncVirtual(ROUTING);
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

BLYNK_WRITE(SLOT1_LATENCY) // Connection 1 buffer adjustment
{
   SetLatencyForSlot(0, param.asInt() - 1);
}

BLYNK_WRITE(SLOT2_LATENCY) // Connection 2 buffer adjustment
{
   SetLatencyForSlot(1, param.asInt() - 1);
}

BLYNK_WRITE(SLOT3_LATENCY) // Connection 3 buffer adjustment
{
   SetLatencyForSlot(2, param.asInt() - 1);
}

void UserSelected(uint8_t user)
{
   uint8_t i;
   uint8_t slot=0;
   char ip[32];

   printf("The select user is index %d.\r\n", user);

   KillAllSlots();

   for (i=0; i<TOTAL_SLOTS; i++) 
   {
      // set parameters
      connectionParams[slot].serverOffset = user * 10 + i;
      connectionParams[slot].clientOffset = i * 10 + user;
      connectionParams[slot].user = i;
      strcpy(connectionParams[slot].clientIP, users[i].ipAddr);
      SetSlotRole(slot, defaultRoles[user][slot]);

      Blynk.setProperty(connectButton[slot], "label", users[i].name);
      sprintf(ip, "%s:%s", users[slot].ipAddr, users[slot].port);
      Blynk.setProperty(roleButton[slot], "label", ip);
      Blynk.virtualWrite(latencySlider[slot],4);
      Blynk.virtualWrite(roleButton[slot], connectionParams[slot].role);
      slot++;
   }
}

BLYNK_WRITE(SESSION_DROP_DOWN) //Address Book
{
   int i;
   int user = param.asInt();
   char label[] = "Connection 1";

   printf("Got address book value write: %d \r\n", param.asInt());

   switch (user)
   {
      case 1: // New Session
         for (i=0; i<TOTAL_SLOTS; i++) {
            Blynk.virtualWrite(roleButton[i], LOW);
            Blynk.syncVirtual(roleButton[i]);
            Blynk.setProperty(roleButton[i], "label", "");

            Blynk.virtualWrite(editButton[i], LOW);
            Blynk.syncVirtual(editButton[i]);
            sprintf(label, "Connection %d", i);
            Blynk.setProperty(editButton[i], "label", label);

            Blynk.virtualWrite(latencySlider[i], 4);
            Blynk.syncVirtual(SLOT1_LATENCY);
         }
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
   sprintf(jackCommand,"sh start_jack.sh -r%s &",sampleRate);
   system(jackCommand);	
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

   Blynk.virtualWrite(connectButton[slot] ,HIGH);
   sprintf(jacktripCommand, "jacktrip -o%d -n1 -z -%s -q%s &", offset, connectionParams[slot].connectionType, connectionParams[slot].rxBufferSize);
   printf("%s\r\n",jacktripCommand);
   system(jacktripCommand);
   connectionState[slot] = true;
}

void KillSlot(int slot)
{
   char killcmd[128];
   int offset = GetCurrentOffset(slot);

   Blynk.virtualWrite(connectButton[slot] ,LOW);
   enableVolume[slot] = false;
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

BLYNK_WRITE(SLOT1_START_BUTTON ) //Connection 1 Connect button
{
   ConnectSlot(0, param[0].asInt());
}


BLYNK_WRITE(SLOT2_START_BUTTON) //Connection 2 Connect button
{
   ConnectSlot(1, param[0].asInt());
}

BLYNK_WRITE(SLOT3_START_BUTTON) //Connection 3 Connect button
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
      Blynk.setProperty(roleButton[slot], "label", "");
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
      sprintf(connectionParams[slot].connectionType,"s --clientname slot%d",slot);
   }
   connectionParams[slot].role = role;
   printf("Slot %d connection type is now: %s\r\n", slot, connectionParams[slot].connectionType);
   if (restartNeeded)
   {
      OpenSlot(slot);
   }
}

BLYNK_WRITE(SLOT1_ROLE_BUTTON) //Connection 1 Connection Type
{
   SetSlotRole(0, param[0].asInt());
}

BLYNK_WRITE(SLOT2_ROLE_BUTTON) //Connection 2 Connection Type
{
   SetSlotRole(1, param[0].asInt());
}

BLYNK_WRITE(SLOT3_ROLE_BUTTON) //Connection 3 Connection Type
{
   SetSlotRole(2, param[0].asInt());
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
		 system("sudo rm /boot/soundcard.txt");
		 system("sudo bash -c \"echo 'dtoverlay=audioinjector-wm8731-audio' >> /boot/soundcard.txt\"");
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
		 system("sudo rm /boot/soundcard.txt");
		 system("sudo bash -c \"echo 'dtoverlay=fe-pi-audio' >> /boot/soundcard.txt\"");
		 
         break;
      case 3: // Hifiberry DAC +ADC
		 system("sudo rm /boot/soundcard.txt");
		 system("sudo bash -c \"echo 'dtoverlay=hifiberry-dacplusadc' >> /boot/soundcard.txt\"");

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

void EcaConnect(uint8_t slot)   //Sets up a connection in Ecasound with an input/output and gain control
{
	
	if (connectionState[slot])
	{
		sprintf(ecaCommand,"c-add slot%d",slot);  //Add a chain for the slot
		printf("%s\r\n",ecaCommand);
		eci_command(ecaCommand);
		sprintf(ecaCommand,"c-select slot%d",slot);  //select chain
		eci_command(ecaCommand);
		printf("%s\r\n",ecaCommand);
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s",sampleRate);  //set audio format
		eci_command(ecaCommand);
		printf("%s\r\n",ecaCommand);
		if (connectionParams[slot].role==0)   //If this slot is a server
		{	
			sprintf(ecaCommand,"ai-add jack,slot%d",slot);  //Connect receive from slot to Ecasound chain
			eci_command(ecaCommand);
			printf("%s\r\n",ecaCommand);
         //Disconnect jacktrip from system playback since connection is now to Ecasound
			sprintf(ecaCommand,"jack_disconnect system:playback_1 slot%d:receive_1",slot);  
			system(ecaCommand);
		}
		else  //If this slot is a client
		{
			sprintf(ecaCommand,"ai-add jack,%s",connectionParams[slot].clientIP); //Connect receive from slot to Ecasound chain
			printf("%s\r\n",ecaCommand);
			eci_command(ecaCommand);
			printf("%s\r\n",ecaCommand);
         //Disconnect jacktrip from system playback since connection is now to Ecasound
			sprintf(ecaCommand,"jack_disconnect system:playback_1 %s:receive_1",connectionParams[slot].clientIP); 
			system(ecaCommand);			
		}
		eci_command("cop-add -eadb:0");  // add gain chain operator
		sprintf(ecaCommand,"slot%d,",slot);
		strcat(connectedSlots,ecaCommand);  //For each slot that is 'connected' add slot to connected slots
		enableVolume[slot] = true;
		switch (slot)
		{
		case 0:
			Blynk.virtualWrite(SLOT1_GAIN_SLIDER, 0);
			break;
		case 1:
			Blynk.virtualWrite(SLOT2_GAIN_SLIDER, 0);
			break;
		case 2:
			Blynk.virtualWrite(SLOT3_GAIN_SLIDER, 0);
			break;
		default:
			break;
		}

	}
}

BLYNK_WRITE(ROUTING) // Ecasound setup/start/stop
{
	printf("Got a value: %s\n", param[0].asStr());
	uint8_t i;
	if (param[0])
	{
		printf("Pressed\n");
		char routeCommand[128] = "jack";	
		eci_init();    //Initialize Ecasound
		
		eci_command("cs-add rJam_chainsetup");  // Add chainsetup to Ecasound
		printf("cs-add\r\n");
	
		eci_command("c-add self");  // Add a chain for monitoring/self
		eci_command("c-select self");  //Select self chain
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s",sampleRate);  //Set audio format, 32 bit, 1 channel, samplerate
		eci_command(ecaCommand);
		eci_command("ai-add jack,system");  //Add input to the chain from jack,system
		eci_command("cop-add -eadb:0");   //Add gain control to the chain
		//eci_command("ao-add loop,1");
		
		eci_command("cs-status");  //Get the chainsetup status
		printf("Chain operator status: %s\n", eci_last_string());

		myVolumeIsEnabled = true;
		Blynk.virtualWrite(MONITOR_GAIN_SLIDER, 0);
	
		eci_command("c-add outL");  //Add a chain for main out left
		eci_command("c-select outL");
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s",sampleRate);
		eci_command(ecaCommand);
		eci_command("ao-add jack,system:playback_1");  //Add output for system playback 1
		
		eci_command("c-add outR");  //Add a chain for main out right
		eci_command("c-select outR");
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s",sampleRate);
		eci_command(ecaCommand);
		eci_command("ao-add jack,system:playback_2");

		eci_command("c-select outL,outR");  // Select both main output chains
		eci_command("ai-add loop,1");  //Assign the loop to both main outputs

		eci_command("cs-status");
		printf("Chain operator status: %s\n", eci_last_string());	
		
		sprintf(connectedSlots,"self,");  //Initialize connectedSlots to self (local monitor) only
		
		for (i=0; i<TOTAL_SLOTS; i++)  //Check each slot for a connection, and connect appropriate slots
		{
			printf("%d\r\n",i);
			EcaConnect(i);
			
		}

		sprintf(ecaCommand,"c-select %s",connectedSlots);  //Select the chains for all connected slots
		eci_command(ecaCommand);
		eci_command("ao-add loop,1");  //Add loop as an output to the chains of all connected slots
		printf("Connected slots: %s\r\n",connectedSlots);
		
		eci_command("cs-status");
		printf("Chain operator status: %s\n", eci_last_string());

		eci_command("cs-connect");  //Connect the chainsetup

		eci_command("start");  //Run the chainsetup
		sleep(1);
		eci_command("engine-status");  //Status of the Ecasound engine

		printf("Chain operator status: %s\n", eci_last_string());
	}
	else  //Audio routing turned off
	{
		eci_command("stop");  //Stop ecasound
		eci_command("cs-disconnect");   //Disconnect ecasound
		eci_command("cop-status");
		printf("Chain operator status: %s", eci_last_string());
		eci_command("cs-remove");
		for (i=0; i<TOTAL_SLOTS; i++)   //Disable the gain control for all slots since Ecasound is stopped
			{
				enableVolume[i] = false;
			}
		eci_cleanup();	
	}
}

void SetGainFromSlider(int gain)   //Get gain from the slider widget, apply to the gain control in ecasound
	{
		sprintf(ecaCommand,"cop-set 1,1,%d",gain);
		printf("Gain: %s\n",ecaCommand);
		eci_command(ecaCommand);
	}

void GainSliderChanged(int slider, int value)
{
   char msg[32];

	if (enableVolume[slider])  //Check to make sure the slot is connected/routed otherwise gain control should be ignored
	{
      sprintf(msg, "c-select slot%d", slider);
		eci_command(msg);
		SetGainFromSlider(value);
	}
	else
	{
		Blynk.virtualWrite(gainSlider[slider], 0);
	}
}

BLYNK_WRITE(SLOT1_GAIN_SLIDER)  // slot0 Gain slider
{
   GainSliderChanged(0, param[0].asInt());
}


BLYNK_WRITE(SLOT2_GAIN_SLIDER)  // slot1 Gain slider
{
   GainSliderChanged(1, param[0].asInt());
}


BLYNK_WRITE(SLOT3_GAIN_SLIDER)  // slot2 Gain slider
{
   GainSliderChanged(2, param[0].asInt());
}

BLYNK_WRITE(LEFT_EDIT_FIELD_TEXT_BOX) //Left edit field
{
	//check currently active edit button, if none are active do nothing, if 'save session' is active do nothing
	//for the 3 slot edit buttons, write the new name, somthing like the next 2 lines only slot dependent?
	strcpy(users[slotBeingEdited].name, param[0].asStr());  //write new user name for active slot
	Blynk.setProperty(connectButton[slotBeingEdited], "label", users[slotBeingEdited].name); // Write user name as label to connect button for active slot
}

BLYNK_WRITE(RIGHT_EDIT_FIELD_TEXT_BOX) //Left edit field
{
   char *p;
   char buf[64];

   strcpy(buf, param[0].asStr());

   // find name
   p = strtok(buf, ":");
   if (p == NULL) return;
   strcpy(users[slotBeingEdited].ipAddr, p);
   strcpy(connectionParams[slotBeingEdited].clientIP, p);

   p = strtok(NULL, ":");
   if (p == NULL) return;
   strcpy(users[slotBeingEdited].port, p);
   strcpy(connectionParams[slotBeingEdited].clientIP, p);

   Blynk.setProperty(roleButton[slotBeingEdited], "label", param[0].asStr());
}

static void EditButtonClicked(int slot, int state)
{
   char msg[32];
   int i;

   if (state)
   {
      slotBeingEdited = slot;

	   //Turn off any other active edit buttons or the 'save session' button
      for(i=0; i<TOTAL_SLOTS; i++) {
         if(i!=slot) {
            Blynk.virtualWrite(editButton[i], LOW);
         }
      }
      Blynk.virtualWrite(SESSION_SAVE_BUTTON, LOW);

		//Store this button as the currently active edit button
      sprintf(msg, "NAME Connection %d", slot+1);
		Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX, "label", msg);  //Populate the label for the left edit field
		Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX, users[slot].name);										  //Seed the data for the left edit field

		Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX, "label", "IP_ADRESS:Port_Offset");                     //Populate the label for the right edit field
      sprintf(msg, "%s:%s", users[slot].ipAddr, users[slot].port);
		Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX, msg);                              //Seed the data for the right edit field
   }
   else
   {
		Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX,"label"," ");  //clear left edit field label
		Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX,"\r");         //clear left edit field data
		Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX,"label"," ");  //clear right edit field label
		Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX,"\r");        //clear right edit field data
		//clear currently active edit button
   }
}

BLYNK_WRITE(SLOT1_EDIT_BUTTON) //Connection 1 Edit button
{
   EditButtonClicked(0, param[0]);
}

BLYNK_WRITE(SLOT2_EDIT_BUTTON) //Connection 1 Edit button
{
   EditButtonClicked(1, param[0]);
}

BLYNK_WRITE(SLOT3_EDIT_BUTTON) //Connection 1 Edit button
{
   EditButtonClicked(2, param[0]);
}

BLYNK_WRITE(MONITOR_GAIN_SLIDER)  // Monitor Gain slider
{
	if (myVolumeIsEnabled)  //Check to make sure the slot is connected/routed otherwise gain control should be ignored
	{
		eci_command("c-select self");
		SetGainFromSlider(param[0].asInt());
	}
	else
	{
		Blynk.virtualWrite(MONITOR_GAIN_SLIDER, 0);
	}	
}


void setup()
{
   Blynk.begin(auth, serv, port);
   eci_init();	//Initialize Ecasound
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

   printf("auth: %s\r\n", auth);
   printf("serv: %s\r\n", serv);
   printf("port: %d\r\n", port);

   setup();

   while(true) {
      loop();
   }

   return 0;
}
