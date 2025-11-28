
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
//#include "main.h"

#include "stm32f103xb.h"
//#include "stm32f10x.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"
#include "string.h"
#include "config.h"


/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart1;

/* Private variables ---------------------------------------------------------*/
#define USE_T_TX 0

CAN_FilterConfTypeDef sFilterConfig;  // used multiple times as temporary config storage
__attribute__ ((aligned (4))) CanTxMsgTypeDef TxMessage;
__attribute__ ((aligned (4))) CanRxMsgTypeDef RxMessage;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_CAN_Init(void);
static void Serialprintln(char _out[]);
static void newline(void);
static void setFilters(Pcanconfig config);
void switchOnMsg(CanRxMsgTypeDef* rxmsg, Pcanconfig config);
void setGPIOoutput(int pinNumber);

#if USE_T_TX >0
HAL_StatusTypeDef T_HAL_CAN_Transmit(CAN_HandleTypeDef* hcan, uint32_t Timeout);
#endif

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  static int unlocked = 0;

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  GPIOC->BSRR = GPIO_PIN_13; //test 1 = an (low active)
  
  Pcanconfig config = config_get();
  
  if(!config->valid || config->dbgOutput){
    MX_USART1_UART_Init();
	Serialprintln("Welcome");
	Serialprintln("UART and GPIO is initiated");
	Serialprintln("Clock configured ");
	Serialprintln("HAL initiated");
	Serialprintln("Going to try to initiate MX_CAN");
	Serialprintln("INITIALISING CAN BUS NOW");
  }
  MX_CAN_Init();

  //overwrite can pin config for debug open-drain mode

  /**CAN GPIO Configuration    
  PB8     ------> CAN_RX
  PB9     ------> CAN_TX 
  */
  if(config->dbgDirectConnetion){
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }


  GPIOC->BRR = GPIO_PIN_13; //test 0 = an
      HAL_Delay(500);
      GPIOC->BSRR = GPIO_PIN_13; //test 0 = aus
      Serialprintln("Setting the Messages and perameters");
      
      if(config->extendedIds){
        RxMessage.IDE = CAN_ID_EXT;
        TxMessage.IDE = CAN_ID_EXT;
      }else{
        RxMessage.IDE = CAN_ID_STD;
        TxMessage.IDE = CAN_ID_STD;
      }
      // RxMessage.RTR = CAN_RTR_DATA;

      Serialprintln("Starting with sFilterConfig");	
      setFilters(config);

      //filter 2 for control msg
      sFilterConfig.FilterIdHigh = (config->configMsgIdRx)<<5;
      sFilterConfig.FilterMaskIdHigh = 0xffff;
      sFilterConfig.FilterNumber = 2;
      sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
      HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
      
      Serialprintln("All linked up to hcan");
      
      Serialprintln("Now for the Tx side of it");
      
      TxMessage.StdId = config->configMsgIdTx ;
      TxMessage.RTR = CAN_RTR_DATA;
     
      TxMessage.DLC = 8;
      memcpy(&TxMessage.Data , (const uint8_t*)"11111111", 8);
      hcan1.pTxMsg = &TxMessage;
      hcan1.pRxMsg = &RxMessage;
      hcan1.pRx1Msg = &RxMessage;

      
      Serialprintln("Message data configured");
      Serialprintln("Linking it to the hcan ");
      
      GPIOC->BSRR = GPIO_PIN_13; //test 0 = an

  /* Infinite loop */
      HAL_Delay(500);
      uint32_t i=1;
      uint8_t hartbeat = 0;
      uint32_t* data32 = (uint32_t*)(TxMessage.Data);
      #define timeout_ms 1 
      char uartbuf[20];
      int sendit = 0;

      Serialprintln("In the loop now");
      Serialprintln("Poll message..."); 
      printf("Enter loop.");
      while (1)
      {
        HAL_Delay(10);
        GPIOC->ODR ^= GPIO_PIN_13; //test 0 = an
        
        if(sendit>0 && config->configMsgIdTx>0){
          sendit--;
          data32[1] = i++;
          if(config->dbgOutput){
            Serialprintln("Trying to send a message");
          }
          HAL_CAN_Transmit(&hcan1, timeout_ms);                
          if(config->dbgOutput){
            Serialprintln("Message sent");
          }
        }
        
        if(__HAL_CAN_MSG_PENDING(&hcan1, CAN_FIFO0))    {
          HAL_CAN_Receive(&hcan1, CAN_FIFO0, 1);  //todo: use HAL_CAN_Receive_IT with callbacks for rx.
          switchOnMsg(hcan1.pRxMsg, config);
          if(config->dbgOutput){
            printf("rx0: %x, %x, %x\r\n", RxMessage.StdId, RxMessage.DLC,
                 RxMessage.Data[4]|(RxMessage.Data[5]<<8));                
            //Serialprintln("rx:")
            HAL_UART_Transmit(&huart1, (uint8_t *)"rx0:" , 3, 10);
            HAL_UART_Transmit(&huart1, &(RxMessage.Data[4]) , 1, 10);
            newline();
          }
        }
        #if 1 //fifo1 used
        if(__HAL_CAN_MSG_PENDING(&hcan1, CAN_FIFO1))    {                                           
          HAL_CAN_Receive(&hcan1, CAN_FIFO1, 1);
          if(config->dbgOutput){
            printf("rx1: %i, %i, %i\r\n", RxMessage.StdId, RxMessage.DLC, RxMessage.Data[4]);
            HAL_UART_Transmit(&huart1, (uint8_t *)"rx1:" , 3, 10);
            HAL_UART_Transmit(&huart1, &(RxMessage.Data[4]) , 1, 10);
            newline();
          }

          if(RxMessage.StdId == config->configMsgIdRx){  // do config
            TxMessage.Data[0] = RxMessage.Data[0];  // 1st byte is cmd
            TxMessage.Data[1] = 0xe1; //default return value is error.
            switch(RxMessage.Data[0]){
            case 0x11:  //init
              config->valid = 0;
              config = config_get();
              MX_CAN_Init();
              setFilters(config);
            case 0xc0:  //unlock/lock
              if(memcmp(RxMessage.Data+1, &(config->key[unlocked*6]), 6)) {
                unlocked = 0; //invalid key!
              }else{
                unlocked++;
                if(unlocked >=3){
                  unlocked = 3;
                }
              }
              TxMessage.Data[1] = 3-unlocked;
              break;
            case 0xcf:  //read config 6 bytes
              config_getUserData(RxMessage.Data[1] *6, &(TxMessage.Data[2]), 6);
              TxMessage.Data[1] = RxMessage.Data[1] ;
              break;
            case 0xc4:  //read config values by id
              //not implemented, yet.
              //need config id table here.
              //config_getUserData(RxMessage.Data[1] *6, &(TxMessage.Data[2]), 6);
              //TxMessage.Data[1] = 0xff;
              break;
            case 0xce:  //edit config [pos, size, data]
              if(unlocked<3){
                break;
              }
              {
                uint8_t size = RxMessage.Data[2]; 
                if(size > 4) size=4;
                TxMessage.Data[1] = config_editDirectMemory(RxMessage.Data[1], &(RxMessage.Data[3]), size);
              }
              break;
            case 0xc2:  //edit config with fixed size=5 and pos is factor 5 [pos, data]
              if(unlocked<3){
                break;
              }
              {
                uint8_t size = 5;
                config_editDirectMemory(RxMessage.Data[1], &(RxMessage.Data[2]), size);
                TxMessage.Data[1] = RxMessage.Data[1];
              }
              break;
              /*
            case 0xc3:  //edit config with fixed size=5 and pos is factor 5 [pos, data]
              if(unlocked<3){
                break;
              }
              {
                uint8_t size = 5;
                config_editDirectMemory(RxMessage.Data[1], &(RxMessage.Data[2]), size);
                TxMessage.Data[1] = RxMessage.Data[1];
              }
              break;
              */
            case 0xc5:  //reset config to defaults
              if(unlocked<3){
                break;
              }
              config_writeDefaults();
              TxMessage.Data[1] = 0;
              break;
            
            case 0xc9:  //write config from ram to flash
              if(unlocked<3){
                break;
              }
              config_write(config_get());
              TxMessage.Data[1] = 0;
              break;
            }
            HAL_CAN_Transmit(&hcan1, 10);
          }
        }
        #endif
        //RxMessage.DLC = 0;
        if(config->dbgOutput){
          uint8_t d= '@'|(hartbeat & 0x1f );
          HAL_UART_Transmit(&huart1, &d , 1, 10);
        }
        hartbeat++;
                  
  }

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

int searchMsgId(void){
  //search in filters
  return 0;
}

void setFilters(Pcanconfig config){
  //read from config and write to registers
  //defaults:
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.BankNumber = 14;  //last bank for can1 = 1st bank for can2
  if(config->filtersAreList){
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
  }else{
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  }

  Pcancfgmsg msgCfg = config->msgCfg;

  for(int filterN=0; filterN < CONFIG_MAX_MSG_ENTRIES; filterN++){
    if(msgCfg[filterN].msg_id ==0)
      continue;
    if(msgCfg[filterN].msg_id ==0xffff)
      continue;

    sFilterConfig.FilterNumber = filterN +1;
    if(config->extendedIds){
      sFilterConfig.FilterIdLow = msgCfg[filterN].msg_id & 0xffff;
      sFilterConfig.FilterIdHigh = (msgCfg[filterN].msg_id <<16)& 0x3fff;
      sFilterConfig.FilterMaskIdLow = 0xffff;
      sFilterConfig.FilterMaskIdHigh = 0x3fff;
    }else{
      sFilterConfig.FilterIdHigh = msgCfg[filterN].msg_id <<5;
      sFilterConfig.FilterMaskIdHigh = 0xffff <<5;
    }    
    HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    setGPIOoutput(msgCfg->outputPin);
  }
}

/* CAN init function */
static void MX_CAN_Init(void)
{
  #define CAN_BS1_13TQ                ((uint32_t)(CAN_BTR_TS1_3 | CAN_BTR_TS1_2))                  /*!< 13 time quantum */
  Pcanconfig cfg = config_get();
  hcan1.Instance = CAN1;

  //default settings for 500k
  hcan1.Init.Prescaler = 9;  
  hcan1.Init.SJW = CAN_SJW_1TQ;
  hcan1.Init.BS1 = CAN_BS1_3TQ;
  hcan1.Init.BS2 = CAN_BS2_4TQ;

  //BRP = (FPCLK / (BaudRate x (TS1 + TS2 + 3))) - 1
  switch(config_get()->canSpeed){
    case 125:
      hcan1.Init.Prescaler = 18;  //test!
      hcan1.Init.BS1 = CAN_BS1_13TQ;
      hcan1.Init.BS2 = CAN_BS2_2TQ;
      break;
    case 250:
      hcan1.Init.Prescaler = 9;   //250k
      hcan1.Init.BS1 = CAN_BS1_13TQ;
      hcan1.Init.BS2 = CAN_BS2_2TQ;
      break;
    case 500:
      hcan1.Init.Prescaler = 9;  
      break;
    case 1000:
      hcan1.Init.Prescaler = 4;  //todo set CAN_time_quantum
      break;
    default:
      break;  //500k
  }

  if(config_get()->ack){
    hcan1.Init.Mode = CAN_MODE_NORMAL;
  }else{
    hcan1.Init.Mode = CAN_MODE_SILENT;
  }

  hcan1.Init.TTCM = DISABLE;
  hcan1.Init.ABOM = DISABLE;
  hcan1.Init.AWUM = DISABLE;
  hcan1.Init.NART = config_get()->noRetransmission;  // NART = no automatic retransmission
  hcan1.Init.RFLM = DISABLE;
  hcan1.Init.TXFP = DISABLE;

  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
		newline();
		Serialprintln("/////////////////////////////////////////");
		Serialprintln("/////////////////////////////////////////");
		Serialprintln("ERROR INITIATING CAN BUS");

    _Error_Handler(__FILE__, __LINE__);
  }
	else{
		Serialprintln("CAN BUS INITIATED");
	}

  __HAL_CAN_DBG_FREEZE(&hcan1, 0);
}


void setGPIOoutput(int pinNumber){
  //pinnumber is from 0..63 for PinA.0 to PinD.15
  GPIO_TypeDef* gpio = GPIOA;
  if(pinNumber > 15)
    gpio = GPIOB;
  if(pinNumber > 31)
    gpio = GPIOC;
  if(pinNumber > 47)
    gpio = GPIOD;
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = 1 < pinNumber;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; 
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);
}

void switchGPIO(int pin, enum CFG_SWITCH_TYPE action){
  GPIO_TypeDef* gpio = GPIOA;
  if(pin > 47)
    gpio = GPIOD;
  else if(pin > 31)
    gpio = GPIOC;
  else if(pin > 15)
    gpio = GPIOB;
  
  switch(action){
  case CONFIG_SWITCH_TYPE_ON:
    gpio->BRR = 1<<pin;
    break;
  case CONFIG_SWITCH_TYPE_OFF:
    gpio->BSRR = 1<<pin;
    break;
  case CONFIG_SWITCH_TYPE_TOGGLE:
    gpio->ODR ^= 1<<pin;
    break;
  default:
    break;
  }
}

void switchOnMsg(CanRxMsgTypeDef* rxmsg, Pcanconfig config){
  Pcancfgmsg cfgmsg = config->msgCfg;

  for(int n =0; n<CONFIG_MAX_MSG_ENTRIES; n++){
    if(cfgmsg->msg_id == rxmsg->StdId){ //search all
      uint8_t val = rxmsg->Data[cfgmsg->bytePos] & cfgmsg->bitMask;
      uint8_t soll = cfgmsg->verifyValue;
      switch(cfgmsg->verifyType){
        case CONFIG_VERIFY_TYPE_EQUAL:
          if(val == soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
        case CONFIG_VERIFY_TYPE_NOT_EQUAL:
          if(val != soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
        case CONFIG_VERIFY_TYPE_GREATER:
          if(val > soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
        case CONFIG_VERIFY_TYPE_SMALLER:
          if(val < soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
        case CONFIG_VERIFY_TYPE_AND:
          if(val & soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
        case CONFIG_VERIFY_TYPE_XOR:  //does this make sense?
          if(val ^ soll) switchGPIO(cfgmsg->outputPin, cfgmsg->switchType); 
          break;
      }
    }
  }
}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure Pins
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  //HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Serialprintln(char _out[]){	
  if(!config_get()->dbgOutput)
    return;
	HAL_UART_Transmit(&huart1, (uint8_t *) _out, strlen(_out), 10);
	char newline[2] = "\r\n";
	HAL_UART_Transmit(&huart1, (uint8_t *) newline, 2, 10);
}

void newline(void){
  if(!config_get()->dbgOutput)
    return;
  char newline[2] = "\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *) newline, 2, 10);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
