# miniCanIO
Small CAN device based on STM32F1xx. Switch anything based on any CAN-messages.

*project is in an early development state. contribute, read, test, or come later.*

## features
- Configurable via CAN 
- config write potection 
- config message can id can be changed
- supports 125kb/s, 250kb/s and 500kb/s 
- until now: 16kB flash memory is enough

## config
### default settings
- CAN Speed 500kb/s
- listening on config CAN-ID 0xBB
- sending return value at CAN-ID 0xB9
- acknowledge CAN-Messages
- config key is: 16x 0x00

### read settings
sample msg: ```0BB: CF <block number>```   
returns: ```0B9: CF <block number> <6 bytes of data>```  
returns 6 bytes of config data starting at offset 6*```<block number>```

### change settings
1. unlock  
	sample msg: ```0BB: C0 <key bytes 0..5>```  	
	sample msg: ```0BB: C0 <key bytes 6..11>```  
	sample msg: ```0BB: C0 <key bytes 12..15>```
2. edit config (position, length, up to 4 data bytes)  
change can-rx-id to ```0x00CA```  
sample msg: ```BB CE 1C 02 CA 00```  
3. write changes to flash
```BB C9```
4. lock 
```BB C0```

### config structs
```
typedef struct _Scanconfig{
    uint32_t valid;
    uint8_t key[16];    // default 0x00 0x00 0x00...
    uint16_t version;   // pos20 = 0x14
    uint16_t canSpeed;  // pos 22 = 0x16
    uint32_t configMsgIdRx;		// pos 24
    uint32_t configMsgIdTx;     // pos 28
    uint32_t pinResetState; // pos 32 = 0x20
    // pos 36 = 0x24
    uint32_t ack:1;
    uint32_t noRetransmission:1;
    uint32_t wakeup:1;  //for future use
    uint32_t extendedIds:1;
    uint32_t filtersAreList:1;
    uint32_t res:27;
    // pos40 = 0x26
    Scancfgmsg msgCfg[9];  
} Scanconfig, *Pcanconfig;
```
```
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
}Scancfgmsg, *Pcancfgmsg; //size: 3*4 +6 = 22
```

## python script for reading and editing config data
to be done

## DBC file for CAN config messages
work in progress

