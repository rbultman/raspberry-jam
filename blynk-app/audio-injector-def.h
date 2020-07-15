/*
 * Audio Injector sound card
 */

#ifndef AUDIO_INJECTOR_DEF_H
#define AUDIO_INJECTOR_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soundCardDef.h"

class AudioInjector : public SoundcardInterface 
{
   public:
      void Initialize() {
         system("amixer set 'Output Mixer HiFi' on");

         inputMicCommand = "amixer set 'Input Mux' 'Mic' && amixer set 'Mic' cap && amixer set 'Line' nocap";
         inputLineCommand = "amixer set 'Input Mux' 'Line In' && amixer set 'Line' cap && amixer set 'Mic' nocap";
         mixMasterCommand = "'Master'";
         mixCaptureCommand = "'Capture'";
         micGainCommand = "amixer set 'Mic Boost' ";
         static const char *gains[] = {"0dB", "+20dB"};
         micGainSettingsCount = sizeof(gains)/sizeof(char *);
         micGainText = gains;
      };
};

#endif // AUDIO_INJECTOR_DEF_H

