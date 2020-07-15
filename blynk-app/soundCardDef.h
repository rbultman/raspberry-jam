/*
 * A sound card definition
 */

#ifndef SOUND_CARD_DEF_H
#define SOUND_CARD_DEF_H

class SoundcardInterface {
   public: 
      virtual void Initialize() = 0;

   public:
      int micGainSettingsCount;

      const char * inputMicCommand;
      const char * inputLineCommand;
      const char * mixMasterCommand;
      const char * mixCaptureCommand;
      const char * micGainCommand;
      const char ** micGainText;
};

#endif // SOUND_CARD_DEF_H

