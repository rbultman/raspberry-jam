/*
 * Fe-Pi sound card
 */

#ifndef FE_PI_DEF_H
#define FE_PI_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soundCardDef.h"

class FePi : public SoundcardInterface 
{
   public:
      void Initialize() {
         puts("Initializing the Fe-Pi sound card.");
         system("amixer -M set 'Lineout' 100%");
         system("amixer -M set 'Headphone' 100%");
         system("amixer set 'Headphone Mux' 'DAC'");

         inputMicCommand = "amixer set 'Capture Mux' 'MIC_IN'";
         inputLineCommand = "amixer set 'Capture Mux' 'LINE_IN'";
         mixMasterCommand = "'PCM'";
         mixCaptureCommand = "'Capture'";
         micGainCommand = "amixer set 'Mic' ";
         static const char * gains[] = {"0dB", "+20dB", "+30dB", "+40dB"};
         micGainSettingsCount = sizeof(gains)/sizeof(char *);
         micGainText = gains;

         system("sudo rm /boot/soundcard.txt");
         system("sudo bash -c \"echo 'dtoverlay=fe-pi-audio' >> /boot/soundcard.txt\"");
      };
};

#endif // FE_PI_DEF_H

