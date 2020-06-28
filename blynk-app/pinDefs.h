#ifdef RASPBERRY
#include <BlynkApiWiringPi.h>
#else
#include <BlynkApiLinux.h>
#endif

#ifndef PIN_DEFS_H
#define PIN_DEFS_H

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

#endif // PIN_DEFS_H
