/*
 * Hifiberry DAC + ADC sound card
 */

#ifndef HIFI_DAC_ADC_DEF_H
#define HIFI_DAC_ADC_DEF_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soundCardDef.h"

class HifiberryDacPlusAdc : public SoundcardInterface 
{
   public:
      void Initialize() {
         puts("Initializing the Hifiberry DAC + ADC sound card.");
		 
         system("amixer -M set 'DSP Program' 'Low latency IIR with de-emphasis'");
         system("amixer -M set 'Digital' 100%");

         inputMicCommand = NULL;
         inputLineCommand = NULL;
		 
         mixMasterCommand = "'Analogue'";
			
         mixCaptureCommand = NULL;
         micGainCommand = NULL;
         static const char * gains[] = {NULL};
         micGainSettingsCount = sizeof(gains)/sizeof(char *);
         micGainText = gains;
      };
};

#endif // HIFI_DAC_ADC_DEF_H