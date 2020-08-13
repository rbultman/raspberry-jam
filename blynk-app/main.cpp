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
#include <stdio.h>
#include <stdlib.h>
#include <libecasoundc/ecasoundc.h>
#include <string>
#include <vector>
#include <ctype.h>
#include "session_db.h"
#include "pinDefs.h"
#include "sessionInfo.h"
#include "utility.h"
#include "fe-pi-def.h"
#include "audio-injector-def.h"
#include "HIFI_DAC_ADC_DEF.h"
#include "settings.h"

#define TOTAL_SLOTS 3
#define ECA_CHANNEL_OFFSET 2

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

int ipPort[TOTAL_SLOTS] = {
   SLOT1_IP_PORT,
   SLOT2_IP_PORT,
   SLOT3_IP_PORT,
};

int connectionName[TOTAL_SLOTS] = {
   SLOT1_CONNECTION_NAME,
   SLOT2_CONNECTION_NAME,
   SLOT3_CONNECTION_NAME,
};

ConnectionInfo_T connections[TOTAL_SLOTS];
SessionInfo_T sessionInfo;
std::vector<char *> sessionNames;

int slotBeingEdited = -1;

enum {
   EditMode_Idle,
   EditMode_Connection,
   EditMode_Session,
   EditMode_DeleteSession
};
int editMode = EditMode_Idle;

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);

#include <BlynkWidgets.h>

char connectedSlots[64];
bool myVolumeIsEnabled = false;
bool debounceFlag = true;

int longPressMillis = 2000;    // time in millis needed for longpress
int longPressTimer;
int pollTimeMillis = 1000;
int pollTimer;
int debounceTimer;


FePi fePiCard;
AudioInjector audioInjectorCard;
HifiberryDacPlusAdc hifiberryDacPlusAdc ;
SoundcardInterface *soundcard;

static const char *rxBufferSize[] = {
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
#define BUFFER_SIZE_COUNT (sizeof(rxBufferSize)/sizeof(char *))

static const char *sampleRate[] = {
   "44100",
   "48000",
   "96000",
};
#define SAMPLE_RATE_COUNT (sizeof(sampleRate)/sizeof(char *))


typedef struct JacktripParams {
   char connectionType[32];
   bool isConnected;
   bool volumeIsEnabled;
   bool pollForClient;
} JacktripParams;

JacktripParams connectionParams[TOTAL_SLOTS] = {
   {"s --clientname slot0", false, false, false},
   {"s --clientname slot1", false, false, false},
   {"s --clientname slot2", false, false, false}
};

BlynkTimer tmr;

// function prototypes
void SetSlotRole(uint8_t slot, uint8_t role);
void KillSlot(int slot);
void KillAllSlots();
void OpenSlot(int slot);
void RouteSlot(int slot);
void initializeSoundCard();

static void PopulateSessionDropDown() {
   int i;
   char buf[64*64];
   char *p;
   BlynkParam labels(buf, 0, sizeof(buf));
   const unsigned char *name;

   for(i=0; i<sessionNames.size(); i++) {
      free(sessionNames[i]);
   }
   sessionNames.clear();

   labels.add("New Session");
   name = GetFirstSessionName();
   while(name != NULL) {
      labels.add((const char *)name);
      p = (char *)malloc(strlen((const char *)name)+1);
      strcpy(p, (const char *)name);
      sessionNames.push_back(p);
      name = GetNextSessionName();
   }

   Blynk.setProperty(SESSION_DROP_DOWN, "labels", labels);
}

BLYNK_CONNECTED() {
   int i;
   
   // Sync buttons that control alsa first, don't need jack started for this
   Blynk.syncVirtual(INPUT_LEVEL); //sync input level on connection
   Blynk.syncVirtual(OUTPUT_LEVEL); //sync output level on connection
   Blynk.syncVirtual(INPUT_SELECT); //sync Input selection
   Blynk.syncVirtual(MIC_GAIN);  //sync Mic Gain
   
   Blynk.virtualWrite(editButton[0],0);
   Blynk.syncVirtual(editButton[0]);
   
   Blynk.virtualWrite(ROUTING,0);
   Blynk.syncVirtual(ROUTING);
   
   Blynk.syncVirtual(SAMPLE_RATE); //sync sample rate on connection
   
      // set connection states to off
   for (i=0; i<TOTAL_SLOTS; i++) {
      Blynk.virtualWrite(connectButton[i], LOW);

   }

   //Sync Slot settings

   for (i=0; i<TOTAL_SLOTS; i++) {

	  Blynk.syncVirtual(latencySlider[i]); //sync Latency
	  Blynk.syncVirtual(gainSlider[i]); //sync Gain
	  Blynk.syncVirtual(ipPort[i]); //sync ip & port
      Blynk.syncVirtual(connectionName[i]); //sync connection name
	  Blynk.syncVirtual(roleButton[i]); //sync Role
	  	  
   }

   Blynk.syncVirtual(MONITOR_GAIN_SLIDER);

   // initialize menu items
   PopulateSessionDropDown();

}

BLYNK_WRITE(OUTPUT_LEVEL) //Output Level Slider
{
   char mixCommand[100];

   if (soundcard == NULL)
   {
      puts("Can't set output level, soundcard is not initialized.");
      return;
   }
   
   sessionInfo.outputLevel = param[0].asInt();

   if(soundcard->mixMasterCommand)
   {
      printf("New output level: %s\n", param[0].asStr());
      sprintf(mixCommand, "amixer -M set %s %s%%", soundcard->mixMasterCommand, param[0].asStr());
      printf("%s\r\n",mixCommand);
      system(mixCommand);
   }
}

BLYNK_WRITE(INPUT_LEVEL)  //Input Level Slider
{
   char mixCommand[100];
   
   if (soundcard == NULL)
   {
      puts("Can't set input level, soundcard is not initialized.");
      return;
   }
   
   sessionInfo.inputLevel = param[0].asInt();

   if(soundcard->mixCaptureCommand)
   {
      printf("New input level: %s\n", param[0].asStr());
      sprintf(mixCommand, "amixer -M set %s %s%%", soundcard->mixCaptureCommand, param[0].asStr());
      printf("%s\r\n",mixCommand);
      system(mixCommand);
   }
}

static void SetLatencyForSlot(uint8_t slot, uint8_t latency)
{
   printf("Latency set for slot %d\r\n", slot);

   if (latency >= BUFFER_SIZE_COUNT) {
      printf("Buffer size index out of range.\r\n");
   } else {
      printf("New latency for slot %d: %s \r\n", slot, rxBufferSize[latency]);
      connections[slot].latency = latency;
      if (connectionParams[slot].isConnected) 
      {
         KillSlot(slot);
         OpenSlot(slot);
		 RouteSlot(slot);
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

void PopulateUiWithSessionInfo()
{
   uint8_t i;
   char ip_port[32];
   char label[64];

   KillAllSlots();

   for (i=0; i<TOTAL_SLOTS; i++) 
   {
      // set parameters
      SetSlotRole(i, connections[i].role);

      Blynk.setProperty(connectButton[i], "label", connections[i].name);
	  Blynk.virtualWrite(connectionName[i],connections[i].name);
      sprintf(ip_port, "%s:%d", connections[i].ipAddr, connections[i].portOffset);
      Blynk.setProperty(roleButton[i], "label", ip_port);
	  Blynk.virtualWrite(ipPort[i],ip_port);
      Blynk.virtualWrite(latencySlider[i], connections[i].latency);
      Blynk.virtualWrite(roleButton[i], connections[i].role);
      Blynk.virtualWrite(gainSlider[i],connections[i].gain);
      Blynk.virtualWrite(editButton[i], LOW);
      Blynk.syncVirtual(editButton[i]);
      sprintf(label, "Connection %d", i);
      Blynk.setProperty(editButton[i], "label", label);
   }

   Blynk.virtualWrite(INPUT_LEVEL, sessionInfo.inputLevel);
   Blynk.syncVirtual(INPUT_LEVEL);
   Blynk.virtualWrite(OUTPUT_LEVEL, sessionInfo.outputLevel);
   Blynk.syncVirtual(OUTPUT_LEVEL);
   Blynk.virtualWrite(SAMPLE_RATE, sessionInfo.sampleRate+1);
   Blynk.syncVirtual(SAMPLE_RATE);
   Blynk.virtualWrite(MONITOR_GAIN_SLIDER, sessionInfo.monitorGain);
   Blynk.syncVirtual(MONITOR_GAIN_SLIDER);
   Blynk.virtualWrite(INPUT_SELECT, sessionInfo.inputSelect);
   Blynk.syncVirtual(INPUT_SELECT);
   Blynk.virtualWrite(MIC_GAIN, sessionInfo.micBoost);
   Blynk.syncVirtual(MIC_GAIN);
}

void PopulateNewSession() 
{
   int i;

   strcpy(sessionInfo.name, "New Session");
   sessionInfo.inputLevel = 50;
   sessionInfo.outputLevel = 50;
   sessionInfo.sampleRate = 2;
   sessionInfo.monitorGain = 0;
   sessionInfo.inputSelect = 0;
   sessionInfo.micBoost = 0;

   for (i=0; i<TOTAL_SLOTS; i++) {
      sprintf(connections[i].name, "User %d", i);
      sprintf(connections[i].ipAddr, "127.0.0.%d", i);
      connections[i].portOffset = i*10;
      connections[i].role = 0;
      connections[i].latency = 4;
      connections[i].gain = 0;
      connections[i].slot = i;
   }

   PopulateUiWithSessionInfo();
}

void PopulateSlotInfoForSession(const char * sessionName)
{
   if(0 == GetAllSessionInfo(sessionName, &sessionInfo, connections))
   {
      PopulateUiWithSessionInfo();
   }
   else
   {
      puts("---> ERROR getting session info\r\n");
   }
}

void PopulateSessionInfo(int session)
{
   KillAllSlots();

   printf("Session selected: %d\r\n", session);
   if (session < sessionNames.size())
   {
      printf("Name: %s\r\n", sessionNames[session]);
      PopulateSlotInfoForSession(sessionNames[session]);
   }
   else
   {
      puts("ERROR: The session number is out of range.");
      PopulateNewSession();
   }
}

BLYNK_WRITE(SESSION_DROP_DOWN) // Sessions Book
{
   int i;
   int session = param.asInt();

   puts("A session is selected.");

   if (session == 1)
   {
      PopulateNewSession();
   }
   else
   {
      PopulateSessionInfo(session-2);
   }
}

void EcaConnect(uint8_t slot)   //Sets up a chain in Ecasound with an input/output and gain control for a slot
{
  char ecaCommand[100];

  if (slot >= TOTAL_SLOTS)
  {
    printf("ERROR: slot out of range in EcaConnect: %d\r\n", slot);
    return;
  }
	
    //Add a chain for the slot
		sprintf(ecaCommand,"c-add slot%d",slot);  
		printf("===> Adding chain for slot command: %s\r\n",ecaCommand);
		eci_command(ecaCommand);

	//select chain
		sprintf(ecaCommand,"c-select slot%d",slot);
		printf("===> Selecting chaing command:%s\r\n",ecaCommand);
		eci_command(ecaCommand);

    //set audio format
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s", sampleRate[sessionInfo.sampleRate]);  
		printf("===> Setting audio format command: %s\r\n",ecaCommand);
		eci_command(ecaCommand);

	//add input for slot
		sprintf(ecaCommand,"ai-add jack");  
		printf("===> Connecting receive from slot to ecasound chain command: %s\r\n",ecaCommand);
		eci_command(ecaCommand);

   
    // add gain chain operator
		eci_command("cop-add -eadb:0");

    //For each slot that is 'connected' add slot to connected slots
		sprintf(ecaCommand,"slot%d,",slot);
		strcat(connectedSlots, ecaCommand);  
		//connectionParams[slot].volumeIsEnabled = true;

	
}

void EcaSetup() // Ecasound setup/start
{
   char ecaCommand[100];
   
	uint8_t i;

		printf("Pressed\n");
		char routeCommand[128] = "jack";	
		eci_init();    //Initialize Ecasound
		
		eci_command("cs-add rJam_chainsetup");  // Add chainsetup to Ecasound
		printf("cs-add\r\n");
	
		eci_command("c-add self");  // Add a chain for monitoring/self
		eci_command("c-select self");  //Select self chain
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s", sampleRate[sessionInfo.sampleRate]);  //Set audio format, 32 bit, 1 channel, samplerate
		eci_command(ecaCommand);
		eci_command("ai-add jack,system");  //Add input to the chain from jack,system
		eci_command("cop-add -eadb:0");   //Add gain control to the chain
		eci_command("cs-status");  //Get the chainsetup status
		printf("Chain operator status: %s\n", eci_last_string());

		myVolumeIsEnabled = true;
	
		eci_command("c-add outL");  //Add a chain for main out left
		eci_command("c-select outL");
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s", sampleRate[sessionInfo.sampleRate]);
		eci_command(ecaCommand);
		eci_command("ao-add jack,system:playback_1");  //Add output for system playback 1
		
		eci_command("c-add outR");  //Add a chain for main out right
		eci_command("c-select outR");
		sprintf(ecaCommand,"cs-set-audio-format 32,1,%s", sampleRate[sessionInfo.sampleRate]);
		eci_command(ecaCommand);
		eci_command("ao-add jack,system:playback_2"); //Add output for system playback 2

		eci_command("c-select outL,outR");  // Select both main output chains
		eci_command("ai-add loop,1");  //Assign the loop to both main outputs

		eci_command("cs-status");
		printf("Chain operator status: %s\n", eci_last_string());	
		
		sprintf(connectedSlots,"self,");  //Initialize connectedSlots to self (local monitor) only
		
		for (i=0; i<TOTAL_SLOTS; i++)  //Create chain for each slot
		{
			printf("Routing slot %d\r\n",i);
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

BLYNK_WRITE(SAMPLE_RATE) // Sampe Rate setting
{
   char jackCommand[128] = "jackd";
   int8_t newSampleRate = param.asInt() - 1;

   printf("New sample rate: %d\r\n", newSampleRate);
   if (newSampleRate >= 0 && newSampleRate < SAMPLE_RATE_COUNT)
   {
      sessionInfo.sampleRate = newSampleRate;
   }
   else
   {
      printf("Unknown sample rate selected: %d\r\n", newSampleRate);
      Blynk.virtualWrite(SAMPLE_RATE, 2);
      Blynk.syncVirtual(SAMPLE_RATE);
      return;
   }
   printf("%s \r\n",sampleRate[sessionInfo.sampleRate]);
   KillAllSlots();
   eci_command("stop-sync");
   eci_command("cs-disconnect");
   system("sudo killall jackd");
   system("jack_wait -q");  //This will wait until jackd is dead before continuing
   sprintf(jackCommand,"jackd -P70 -p32 -t2000 -d alsa -r%s -p64 -s -S &", sampleRate[sessionInfo.sampleRate]);
   //sprintf(jackCommand,"sh start_jack.sh -r%s &", sampleRate[sessionInfo.sampleRate]);
   system(jackCommand);	
   system("jack_wait -w"); //This will wait until jack server is available before continuing
   EcaSetup();  // Sets up Ecasound chain-setup, chains, starts Ecasound

   puts("Exiting sample rate write");
}

void OpenSlot(int slot)
{
   char jacktripCommand[128];

   //Blynk.virtualWrite(connectButton[slot] ,HIGH);
   sprintf(connections[slot].jacktripKillSearch, "[j]acktrip -o%d -n1 -z -%s -q%s --nojackportsconnect", 
         connections[slot].portOffset, 
         connectionParams[slot].connectionType, 
         rxBufferSize[connections[slot].latency]);
   sprintf(jacktripCommand, "jacktrip -o%d -n1 -z -%s -q%s --nojackportsconnect &", 
         connections[slot].portOffset, 
         connectionParams[slot].connectionType, 
         rxBufferSize[connections[slot].latency]);
   printf("jacktrip command - %s\r\n", jacktripCommand);
   system(jacktripCommand);
   connectionParams[slot].isConnected = true;
}

void StopTimerIfConnectionsResolved()
{
	int i;
	bool stopTimer = true;
	
	for (i=0; i<TOTAL_SLOTS; i++) {
		if (connectionParams[i].pollForClient) {
		stopTimer = false;
		}
	}
	if (stopTimer) {
	tmr.disable(pollTimer);	
	}
}

void KillSlot(int slot)
{
   char killcmd[255];

   Blynk.virtualWrite(connectButton[slot] ,LOW);
   connectionParams[slot].volumeIsEnabled = false;
   //send kill command
   sprintf(killcmd, "kill `ps aux | grep \"%s\" | awk '{print $2}'`", connections[slot].jacktripKillSearch);
   printf("kill cmd = %s\r\n", killcmd);
   system(killcmd);
   connectionParams[slot].isConnected = false;
   connectionParams[slot].pollForClient = false;
   connectionParams[slot].volumeIsEnabled = false;
   StopTimerIfConnectionsResolved();
   // sleep a little so the process dies and jackd recovers
   sleep_millis(250);
}

void PollForClient()
{
	int i;
	char ecaCommand[100];
	int status;
	int ret;
	printf("POLL FOR CLIENT TEST\r\n");
	for (i=0; i<TOTAL_SLOTS; i++) {
		//Check if slot needs to be polled
		if (connectionParams[i].pollForClient) {
			printf("Polling Slot %d\r\n",i);
			sprintf(ecaCommand,"jack_connect ecasound:in_%d slot%d:receive_1",i + ECA_CHANNEL_OFFSET,i);  
			printf("===> Connecting receive from slot to ecasound chain command: %s\r\n",ecaCommand);
			status = system(ecaCommand);
			if (status != -1) { // -1 means an error with the call itself
				ret = WEXITSTATUS(status);
				printf("===> Exit Code: %d\r\n",ret);
				if (ret == 0) {
				connectionParams[i].pollForClient = false;
				connectionParams[i].volumeIsEnabled = true;
				sprintf(ecaCommand,"jack_connect system:capture_1 slot%d:send_1",i);
				//Blynk.virtualWrite(connectButton[i] ,HIGH);
				system(ecaCommand);
				StopTimerIfConnectionsResolved();
				printf("Slot %d routed\r\n",i);
				}
			}	
		}
		//try to connect slot
	}
}

void RouteSlot(int slot)
{
	char ecaCommand[100];
	int ret;
	int status;
	
    // Connect slot to ecasound
		if (connections[slot].role==0)   //If this slot is a server
		{	
		connectionParams[slot].pollForClient = true;  //server connections need to be polled until clinet connects
		
		tmr.enable(pollTimer); //Enable timer to poll for client connection

		}
		else  //If this slot is a client
		{
      //Connect receive from slot to Ecasound chain

			sprintf(ecaCommand,"jack_connect %s:receive_1 ecasound:in_%d",connections[slot].ipAddr,slot + ECA_CHANNEL_OFFSET); 
			printf("===> Connecting receive from slot to ecasound chain command: %s\r\n",ecaCommand);
			ret = 1;
			while (ret == 1){  //repeat connection attempt until successful
			status = system(ecaCommand);
			if (status != -1) { // -1 means an error with the call itself
				ret = WEXITSTATUS(status);  //get exit code from system command
				printf("===> Exit Code: %d\r\n",ret);
				}
			}
			connectionParams[slot].volumeIsEnabled = true;
			//Blynk.virtualWrite(connectButton[slot] ,HIGH);
			sprintf(ecaCommand,"jack_connect %s:send_1 system:capture_1",connections[slot].ipAddr);
			system(ecaCommand);
		
		}
	Blynk.virtualWrite(connectButton[slot] ,HIGH);
}

void RouteAllSlots()
{
   int i;

   for(i=0; i<TOTAL_SLOTS; i++)
   {
      if (connectionParams[i].isConnected)
      {
         RouteSlot(i);
      }
   }
}

void KillAllSlots()
{
   int i;

   for(i=0; i<TOTAL_SLOTS; i++)
   {
      if (connectionParams[i].isConnected)
      {
         KillSlot(i);
      }
   }
}

void DisconnectSlot(void *pSlot)
{
   uint32_t slot = (uint32_t)pSlot;

   printf("Disconnecting slot: %d\r\n", slot);
   Blynk.virtualWrite(connectButton[slot],LOW);
   KillSlot(slot);
   printf("Slot %d disconnected\r\n", slot+1);
}

void SetDebounceFlag()
{
    debounceFlag = true;	
}

void ConnectSlot(int slot, int pressed)
{
   printf("Connecting slot %d %d\r\n", slot,pressed);

   if (pressed)
   {
      longPressTimer = tmr.setTimeout(longPressMillis, DisconnectSlot, (void *)slot);
   }
   else
   {	
	//if (connectionParams[slot].isConnected == false ) 
	//{
	  if (debounceFlag)
	  {
	  KillSlot(slot);
      OpenSlot(slot);
	  RouteSlot(slot);
	  }
	  debounceTimer = tmr.setTimeout(100,SetDebounceFlag);
	  debounceFlag = false;
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
   printf("New IP address requested for slot: %s\n", newIp);
   ipCheck=checkIpFormat(newIp);
   if (ipCheck == 0)
   {	
      strcpy(connections[slot].ipAddr, newIp);
      Blynk.virtualWrite(roleButton[slot], HIGH);
      connections[slot].role = 1;
      sprintf(connectionParams[slot].connectionType, "c%s", connections[slot].ipAddr);
      printf("New ip address: <%s>\r\n",connections[slot].ipAddr);
   }
   else
   {
      puts("ERROR: IP address rejected.");
      Blynk.setProperty(roleButton[slot], "label", "");
   }
}

void SetSlotRole(uint8_t slot, uint8_t role)
{
   bool restartNeeded = false;

   printf("New role for slot %d: %d\n", slot, role);
   if (connectionParams[slot].isConnected)
   {
      KillSlot(slot);
      restartNeeded = true;
   }
   if (role == 1)
   {
      sprintf(connectionParams[slot].connectionType,"c%s", connections[slot].ipAddr);
   }
   else
   {
      sprintf(connectionParams[slot].connectionType,"s --clientname slot%d",slot);
   }
   connections[slot].role = role;
   printf("Slot %d connection type is now: %s\r\n", slot, connectionParams[slot].connectionType);
   if (restartNeeded)
   {
      OpenSlot(slot);
	  RouteSlot(slot);
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

// BLYNK_WRITE(SOUNDCARD) //Soundcard Selection
void initializeSoundCard()
{
   int card = GetSelectedSoundcard();

   printf("Sound card selection: %d\r\n", card);

   switch (card)
   {
      case 1: // Fe-Pi
         fePiCard.Initialize();
         soundcard = &fePiCard;
         break;
      case 2: // Audio Injector Stereo
         audioInjectorCard.Initialize();
         soundcard = &audioInjectorCard;
         break;
      case 3: // Hifiberry DAC +ADC
         hifiberryDacPlusAdc.Initialize();
         soundcard = &hifiberryDacPlusAdc;
         break;
      default:
         soundcard = NULL;
         printf("Unknown sound card selected \r\n");
   }
}

BLYNK_WRITE(INPUT_SELECT) //Input Selection
{

   if (soundcard == NULL)
   {
      puts("Can't change the input source, soundcard is not initialized.");
      return;
   }

   printf("New input selected: %s\n", param[0].asStr());
   sessionInfo.inputSelect = param[0];

   if (param[0])
   {
      if (soundcard->inputMicCommand)
      {
         system(soundcard->inputMicCommand);
      }
   }
   else
   {
      if (soundcard->inputLineCommand)
      {
         system(soundcard->inputLineCommand);
      }
   }
}

BLYNK_WRITE(MIC_GAIN) //Mic Gain
{
   char mixCommand[100];

   if (soundcard == NULL)
   {
      puts("Can't change the mic gain, soundcard is not initialized.");
      return;
   }

   if (param[0])
   {
      if (soundcard->micGainText && soundcard->micGainCommand)
      {
         sessionInfo.micBoost++;
         if (sessionInfo.micBoost >= soundcard->micGainSettingsCount)
         {
            sessionInfo.micBoost = 0;
         }

         Blynk.setProperty(MIC_GAIN,"offLabel", soundcard->micGainText[sessionInfo.micBoost]);
         sprintf(mixCommand, "%s %u", soundcard->micGainCommand, sessionInfo.micBoost);
         printf("Mic gain command: %s\r\n", mixCommand);
         system(mixCommand);
      }
   }
}

void SetGainFromSlider(int gain)   //Get gain from the slider widget, apply to the gain control in ecasound
{
   char ecaCommand[100];

   sprintf(ecaCommand,"cop-set 1,1,%d",gain);
   printf("Gain: %s\n",ecaCommand);
   eci_command(ecaCommand);
}

BLYNK_WRITE(ROUTING) // Ecasound setup/start/stop
{
   
	printf("Routing button selected: %s\n", param[0].asStr());
	char msg[100];
	uint8_t i;
	if (param[0])
	{
		if (slotBeingEdited != -1) {    //Check that an edit button is pressed
			if (connectionParams[slotBeingEdited].volumeIsEnabled) {  //Make sure the connection is active
			
				sprintf(msg, "c-select slot%d", slotBeingEdited);  //Turn down the gain to the outputs so you don't hear the test
				eci_command(msg);
     			SetGainFromSlider(-100);
				printf("slot being edited roll: %d\r\n",connections[slotBeingEdited].role);
				if (connections[slotBeingEdited].role == 0) {  //If connection is a server
  
					system("jack_iodelay &");  // Start jack_iodelay
					sleep_millis(5000);
					sprintf(msg,"jack_connect slot%d:send_1 jack_delay:out",slotBeingEdited);  //Connect jack io_delay
					printf("jack_connect slot%d:send_1 jack_delay:out",slotBeingEdited);
					system(msg);
					sprintf(msg,"jack_connect slot%d:receive_1 jack_delay:in",slotBeingEdited);
					printf("jack_connect slot%d:receive_1 jack_delay:in",slotBeingEdited);
					system(msg);
					
					
				}
				else  //Connection is a client
				{
				    
					sprintf(msg,"jack_connect %s:receive_1 %s:send_1",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);  //establish loopback
					printf ("jack_connect %s:receive_1 %s:send_1\r\n",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);
					system(msg);
					
					
				}
			}
		}	
	}
	else
	{
	system("killall jack_iodelay");  // Stop jack io_delay
	sprintf(msg,"jack_disconnect %s:receive_1 %s:send_1",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);  //disable loopback
	printf ("jack_connect %s:receive_1 %s:send_1\r\n",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);
	system(msg);
	if (slotBeingEdited != -1) {
		Blynk.syncVirtual(gainSlider[slotBeingEdited]);  // Set gain back to value from the slider
		}
	}
}


void GainSliderChanged(int slider, int value)
{
   char msg[32];

   printf("Gain changed for slider %d\r\n", slider);

   sprintf(msg, "c-select slot%d", slider);
   eci_command(msg);
   SetGainFromSlider(value);
   connections[slider].gain = value;

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

void ClearEditBoxes() {
   Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX,"label"," ");  //clear left edit field label
   Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX,"\r");         //clear left edit field data
   Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX,"label"," ");  //clear right edit field label
   Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX,"\r");        //clear right edit field data
   editMode = EditMode_Idle;
   slotBeingEdited = -1;
}

BLYNK_WRITE(LEFT_EDIT_FIELD_TEXT_BOX) //Left edit field
{
   char *p;

	//check currently active edit button, if none are active do nothing, if 'save session' is active do nothing
	//for the 3 slot edit buttons, write the new name, somthing like the next 2 lines only slot dependent?
   if (editMode == EditMode_Connection) {
      strcpy(connections[slotBeingEdited].name, param[0].asStr());  //write new user name for active slot
      TrimWhitespace(connections[slotBeingEdited].name);
      Blynk.setProperty(connectButton[slotBeingEdited], "label", connections[slotBeingEdited].name); // Write user name as label to connect button for active slot
	  Blynk.virtualWrite(connectionName[slotBeingEdited], connections[slotBeingEdited].name);
   }
}

BLYNK_WRITE(RIGHT_EDIT_FIELD_TEXT_BOX) // Right edit field
{
   char *p;
   char buf[64];
   int i;

   if (editMode == EditMode_Connection) {
      strcpy(buf, param[0].asStr());

      TrimWhitespace(buf);

      // find ip address
      p = strtok(buf, ":");
      if (p == NULL) return;
      SetSlotIpAddress(slotBeingEdited, p);

      // find offset
      p = strtok(NULL, ":");
      if (p == NULL) return;
      connections[slotBeingEdited].portOffset = strtol(p, NULL, 10);

      sprintf(buf, "%s:%d", connections[slotBeingEdited].ipAddr, connections[slotBeingEdited].portOffset);

      Blynk.setProperty(roleButton[slotBeingEdited], "label", buf);
	  Blynk.virtualWrite(ipPort[slotBeingEdited], buf);
   } else if (editMode == EditMode_Session) {
      // get it
      strcpy(sessionInfo.name, param[0].asStr());
      TrimWhitespace(sessionInfo.name);

      printf("Session name is now: \r\n");
      printf("<%s>\r\n", sessionInfo.name);
   }
}

static void EditButtonClicked(int slot, int state)
{
   char msg[32];
   int i;

   printf("Slot %d edit button clicked.\r\n", slot);

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

      editMode = EditMode_Connection;

		//Store this button as the currently active edit button
      sprintf(msg, "NAME Connection %d", slot+1);
		Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX, "label", msg);  //Populate the label for the left edit field
		Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX, connections[slot].name);										  //Seed the data for the left edit field

		Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX, "label", "IP_ADRESS:Port_Offset");                     //Populate the label for the right edit field
      sprintf(msg, "%s:%d", connections[slot].ipAddr, connections[slot].portOffset);
		Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX, msg);                              //Seed the data for the right edit field
   }
   else
   {
      ClearEditBoxes();
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
   puts("Monitor gain slider changed.");


   eci_command("c-select self");
   SetGainFromSlider(param[0].asInt());
   sessionInfo.monitorGain = param[0].asInt();
		
}

bool longPressDetected = false;

void EnterDeleteSessionState(void *notused)
{
   tmr.deleteTimer(longPressTimer);

   longPressDetected  = true;

   switch(editMode) {
      case EditMode_Session:
         editMode = EditMode_DeleteSession;
         Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX, "label", "Delte session info");
         Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX, "Tap Session to abort");

         Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX, "label", "Hold Session to delete");
         Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX, sessionInfo.name);
         break;

      case EditMode_DeleteSession:
         ClearEditBoxes();
         DeleteSessionInfo(sessionInfo.name);
         PopulateSessionDropDown();
         Blynk.virtualWrite(SESSION_DROP_DOWN, 1);
         Blynk.syncVirtual(SESSION_DROP_DOWN);
         break;
   }
}

BLYNK_WRITE(SESSION_SAVE_BUTTON)
{
   int i;
   char buf[64];

   if(param[0])
   {
      longPressTimer = tmr.setTimeout(longPressMillis, EnterDeleteSessionState, NULL);

      if (editMode == EditMode_DeleteSession) {
         return;
      }

      if (editMode == EditMode_Session) {
         SaveAllSessionInfo(&sessionInfo, connections);

         strcpy(buf, sessionInfo.name);

         // reset edit info
         Blynk.virtualWrite(SESSION_SAVE_BUTTON, LOW);
         ClearEditBoxes();

         PopulateSessionDropDown();

         for(i=0; i<sessionNames.size(); i++)
         {
            if (strcmp(sessionNames[i], buf) == 0)
            {
               Blynk.virtualWrite(SESSION_DROP_DOWN, i+2);
               break;
            }
         }
      } else {
         Blynk.virtualWrite(SESSION_SAVE_BUTTON, HIGH);

         for(i=0; i<TOTAL_SLOTS; i++) 
         {
            Blynk.virtualWrite(editButton[i], LOW);
         }

         editMode = EditMode_Session;

         Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX, "label", "Save session info");
         Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX, "Edit name, tap Session");

         Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX, "label", "Session Name");
         Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX, sessionInfo.name);
      }
   }
   else
   {
      tmr.deleteTimer(longPressTimer);
      if (longPressDetected)
      {
         longPressDetected = false;
      }
      else
      {
         if (editMode == EditMode_DeleteSession)
         {
            ClearEditBoxes();
         }
      }
   }
}

BLYNK_WRITE(SLOT1_CONNECTION_NAME)
{
	printf("Slot 0 name %s \r\n",param[0].asStr());
	strcpy(connections[0].name, param[0].asStr());
}

BLYNK_WRITE(SLOT2_CONNECTION_NAME)
{
	printf("Slot 1 name %s \r\n",param[0].asStr());
	strcpy(connections[1].name, param[0].asStr());
}

BLYNK_WRITE(SLOT3_CONNECTION_NAME)
{
	printf("Slot 2 name %s \r\n",param[0].asStr());
	strcpy(connections[2].name, param[0].asStr());
}

BLYNK_WRITE(SLOT1_IP_PORT)
{
	char *p;
	char buf[64];

    strcpy(buf, param[0].asStr());

    // find ip address
    p = strtok(buf, ":");
    if (p == NULL) return;
	strcpy(connections[0].ipAddr, p);

      // find offset
    p = strtok(NULL, ":");
    if (p == NULL) return;
    connections[0].portOffset = strtol(p, NULL, 10);
}

BLYNK_WRITE(SLOT2_IP_PORT)
{
	char *p;
	char buf[64];

    strcpy(buf, param[0].asStr());

    // find ip address
    p = strtok(buf, ":");

    if (p == NULL) return;
	strcpy(connections[1].ipAddr, p);

      // find offset
    p = strtok(NULL, ":");
    if (p == NULL) return;
    connections[1].portOffset = strtol(p, NULL, 10);
}

BLYNK_WRITE(SLOT3_IP_PORT)
{
	char *p;
	char buf[64];

    strcpy(buf, param[0].asStr());

    // find ip address
    p = strtok(buf, ":");

    if (p == NULL) return;
	strcpy(connections[2].ipAddr, p);

      // find offset
    p = strtok(NULL, ":");
    if (p == NULL) return;
    connections[2].portOffset = strtol(p, NULL, 10);
}

void setup(const char *auth, const char *serv, uint16_t port)
{
   initializeSoundCard();
   eci_init();	//Initialize Ecasound
   pollTimer = tmr.setInterval(pollTimeMillis, PollForClient);
   tmr.disable(pollTimer);
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
   const char *auth;
   const char *serv;
   uint16_t port;

   parse_options(argc, argv, auth, serv, port);

   setup(auth, serv, port);

   while(true) {
      loop();
   }

   return 0;
}

