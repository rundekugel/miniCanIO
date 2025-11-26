//config for can test

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "string.h"
//
#include "stm32f103xb.h"
//#include "stm32f1xx_hal_def.h"
#if 0
//#include "stm32f1xx_hal_flash.h"
#else
  //error if include stm32...flash.h, so try to do define here
  #define FLASH_TYPEPROGRAM_HALFWORD             0x01U  /*!<Program a half-word (16-bit) at a specified address.*/
  #define FLASH_TYPEPROGRAM_WORD                 0x02U  /*!<Program a word (32-bit) at a specified address.*/
  #define FLASH_TYPEPROGRAM_DOUBLEWORD           0x03U  /*!<Program a double word (64-bit) at a specified address*/

  int HAL_FLASH_Unlock(void);
  void    FLASH_PageErase(uint32_t PageAddress);
  int HAL_FLASH_Lock(void);
  int HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
#endif

#define CONFIG_END_OF_FLASH 0x4000
#define CONFIG_MAX_SIZE  0x100
#define CONFIG_STARTADDRESS (CONFIG_END_OF_FLASH - CONFIG_MAX_SIZE)
#define CONFIG_VALID 0x55aa0033

#if 1 //use default config in ram, which will be overridden by flash content
Scanconfig m_configS = {
  .valid = 0,
  .key = {0,0,0,0,0,0,0,0},
  .configMsgIdRx = 0xbb,
  .configMsgIdTx = 0xb9,
  .canSpeed = CONFIG_SPEED_250k, //CONFIG_SPEED_500k,
  .pinResetState = 0xffffFFFF,
  //.ack_nor_wu_combo = 1 | (0<<1) | (0<<2) | (0<<3), //ack //noRetransmission //wakeup // reserved
  .ack=1,
  .noRetransmission=0,
  .wakeup = 1,
  .res = 3,
  //.dbgCombo = 0,
  .dbgOutput = 1,
  .dbgDirectConnetion = 0
};
Pcanconfig m_config = &m_configS;
#else
  Pcanconfig m_config = (Pcanconfig)CONFIG_STARTADDRESS;
#endif

/*
void config_init(){
  memcpy(&m_configS, (void*)CONFIG_STARTADDRESS, sizeof(m_configS));
}
*/

Pcanconfig config_get(void){
  //todo assert(CONFIG_MAX_SIZE < sizeof(Scanconfig));
  if(m_config->valid != CONFIG_VALID){
    memcpy(&m_configS, (void*)CONFIG_STARTADDRESS, sizeof(m_configS));
  }

  if(m_config->valid != CONFIG_VALID){
    config_writeDefaults();
  }
  return m_config;
}

void config_setValid(Pcanconfig config){
  config->valid = CONFIG_VALID;
}

int config_write(Pcanconfig config){
  //write config to flash
  if(config->valid != CONFIG_VALID){
    return 1;
  }
  uint32_t* data = (uint32_t*)config;  
  //init flash and write...
  HAL_FLASH_Unlock();
  FLASH_PageErase(CONFIG_STARTADDRESS);
  //CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
  ((FLASH->CR) &= ~(FLASH_CR_PER));
  HAL_FLASH_Lock();
  HAL_FLASH_Unlock();
  for(uint32_t addr = CONFIG_STARTADDRESS; addr < CONFIG_STARTADDRESS + sizeof(Scanconfig); addr+=4){
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data[0]);
    data +=1;
  }
  HAL_FLASH_Lock();
  return 0;
}

void config_writeDefaults(void){
  Scanconfig cfg = {};

  memcpy(&(cfg.key), (uint8_t[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},16);
  cfg.canSpeed = CONFIG_SPEED_250k, //CONFIG_SPEED_500k
  cfg.configMsgIdRx = 0xbb;
  cfg.configMsgIdTx = 0xb9; // if 0, then don't use.
  //
  //for(uint32_t i=0; i<CONFIG_MAX_MSG_ENTRIES){
  //  cfg.msgCfg[i]=0;
  //}
  cfg.ack = 1;
  cfg.wakeup = 1;  //for future use
  cfg.pinResetState = 0xffffFFFF;
  cfg.noRetransmission = 0;
  cfg.dbgDirectConnetion = 0;
  cfg.dbgOutput = 0;
  cfg.dbgSleepMode = 1; 

  config_setValid(&cfg);
  config_write(&cfg);
}

int config_getUserData(int pos, uint8_t* data, int maxLen){
  //int size;
  uint8_t* firstAllowedAddress = (uint8_t*)&(m_config->configMsgIdRx);
  uint32_t maxsize = sizeof(Scanconfig)-(firstAllowedAddress -((uint8_t*)&m_config));
  //if(pos >= maxsize){
  //  return 0;
  //}
  int i=0;
  while((pos+i)<maxsize){
    data[i] = firstAllowedAddress[pos +i];
    i++;
    if(i>=maxLen){
      break;
    }
  }
  return i;
}

int config_editDirectMemory(int pos, uint8_t* data, int maxLen){
  uint8_t* firstAllowedAddress = (uint8_t*)m_config;
  uint32_t maxsize = sizeof(Scanconfig);
  int i=0;
  while((pos+i)<maxsize){
    firstAllowedAddress[pos +i] = data[i];
    i++;
    if(i>=maxLen){
      break;
    }
  }
  return i;
}

//eof