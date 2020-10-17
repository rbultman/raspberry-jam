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
#include "utility.h"
#include "settings.h"
#include "config.h"
#include "rjam_api.h"


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

static std::vector<char *> sessionNames;

int slotBeingEdited = -1;
int slotBeingTested = -1;

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

bool debounceFlag = true;

int longPressMillis = 2000;    // time in millis needed for longpress
int longPressTimer;
int pollTimeMillis = 1000;
int debounceTimer;

BlynkTimer tmr;

// function prototypes
void SetSlotRole(uint8_t slot, uint8_t role);
void KillSlot(int slot);
void KillAllSlots();
void RouteSlot(int slot);
void ClearEditBoxes();
static void setEditButtonLabels(int slot);

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
   
   Blynk.syncVirtual(SAMPLE_RATE); //sync sample rate on connection
   
      // set connection states to off
   for (i=0; i<TOTAL_SLOTS; i++) {
      Blynk.virtualWrite(connectButton[i], LOW);

   }

   //Sync Slot settings

   for (i=0; i<TOTAL_SLOTS; i++) {

	  Blynk.syncVirtual(latencySlider[i]); //sync Latency
	  Blynk.syncVirtual(gainSlider[i]); //sync Gain
	  Blynk.syncVirtual(roleButton[i]); //sync Role
	  Blynk.setProperty(editButton[i], "offLabel", "Edit");
	  Blynk.setProperty(editButton[i], "onLabel", "Edit");
	  Blynk.virtualWrite(editButton[i],0);
	  	  
   }

   Blynk.syncVirtual(MONITOR_GAIN_SLIDER);

   // initialize menu items
   PopulateSessionDropDown();

}

BLYNK_WRITE(OUTPUT_LEVEL) //Output Level Slider
{
   RjamApi_SetOutputLevel(param[0].asInt());
}

BLYNK_WRITE(INPUT_LEVEL)  //Input Level Slider
{
   RjamApi_SetInputLevel(param[0].asInt());
}

static void SetLatencyForSlot(uint8_t slot, uint8_t latency)
{
   printf("Latency set for slot %d\r\n", slot);
   if (RjamApi_SetLatencyForSlot(slot, latency))
   {
      if (RjamApi_GetSlotRole(slot) == ClientRole)
      {
         setEditButtonLabels(slot);
      }
   }
}

BLYNK_WRITE(SLOT1_LATENCY) // Connection 1 buffer adjustment
{
   RjamApi_SetLatencyForSlot(0, param.asInt() - 1);
}

BLYNK_WRITE(SLOT2_LATENCY) // Connection 2 buffer adjustment
{
   RjamApi_SetLatencyForSlot(1, param.asInt() - 1);
}

BLYNK_WRITE(SLOT3_LATENCY) // Connection 3 buffer adjustment
{
   RjamApi_SetLatencyForSlot(2, param.asInt() - 1);
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
      SetSlotRole(i, RjamApi_GetSlotRole(i));

      Blynk.setProperty(connectButton[i], "label", RjamApi_GetSlotName(i));
      sprintf(ip_port, "%s:%d", connections[i].ipAddr, RjamApi_GetSlotPortOffset(i));
      Blynk.setProperty(roleButton[i], "label", ip_port);
      Blynk.virtualWrite(latencySlider[i], connections[i].latency);
      Blynk.virtualWrite(roleButton[i], RjamApi_GetSlotRole(i));
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
   RjamApi_CreateNewSession();
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

BLYNK_WRITE(SAMPLE_RATE) // Sampe Rate setting
{
   int8_t newSampleRate = param.asInt() - 1;

   printf("New sample rate: %d\r\n", newSampleRate);
   if(!RjamApi_SetSampleRate(newSampleRate))
   {
      printf("Unknown sample rate selected: %d\r\n", newSampleRate);
      Blynk.virtualWrite(SAMPLE_RATE, 2);
      Blynk.syncVirtual(SAMPLE_RATE);
      return;
   }
}

void KillSlot(int slot)
{
   Blynk.virtualWrite(connectButton[slot] ,LOW);
   Blynk.setProperty(editButton[slot], "offLabel", "Edit");
   Blynk.setProperty(editButton[slot], "onLabel", "Edit");
   RjamApi_KillSlot(slot);
}

void RouteSlot(int slot)
{
   RjamApi_RouteSlot(slot);

   if (RjamApi_GetSlotRole(slot) == ClientRole)
   {
      setEditButtonLabels(slot);
   }
   Blynk.virtualWrite(connectButton[slot], HIGH);
}

void RouteAllSlots()
{
   int i;

   for(i=0; i<TOTAL_SLOTS; i++)
   {
      if (RjamApi_isConnected(i))
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
      if (RjamApi_isConnected(i))
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
      if (debounceFlag)
      {
         KillSlot(slot);
         RjamApi_OpenSlot(slot);
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
   printf("New IP address requested for slot: %s\n", newIp);
   if (RjamApi_SetSlotIpAddress(slot, newIp))
   {	
      Blynk.virtualWrite(roleButton[slot], HIGH);
   }
   else
   {
      puts("ERROR: IP address rejected.");
      Blynk.setProperty(roleButton[slot], "label", "");
   }
}

void SetSlotRole(uint8_t slot, uint8_t role)
{
   bool updateLabels = false;

   printf("New role for slot %d: %d\n", slot, role);
   if (RjamApi_isConnected(slot))
   {
      updateLabels = true;
   }
   RjamApi_SetSlotRole(slot, role);
   printf("Slot %d connection type is now: %s\r\n", slot, RjamApi_GetConnectionType(slot));
   if (updateLabels)
   {
      if (RjamApi_GetSlotRole(slot) == ClientRole)
      {
         setEditButtonLabels(slot);
      }
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

BLYNK_WRITE(INPUT_SELECT) //Input Selection
{
   RjamApi_InputSelect(param[0].asInt());
}

BLYNK_WRITE(MIC_GAIN) //Mic Gain
{
   if (param[0])
   {
      RjamApi_ChangeMicGain();
      Blynk.setProperty(MIC_GAIN,"offLabel", RjamApi_GetMicGain());
   }
}

void StopClientTest()   //Check to see if a client is in test mode, if so disable the loopback
{
	char msg[100];
	if (slotBeingTested != -1)
	{
		sprintf(msg,"jack_disconnect %s:receive_1 %s:send_1",connections[slotBeingTested].ipAddr,connections[slotBeingTested].ipAddr);  //disable loopback
		printf ("slotbeing edited: %d, jack_disconnect %s:receive_1 %s:send_1\r\n",slotBeingTested,connections[slotBeingTested].ipAddr,connections[slotBeingTested].ipAddr);
		system(msg);
		Blynk.syncVirtual(gainSlider[slotBeingTested]);  // Set gain back to value from the slider
		slotBeingTested = -1;
	}
}


void LatencyTest () // Find latency, report in the app
{

   printf("Latenct test initiated");
   char msg[100];
   char buf[256];
   char latency[24] = ""	;
   float latencyValue;
   FILE *fp;
   int status;
   int ret;
   bool latencyResult = false;
   uint8_t i;
   uint8_t j;
   char *ptr;


   if (slotBeingEdited != -1) {    //Check that an edit button is pressed
      if (RjamApi_VolumeIsEnabled(slotBeingEdited)) {  //Make sure the connection is active

         sprintf(msg, "c-select slot%d", slotBeingEdited);  //Turn down the gain to the outputs so you don't hear the test
         eci_command(msg);
         RjamApi_SetSlotGain(slotBeingEdited, -100);

         printf("slot being edited roll: %d\r\n", RjamApi_GetSlotRole(slotBeingEdited));
         if (RjamApi_GetSlotRole(slotBeingEdited) == ServerRole) {  					//If connection is a server

            if ((fp = popen("unbuffer jack_iodelay", "r")) == NULL) {   //Start jack_iodelay
               printf("Error opening pipe!\n");

            }
            sprintf(msg,"jack_connect slot%d:send_1 jack_delay:out",slotBeingEdited);  //Connect jack io_delay
            printf("jack_connect slot%d:send_1 jack_delay:out\r\n",slotBeingEdited);
            ret = 1;
            while (ret == 1){  //repeat connection attempt until successful
               status = system(msg);
               if (status != -1) { // -1 means an error with the call itself
                  ret = WEXITSTATUS(status);  //get exit code from system command
                  printf("===> Exit Code: %d\r\n",ret);
               }
            }

            sprintf(msg,"jack_connect slot%d:receive_1 jack_delay:in",slotBeingEdited);
            printf("jack_connect slot%d:receive_1 jack_delay:in\r\n",slotBeingEdited);
            system(msg);

            for (j=1;j<11;j++)
            {	
               fgets(buf, 256, fp); 
               ptr = strstr(buf," ms ");
               if (ptr != NULL)
               {
                  strncpy(latency,ptr - 8,12);
                  latencyValue = strtof(latency, NULL);
                  latencyValue = latencyValue/2;
                  sprintf(msg,"%.3f ms",latencyValue);
                  Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX,msg);
                  sprintf(msg,"Latency to %s:",RjamApi_GetSlotName(slotBeingEdited));
                  Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX,msg);
                  printf("%s \r\n",latency);
                  latencyResult = true;
               }						
            }
            if (latencyResult == false)  //No latency data in the results
            {
               Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX,"No latenct results");
               Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX,"Is client in test mode?");
            }
            system("killall jack_iodelay");
            pclose(fp);
            Blynk.virtualWrite(editButton[0],0);
            Blynk.syncVirtual(gainSlider[slotBeingEdited]);  // Set gain back to value from the slider
            slotBeingEdited = -1;

         }
         else  //Connection is a client
         {
            sprintf(msg,"Testing to %s",RjamApi_GetSlotName(slotBeingEdited));
            Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX,msg);
            Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX,"Toggle Test to resume");
            slotBeingTested = slotBeingEdited;
            sprintf(msg,"jack_connect %s:receive_1 %s:send_1",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);  //establish loopback
            printf ("jack_connect %s:receive_1 %s:send_1\r\n",connections[slotBeingEdited].ipAddr,connections[slotBeingEdited].ipAddr);
            system(msg);
         }
      }
   }	
}


void GainSliderChanged(int slider, int value)
{
   RjamApi_SetSlotGain(slider, value);
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
      RjamApi_SetSlotName(slotBeingEdited, param[0].asStr());
      Blynk.setProperty(connectButton[slotBeingEdited], "label", RjamApi_GetSlotName(slotBeingEdited)); // Write user name as label to connect button for active slot
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
      RjamApi_SetSlotPortOffset(slotBeingEdited, strtol(p, NULL, 10));
      connections[slotBeingEdited].slot = slotBeingEdited;

      sprintf(buf, "%s:%d", connections[slotBeingEdited].ipAddr, RjamApi_GetSlotPortOffset(slotBeingEdited));

      Blynk.setProperty(roleButton[slotBeingEdited], "label", buf);
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
   slotBeingEdited = slot;
   StopClientTest();

   if (state)
   {
      //Turn off any other active edit buttons or the 'save session' button
      for(i=0; i<TOTAL_SLOTS; i++) {
         if(i!=slot) {
            Blynk.virtualWrite(editButton[i], LOW);
         }
      }
      Blynk.virtualWrite(SESSION_SAVE_BUTTON, LOW);


      printf ("Slot %d test: %d\r\n", slot, RjamApi_VolumeIsEnabled(slot));

      if (RjamApi_VolumeIsEnabled(slot) == true)
      {
         LatencyTest();
      }
      else
      {
         editMode = EditMode_Connection;

         //Store this button as the currently active edit button
         sprintf(msg, "NAME Connection %d", slot+1);
         Blynk.setProperty(LEFT_EDIT_FIELD_TEXT_BOX, "label", msg);  //Populate the label for the left edit field
         Blynk.virtualWrite(LEFT_EDIT_FIELD_TEXT_BOX, RjamApi_GetSlotName(slot));										  //Seed the data for the left edit field

         Blynk.setProperty(RIGHT_EDIT_FIELD_TEXT_BOX, "label", "IP_ADRESS:Port_Offset");                     //Populate the label for the right edit field
         sprintf(msg, "%s:%d", connections[slot].ipAddr, RjamApi_GetSlotPortOffset(i));
         Blynk.virtualWrite(RIGHT_EDIT_FIELD_TEXT_BOX, msg);                              //Seed the data for the right edit field
      }
   }
   else
   {
      printf("Slot being edited: %d\r\n",slotBeingEdited);

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
   RjamApi_SetSlotGain(-1, param[0].asInt());
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
   StopClientTest();

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

static void setEditButtonLabels(int slot)
{
   Blynk.setProperty(editButton[slot], "offLabel", "Test");
   Blynk.setProperty(editButton[slot], "onLabel", "Test");
}

void setup(const char *auth, const char *serv, uint16_t port)
{
   RjamApi_Initialize(setEditButtonLabels);
   RjamApi_InitializeSoundCard();
   eci_init();	//Initialize Ecasound
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

