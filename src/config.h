//config for can test

#ifndef _config_h
#define _config_h


/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

#define CONFIG_MAX_MSG_ENTRIES  9

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum CFG_VERIFY_TYPE{
 CONFIG_VERIFY_TYPE_EQUAL,
 CONFIG_VERIFY_TYPE_NOT_EQUAL,
 CONFIG_VERIFY_TYPE_GREATER,
 CONFIG_VERIFY_TYPE_SMALLER,
 CONFIG_VERIFY_TYPE_AND,
 CONFIG_VERIFY_TYPE_XOR,  //does this make sense? could be done in config with EQUAL and (not value)
 CONFIG_VERIFY_TYPE_INVALID = 0xff
 // senseless: CONFIG_VERIFY_TYPE_OR,
}eCFG_VERIFY_TYPE;

typedef enum CFG_SWITCH_TYPE{
 CONFIG_SWITCH_TYPE_ON,
 CONFIG_SWITCH_TYPE_OFF,
 CONFIG_SWITCH_TYPE_TOGGLE,
 /* //not implemented, yet
 CONFIG_SWITCH_TYPE_BLINK,  
 CONFIG_SWITCH_TYPE_TIME,
 CONFIG_SWITCH_TYPE_PWM,
 CONFIG_SWITCH_TYPE_FREQ,
 */
 CONFIG_SWITCH_TYPE_INVALID = 0xff
}eCFG_SWITCH_TYPE;


typedef struct _Scancfgmsg{
  uint32_t msg_id:29;
  uint32_t reserved:1;
  uint32_t msg_id_is_ext:1;
  uint8_t bytePos;
  uint8_t bitMask;
  uint8_t verifyType;
  uint8_t verifyValue;
  uint8_t switchType;
  uint8_t outputPin;
}Scancfgmsg, *Pcancfgmsg; //size: 4 +6 = 10

typedef struct _Scanconfig{
    uint32_t valid;
    uint8_t key[16];
    uint16_t version;   // pos20 = 0x14
    uint16_t canSpeed;  //pos 22 = 0x16
    uint32_t configMsgIdRx;
    uint32_t configMsgIdTx;
    uint32_t pinResetState; //0x20 = 32

    uint32_t ack:1;
    uint32_t noRetransmission:1;
    uint32_t wakeup:1;  //for future use
    uint32_t extendedIds:1;
    uint32_t filtersAreList:1;
    uint32_t res:27;

    Scancfgmsg msgCfg[CONFIG_MAX_MSG_ENTRIES];  // pos40

    //union {
      uint32_t dbgSleepMode:1;
      uint32_t dbgOutput:1;
      uint32_t dbgShowAllMsg:1;
      uint32_t dbgDirectConnetion:1;
      uint32_t dbgReserved:28;
      //uint32_t dbgCombo;
    //};
    //}_debug;
} Scanconfig, *Pcanconfig;


Pcanconfig config_get(void);
int config_write(Pcanconfig config);  //write config to flash
void config_setValid(Pcanconfig config);
void config_writeDefaults(void);
int config_getUserData(int pos, uint8_t* data, int maxLen);
int config_editDirectMemory(int pos, uint8_t* data, int maxLen);

#ifdef __cplusplus
 {extern "C" {}
#endif

#endif  //_config_h
//eof
