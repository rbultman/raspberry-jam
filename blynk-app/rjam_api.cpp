/*
   rjam_api.cpp

   An API for controlling Raspberry Jam from various UIs
*/

#include <libecasoundc/ecasoundc.h>
#include "settings.h"
#include "rjam_api.h"
#include "fe-pi-def.h"
#include "audio-injector-def.h"
#include "HIFI_DAC_ADC_DEF.h"

SessionInfo_T sessionInfo;

static SoundcardInterface *soundcard = NULL;
static FePi fePiCard;
static AudioInjector audioInjectorCard;
static HifiberryDacPlusAdc hifiberryDacPlusAdc;

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

void RjamApi_SetGain(int gain)
{
   char ecaCommand[100];

   sprintf(ecaCommand,"cop-set 1,1,%d",gain);
   printf("Gain: %s\n",ecaCommand);
   eci_command(ecaCommand);
}
