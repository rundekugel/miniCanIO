//config for can test

#ifndef _config_h
#define _config_h


/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

#define CONFIG_MAX_MSG_ENTRIES  9

#ifdef __cplusplus
 extern "C" {
#endif

enum{
 CONFIG_SPEED_invalid,
 CONFIG_SPEED_10k,
 CONFIG_SPEED_100k,
 CONFIG_SPEED_125k,  
 CONFIG_SPEED_250k,  
 CONFIG_SPEED_500k,  
 CONFIG_SPEED_1000k,
 CONFIG_SPEED_invalid2 = 0xff
};

typedef struct _Scancfgmsg{
  uint32_t msg_id;
  uint8_t bytePos;
  uint8_t bitMask;
  uint8_t verifyType;
  uint8_t verifyValue;
  uint8_t switchType;
  uint8_t outputPin;
}Scancfgmsg, Pcancfgmsg;

typedef struct _Scanconfig{
    uint32_t valid;
    uint8_t key[16];
    uint16_t version;
    uint16_t canSpeed;
    uint32_t configMsgIdRx;
    uint32_t configMsgIdTx;
    uint32_t pinResetState;
    //union { 
      uint32_t ack:1;
      uint32_t noRetransmission:1;
      uint32_t wakeup:1;  //for future use
      uint32_t res:29;
      //uint32_t ack_nor_wu_combo;
    //};
    Scancfgmsg msgCfg[CONFIG_MAX_MSG_ENTRIES];
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
