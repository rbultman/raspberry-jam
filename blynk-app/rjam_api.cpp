/*
   rjam_api.cpp

   An API for controlling Raspberry Jam from various UIs
*/

#include <libecasoundc/ecasoundc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "settings.h"
#include "utility.h"
#include "rjam_api.h"
#include "fe-pi-def.h"
#include "audio-injector-def.h"
#include "HIFI_DAC_ADC_DEF.h"
#include "Timer/Timer.h"

#define ECA_CHANNEL_OFFSET 2

SessionInfo_T sessionInfo;
ConnectionInfo_T connections[TOTAL_SLOTS];
const char *sampleRate[SAMPLE_RATE_COUNT] = {
   "44100",
   "48000",
   "96000",
};

JacktripParams connectionParams[TOTAL_SLOTS] = {
   {"s --clientname slot0", false, false, false},
   {"s --clientname slot1", false, false, false},
   {"s --clientname slot2", false, false, false}
};

static char connectedSlots[64];
static bool myVolumeIsEnabled = false;
static Timer slotsConnectedTimer;
static SlotConnectedCallback slotConnectedCallback = NULL;

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

static SoundcardInterface *soundcard = NULL;
static FePi fePiCard;
static AudioInjector audioInjectorCard;
static HifiberryDacPlusAdc hifiberryDacPlusAdc;

void StopTimerIfConnectionsResolved();

void SlotsConnectedTimerExpired(void *pIgnored)
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
               if (slotConnectedCallback)
               {
                  slotConnectedCallback(i);
               }
               sprintf(ecaCommand,"jack_connect system:capture_1 slot%d:send_1",i);
               system(ecaCommand);
               StopTimerIfConnectionsResolved();
               printf("Slot %d routed\r\n",i);
            }
         }	
      }
   }
}

void RjamApi_Initialize(SlotConnectedCallback callback)
{
   slotConnectedCallback = callback;
   slotsConnectedTimer.setInterval(1000);
   slotsConnectedTimer.setPeriodic(true);
   slotsConnectedTimer.setCallback(SlotsConnectedTimerExpired);
}

void RjamApi_InitializeSoundCard()
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

void RjamApi_SetOutputLevel(int level)
{
   char mixCommand[100];

   if (soundcard == NULL)
   {
      puts("Can't set output level, soundcard is not initialized.");
      return;
   }
   
   sessionInfo.outputLevel = level;

   if(soundcard->mixMasterCommand)
   {
      printf("New output level: %d\n", level);
      sprintf(mixCommand, "amixer -M set %s %d%%", soundcard->mixMasterCommand, level);
      printf("%s\r\n",mixCommand);
      system(mixCommand);
   }
}

void RjamApi_SetInputLevel(int level)
{
   char mixCommand[100];
   
   if (soundcard == NULL)
   {
      puts("Can't set input level, soundcard is not initialized.");
      return;
   }
   
   sessionInfo.inputLevel = level;

   if(soundcard->mixCaptureCommand)
   {
      printf("New input level: %d\n", level);
      sprintf(mixCommand, "amixer -M set %s %d%%", soundcard->mixCaptureCommand, level);
      printf("%s\r\n",mixCommand);
      system(mixCommand);
   }
}

void RjamApi_InputSelect(int input)
{
   if (soundcard == NULL)
   {
      puts("Can't change the input source, soundcard is not initialized.");
      return;
   }

   printf("New input selected: %d\n", input);
   sessionInfo.inputSelect = input;

   if (input)
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

void RjamApi_ChangeMicGain()
{
   char mixCommand[100];

   if (soundcard == NULL)
   {
      puts("Can't change the mic gain, soundcard is not initialized.");
      return;
   }

   if (soundcard->micGainText && soundcard->micGainCommand)
   {
      sessionInfo.micBoost++;
      if (sessionInfo.micBoost >= soundcard->micGainSettingsCount)
      {
         sessionInfo.micBoost = 0;
      }

      sprintf(mixCommand, "%s %u", soundcard->micGainCommand, sessionInfo.micBoost);
      printf("Mic gain command: %s\r\n", mixCommand);
      system(mixCommand);
   }
}

const char * RjamApi_GetMicGain()
{
   return soundcard->micGainText[sessionInfo.micBoost];
}

void RjamApi_SetSlotGain(int slot, int gain)
{
   char ecaCommand[100] = "c-select self";

   if (slot < 0)
   {
      printf("Gain changed for monitor\r\n");
      sessionInfo.monitorGain = gain;
   }
   else
   {
      printf("Gain changed for slot %d\r\n", slot);
      sprintf(ecaCommand, "c-select slot%d", slot);
      connections[slot].gain = gain;
   }
   eci_command(ecaCommand);

   sprintf(ecaCommand,"cop-set 1,1,%d",gain);
   printf("Gain: %s\n",ecaCommand);
   eci_command(ecaCommand);
}

void RjamApi_EcaConnect(uint8_t slot)   //Sets up a chain in Ecasound with an input/output and gain control for a slot
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
}

void RjamApi_EcaSetup() // Ecasound setup/start
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
      RjamApi_EcaConnect(i);
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

void RjamApi_KillSlot(int slot)
{
   char killcmd[255];

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
      slotsConnectedTimer.stop();
	}
}

bool RjamApi_SetSampleRate(int newRate)
{
   char jackCommand[128] = "jackd";

   printf("New sample rate: %d\r\n", newRate);
   if (newRate >= 0 && newRate < SAMPLE_RATE_COUNT)
   {
      sessionInfo.sampleRate = newRate;
   }
   else
   {
      printf("Unknown sample rate selected: %d\r\n", newRate);
      return false;
   }
   printf("%s \r\n",sampleRate[sessionInfo.sampleRate]);
   eci_command("stop-sync");
   eci_command("cs-disconnect");
   system("sudo killall jackd");
   system("jack_wait -q");  //This will wait until jackd is dead before continuing
   sprintf(jackCommand,"jackd -P70 -p32 -t2000 -d alsa -r%s -p64 -s -S &", sampleRate[sessionInfo.sampleRate]);
   //sprintf(jackCommand,"sh start_jack.sh -r%s &", sampleRate[sessionInfo.sampleRate]);
   system(jackCommand);	
   system("jack_wait -w"); //This will wait until jack server is available before continuing
   RjamApi_EcaSetup();  // Sets up Ecasound chain-setup, chains, starts Ecasound
   return true;
}

void RjamApi_RouteSlot(int slot)
{
   char ecaCommand[100];
   int ret;
   int status;

   // Connect slot to ecasound
   if (connections[slot].role==0)   //If this slot is a server
   {	
      connectionParams[slot].pollForClient = true;  //server connections need to be polled until clinet connects
      slotsConnectedTimer.start();
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
      sprintf(ecaCommand,"jack_connect %s:send_1 system:capture_1",connections[slot].ipAddr);
      system(ecaCommand);

   }
}

void RjamApi_OpenSlot(int slot)
{
   char jacktripCommand[128];

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

bool RjamApi_SetLatencyForSlot(uint8_t slot, uint8_t latency)
{
   printf("Latency set for slot %d\r\n", slot);

   if (latency >= BUFFER_SIZE_COUNT) {
      printf("Buffer size index out of range.\r\n");
   } else {
      printf("New latency for slot %d: %s \r\n", slot, rxBufferSize[latency]);
      connections[slot].latency = latency;
      if (connectionParams[slot].isConnected) 
      {
         RjamApi_KillSlot(slot);
         RjamApi_OpenSlot(slot);
         RjamApi_RouteSlot(slot);
         return true;
      }
   }

   return false;
}

bool RjamApi_SetSlotIpAddress(uint8_t slot, const char *newIp) 
{
   int ipCheck = 1;
   bool retval = false;

   printf("New IP address requested for slot: %s\n", newIp);
   ipCheck=checkIpFormat(newIp);

   if (ipCheck == 0)
   {	
      strcpy(connections[slot].ipAddr, newIp);
      connections[slot].role = 1;
      sprintf(connectionParams[slot].connectionType, "c%s", connections[slot].ipAddr);
      printf("New ip address: <%s>\r\n",connections[slot].ipAddr);
      retval = true;
   }

   return retval;
}

void RjamApi_SetSlotRole(uint8_t slot, uint8_t role)
{
   bool restartNeeded = false;

   printf("New role for slot %d: %d\n", slot, role);
   if (connectionParams[slot].isConnected)
   {
      RjamApi_KillSlot(slot);
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
      RjamApi_OpenSlot(slot);
      RjamApi_RouteSlot(slot);
   }
}

