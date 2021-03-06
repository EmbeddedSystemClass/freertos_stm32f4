/**
  ******************************************************************************
  * @file    src/main.c 
  * @author  Harald W. Leschner (DK6YF) - H9 Laboratory Ltd.
  * @version V1.0.0
  * @date    15-July-2017
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
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
#include <string.h>

#include "config.h"

/* XCore drivers */
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "XCore407I.h"

/* Hardware peripherals */
#include "uart.h"
#include "rtc_clock.h"
#include "eth_if.h"
#include "flashdrive.h"
#include "adc.h"

#include "onewire.h"
#include "ds18b20.h"

/* GUI and interfaces */
#include "gfx.h"
#include "gui.h"
#include "pages/zen_menu.h"
//#include "vt100.h"
//#include "test.h"

/* LWIP stack */
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dns.h"

/* NTP, DNS and DHCP */
#include "lwip/apps/sntp.h"
#include "dhcp_eth.h"

/* HTTP server */
#include "httpd_server.h"
//#include "test_server.h"

/* Clients */
#include "httplog.h"
//#include "lwip/apps/mqtt.h"
#include "MQTTlwip.h"
#include "MQTTClient.h"


/* FatFs includes component */
#include "ff_gen_drv.h"
#include "usbh_diskio.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LED1_UPDATE_DELAY   125
#define LED2_UPDATE_DELAY   250



/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
osThreadId LEDThread1Handle, LEDThread2Handle;

  
FATFS   USBDISKFatFs;           /* File system object for USB disk logical drive */
char    USBDISKPath[4];         /* USB Host logical drive path */

FIL     MyFile;                 /* File object */

USBH_HandleTypeDef hUSB_Host;   /* USB Host handle */

typedef enum {
  DISCONNECTION_EVENT = 1,  
  CONNECTION_EVENT,    
}MSC_ApplicationTypeDef;

typedef enum {
  DISK_READY_EVENT = 1,  
  DISK_REMOVE_EVENT,    
}FatFs_DiskTypeDef;


osMessageQId AppliEvent;
osMessageQId DiskEvent;




/* Private function prototypes -----------------------------------------------*/
static void os_init(void);
static void os_tasks(void);
static void SystemClock_Config(void);

static void Register_printout(void);


/* Threads and startup tasks */
static void LED1_thread (void const * arg);   /* Blink LED 1 */
static void LED2_thread (void const * arg);   /* Blink LED 2 */

static void GUI_start   (void const * arg);   /* Initiate LCD and uGFX */
static void GUI_thread  (void const * arg);   /* Operate GUI */

static void NET_start   (void const * arg);   /* ETH Networking */
static void HTTP_start  (void const * arg);   /* HTTP Server */

static void MQTT_start  (void const * arg);   /* MQTT Server + Client */
//static void MQTT_connect(mqtt_client_t *client);


static void TTY_thread  (void const * arg);   /* TTY Console command interpreter */
static void RTC_thread  (void const * arg);   /* Real Time Clock */
static void ADC_thread  (void const * arg);   /* Read ADC input */

//static void OW_thread   (void const * arg);   /* Read ONEWIRE Temperature Sensor */


/* USB Tasks */
static void USB_thread  (void const * arg);
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
static void MSC_Application(void);

static void DISK_thread (void const * arg);




/* Private functions ---------------------------------------------------------*/
static void os_init(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();
  
  /* Configure the system clock to 168 MHz */
  SystemClock_Config();
  HAL_NVIC_SetPriority(SysTick_IRQn, 0x0, 0x0);

  /* Configure LEDs */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);

  /* Configure UART */
  uart_init();

  /* Configure RTC */
  rtc_init();
  RTC_CalendarConfig(17, 1, 1, 0, 0, 0);

  Register_printout();

  /* Configure ADC Temperature Sensor */
  //adc_temp_init();

}


/* Task definitions */
static void os_tasks(void)
{

  /* LED 1 */
  osThreadDef(led1, LED1_thread,   osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 1);
  LEDThread1Handle = osThreadCreate( osThread(led1),  NULL);

  /* LED 2 */
  osThreadDef(led2, LED2_thread,   osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 1);
  LEDThread2Handle = osThreadCreate( osThread(led2),  NULL);

  /* GUI */
  osThreadDef(sgui, GUI_start,     osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate( osThread(sgui),  NULL);

  /* NETWORK */
  osThreadDef(snet, NET_start,     osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate( osThread(snet),  NULL);

  /* RTC */
  //osThreadDef(rtc0, RTC_thread,    osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 1);
  //osThreadCreate( osThread(rtc0),  NULL);

  /* ADC */
  //osThreadDef(adc0, ADC_thread,    osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 1);
  //osThreadCreate( osThread(adc0),  NULL);

  /* ONEWIRE */
  osThreadDef(ow0, OW_thread,      osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate( osThread(ow0),   NULL);



#if 0
  /*##-1- Start task #########################################################*/
  osThreadDef(USB_drv, USB_thread,   osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 8);
  osThreadCreate(osThread(USB_drv), NULL);
  
  osThreadDef(USB_fat, DISK_thread,  osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 8);
  osThreadCreate (osThread(USB_fat), NULL);  

  /*##-2- Create Application Queue ###########################################*/
  osMessageQDef(usb_queue,  1, uint16_t);
  AppliEvent = osMessageCreate(osMessageQ(usb_queue), NULL);

  /*##-3- Create Disk Queue ##################################################*/
  osMessageQDef(disk_queue, 1, uint16_t);
  DiskEvent  = osMessageCreate (osMessageQ(disk_queue), NULL);
#endif

}

/* Total amount of STACK_SIZE allocated: 34 * 128 = 4352 Bytes */

int main(void)
{

  /* Initialize modules and hardware */
	os_init();

  /* Create all tasks */
  os_tasks();

  /* Start scheduler - should never leave this ;) */
  osKernelStart();

	for(;;)

  /* Should never ever get here ..*/
	return 0;
}





















/**
  * @brief  Start task
  * @param  pvParameters not used
  * @retval None
  */
static void USB_thread(void const * arg)
{

  (void) arg;

  osEvent event;
  
  /* Link the USB Host disk I/O driver */
  if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
  {
    writef("Driver linked\r\n");

    /* Init Host Library */
    USBH_Init(&hUSB_Host, USBH_UserProcess, 0);
    
    /* Add Supported Class */
    USBH_RegisterClass(&hUSB_Host, USBH_MSC_CLASS);
    
    /* Start Host Process */
    USBH_Start(&hUSB_Host);
    
    for( ;; )
    {
      event = osMessageGet(AppliEvent, osWaitForever);
      
      if(event.status == osEventMessage)
      {
        switch(event.value.v)
        {
        case CONNECTION_EVENT:
          MSC_Application();
          break;
          
        case DISCONNECTION_EVENT:
          f_mount(NULL, (TCHAR const*)"", 0);
          break;
          
        default:
          break;
        }
      }
    }
  }
}


/**
  * @brief  Main routine for Mass Storage Class
  * @param  None
  * @retval None
  */
static void MSC_Application(void)
{
  
  //FRESULT res;                                          /* FatFs function common result code */
  //uint32_t byteswritten, bytesread;                     /* File write/read counts */
  //uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
  //Suint8_t rtext[100];                                   /* File read buffer */
  
  GFILE* imgFile;
  static gdispImage myImage;


  /* Register the file system object to the FatFs module */
  if(f_mount(&USBDISKFatFs, (TCHAR const*)USBDISKPath, 0) != FR_OK)
  {
    /* FatFs Initialization Error */
    Error_Handler();
  }


#if 1
  else {

    /* Allow Second task to have access to FatFs */
    osMessagePut(DiskEvent, DISK_READY_EVENT, 0);

  }
#endif



  //else
  //{


#if 0
    /* Create and Open a new text file object with write access */
    if(f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) 
    {
      Error_Handler();
    }

    else {

      f_close(&MyFile);
      /*End test for file existence*/

      imgFile = gfileOpen("tenor.gif", "rb");

      gdispImageOpenGFile(&myImage, imgFile);

      //taskENTER_CRITICAL();
      //gdispImageDraw(&myImage, 0, 0, 320, 240, 0, 0);
      //taskEXIT_CRITICAL();
      
      delaytime_t delay;

      while (1) {
        gdispImageDraw(&myImage, 0, 0, 320, 240, 0, 0);

        delay = gdispImageNext(&myImage);

        gfxSleepMilliseconds(100);
      }

      gdispImageClose(&myImage);

      gfileClose(imgFile);
    }
#endif





#if 0

    else
    {
      /* Write data to the text file */
      res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);
      
      if((byteswritten == 0) || (res != FR_OK))
      {
        /* 'STM32.TXT' file Write or EOF Error */
        Error_Handler();
      }
      else
      {
        /* Close the open text file */
        f_close(&MyFile);
        
        /* Open the text file object with read access */
        if(f_open(&MyFile, "STM32.TXT", FA_READ) != FR_OK)
        {
          /* 'STM32.TXT' file Open for read Error */
          Error_Handler();
        }
        else
        {
          /* Read data from the text file */
          res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);
          
          if((bytesread == 0) || (res != FR_OK))
          {
            /* 'STM32.TXT' file Read or EOF Error */
            Error_Handler();
          }
          else
          {
            /* Close the open text file */
            f_close(&MyFile);
            
            /* Compare read data with the expected data */
            if((bytesread != byteswritten))
            {                
              /* Read data is different from the expected data */
              Error_Handler();
            }
            else
            {
              /* Success of the demo: no error occurrence */
              BSP_LED_On(LED4);
            }
          }
        }
      }
    }
#endif

  //}
  
  /* Unlink the USB disk I/O driver */
  //FATFS_UnLinkDriver(USBDISKPath);

}


/**
  * @brief  User Process
  * @param  phost: Host handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{  
  switch(id)
  { 
  case HOST_USER_SELECT_CONFIGURATION:
    break;
    
  case HOST_USER_DISCONNECTION:
    BSP_LED_Off(LED4);
    osMessagePut(AppliEvent, DISCONNECTION_EVENT, 0);
    break;
    
  case HOST_USER_CLASS_ACTIVE:
    osMessagePut(AppliEvent, CONNECTION_EVENT, 0);
    break;
    
  default:
    break; 
  }
}



#include "settings.h"


static void DISK_thread(void const * arg)
{
  
  (void) arg;

  osEvent event;
  
  //FRESULT res;                                          /* FatFs function common result code */
  //uint16_t byteswritten;                                /* File write count */
  //uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
  
  for( ;; )
  {
    event = osMessageGet(DiskEvent, osWaitForever);
    
    if(event.status == osEventMessage)
    {
      switch(event.value.v)
      {
      case DISK_READY_EVENT:
        writef("USBDisk mounted.\r\n");
        
        osDelay(500);
        
        ini_create_file();
        ini_parse_file("SETTINGS.water");
        
        break;
        
      case DISK_REMOVE_EVENT:
        /* Unlink the USB disk I/O driver */
        FATFS_UnLinkDriver(USBDISKPath);
        break;
        
      default:
        break; 
      }
    }
  }
}


























static void NET_start (void const * arg)
{

  (void) arg;

  static ip_addr_t ipaddr;
  static ip_addr_t netmask;
  static ip_addr_t gw;

  static ip_addr_t dns;

#ifdef USE_DHCP
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;
#else
  /* IP address default setting */
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

  //lwip_init(); /* TODO: Check this */ 
  tcpip_init(NULL,NULL);

  /* Add the network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called */
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }

  /* Start DNS service - set to ip of gateway (most routers do offer dns) */
  dns_init();
  IP4_ADDR(&dns, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
  dns_setserver(0, &dns);

#if 0
  /* Start NTP service */
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "de.pool.ntp.org");
  sntp_init();
#endif

  /* Test httplog */
  //int numberone = 123;
  //httplog("led1=%d", numberone);

  /* Start HTTP service */
  osThreadDef(http, HTTP_start,    osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate( osThread(http),  NULL);

  /* Start MQTT service */
  osThreadDef(mqtt, MQTT_start,    osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 4);
  osThreadCreate( osThread(mqtt),  NULL);

  while (1) {
    osThreadTerminate( NULL );  /* important to stop task here !! */
  }

}


static void HTTP_start (void const * arg)
{

  (void) arg;

  /* Starts the httpd server */
  http_server_init();
  //http_server_socket_init();

  while(1) {

  }

}







void messageArrived(MessageData* md)
{
  MQTTMessage* message = md->message;

  writef("  Topic: %.*s\r\n", md->topicName->lenstring.len, md->topicName->lenstring.data);
  writef("Payload: %.*s\r\n", (int)message->payloadlen, (char*)message->payload);


  if (strncmp((char *) message->payload, "on", (int) message->payloadlen) == 0) {
    writef("ON\r\n");
    gwinSetText(ghScaleADCvalue, "ON", 1);

  } else if (strncmp((char *) message->payload, "off", (int) message->payloadlen) == 0) {
    writef("OFF\r\n");
    gwinSetText(ghScaleADCvalue, "OFF", 1);
  }
}






static void MQTT_start (void const * arg)
{

  (void) arg;


  int rc = 0;
  Network n;
  Client c;
  unsigned char sendbuf[100];   /* Sending   buffer */
  unsigned char readbuf[100];   /* Receiving buffer */

  ip4_addr_t broker_ipaddr;
  //IP4_ADDR( &broker_ipaddr, 192, 168, 1, 101);
  IP4_ADDR( &broker_ipaddr, 37, 61, 201, 83);     // cumulus.intewa.net:15672


  //err_t err;

  mqtt_lwip_init(&n);


  rc = mqtt_lwip_connect(&n, &broker_ipaddr, 1883);

  if (rc != 0) {
    writef("Failed to connect to MQTT server\r\n");

    goto blink_to_death;
  }

  writef("Initializing MQTT service\r\n");

  MQTTClient(&c, &n, 1000, sendbuf, 100, readbuf, 100);

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  
  char clientID_buf[256] = {0};

  sprintf(clientID_buf, "AL-MS-CU_%02lx%02lx%02lx%02lx%02lx%02lx", 
    (uint32_t) MAC_ADDR0, 
    (uint32_t) MAC_ADDR1, 
    (uint32_t) MAC_ADDR2, 
    (uint32_t) MAC_ADDR3, 
    (uint32_t) MAC_ADDR4, 
    (uint32_t) MAC_ADDR5);  

  data.clientID.cstring = clientID_buf;
  data.username.cstring = "device";
  data.password.cstring = "device";
  
  data.MQTTVersion = 3;
  data.willFlag = 0;
  data.keepAliveInterval = 5;
  data.cleansession = 1;
  
  rc = MQTTConnect(&c, &data);
  
  if (rc == MQTT_FAILURE) {
    writef("Failed to send connect request\r\n");
    goto blink_to_death;
  }

  writef("Subscribing to its topic\r\n");


  char topic_buf[128] = {0};

  sprintf(topic_buf, "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx/do/#",
    (uint32_t) MAC_ADDR0, 
    (uint32_t) MAC_ADDR1, 
    (uint32_t) MAC_ADDR2, 
    (uint32_t) MAC_ADDR3, 
    (uint32_t) MAC_ADDR4, 
    (uint32_t) MAC_ADDR5); 

  rc = MQTTSubscribe(&c, topic_buf, QOS0, messageArrived);

  //rc = MQTTSubscribe(&c, "hello/world", QOS0, messageArrived);
  
  if (rc == MQTT_FAILURE) {
    writef("Failed to subscribe to its topic\r\n");
    MQTTDisconnect(&c);
    goto blink_to_death;
  }

  writef("Connected to MQTT server...\r\n");



  while(1) {

    MQTTMessage message;
    
    unsigned char sendtemp[] = {'0', '0', '.', '0', '0', 0 };

    message.qos = QOS0;
    message.retained = 0;
    message.payload = sendtemp;

#if 1
    memset(sendtemp, 0, sizeof(sendtemp));
    sprintf((char*)sendtemp, "%2.2f", ds18b20[0].Temperature);

    //message.payloadlen = strlen((char*)sendbuf);
    message.payloadlen = 5;

    char topic_buf_2[128] = {0};

    memset(topic_buf_2, 0, sizeof(topic_buf_2));
    sprintf(topic_buf_2, "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx/basement/temp",
      (uint32_t) MAC_ADDR0, 
      (uint32_t) MAC_ADDR1, 
      (uint32_t) MAC_ADDR2, 
      (uint32_t) MAC_ADDR3, 
      (uint32_t) MAC_ADDR4, 
      (uint32_t) MAC_ADDR5); 

    MQTTPublish(&c, topic_buf_2, &message);

    MQTTYield(&c, 1000);

    osDelay(500);
#endif

#if 1

    MQTTMessage online_status;

    unsigned char sendstr[] = {'o', 'n', 'l', 'i', 'n', 'e'};

    online_status.qos = QOS0;
    online_status.retained = 0;
    online_status.payload = sendstr;

    online_status.payloadlen = 6;

    char topic_online_buf[100] = {0};
    

    memset(topic_online_buf, 0, sizeof(topic_online_buf));
    sprintf(topic_online_buf, "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx/online_status",
      (uint32_t) MAC_ADDR0, 
      (uint32_t) MAC_ADDR1, 
      (uint32_t) MAC_ADDR2, 
      (uint32_t) MAC_ADDR3, 
      (uint32_t) MAC_ADDR4, 
      (uint32_t) MAC_ADDR5); 

    MQTTPublish(&c, topic_online_buf, &online_status);

    MQTTYield(&c, 1000);

    osDelay(500);
#endif

  }

  //return RDY_OK;

  blink_to_death:
  
  while(1){

  }

  //return RDY_OK;

}



















#if 1

static void GUI_start (void const * arg)
{

  (void) arg;

  /* Start µGFX */
  gfxInit();

  gdispSetBacklight(100);
  gdispSetContrast(100);

  guiCreate();

#if 1
  osThreadDef(gui, GUI_thread,  osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate( osThread(gui), NULL);

  osDelay(100);

  //osThreadDef(tty0, TTY_thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 1);
  //osThreadCreate( osThread(tty0), NULL);
#endif

  while (1) {
    //osDelay(100);
    osThreadTerminate( NULL );  /* important to stop task here !! */
  }

}

static void GUI_thread (void const * arg)
{

  (void) arg;

  while (1) {
    guiEventLoop();

    if (curWindow == &winMainHome) {
      // Draw LCD on page HOME
      gui_create_lcd((uint16_t)gwinSliderGetPosition(ghSliderADCvalue));
    }

    osDelay(100);

  }
}


#endif

















#if 1

#define BUFFER_LEN 32

#include <string.h>
#include <ctype.h>

/* Example includes. */
#include "FreeRTOS_CLI.h"

/* The CLI commands are defined in CLI-commands.c. */
void vRegisterSampleCLICommands( void );

static char input[BUFFER_LEN];
static char input_b[BUFFER_LEN];

static void TTY_thread (void const * arg)
{

  (void) arg;


  static unsigned int saved = 0;
  
  static uint8_t index = 0;
  char rx_char;

  static uint32_t printPrompt = 1;

  char *pcOutputString;
  portBASE_TYPE xReturned;

  pcOutputString = FreeRTOS_CLIGetOutputBuffer();

  /* Register two command line commands to show task stats and run time stats respectively. */
  vRegisterSampleCLICommands();

  while (1) {

    // Enable interrupt RX Not Empty Interrupt
    //__HAL_UART_ENABLE_IT(&hUART, UART_IT_RXNE);

    /* Do we have to print the prompt? */
    if(printPrompt) {
      //writef("> "); /* printf("\n"); */
      //vt100_puts("> ");
      
      printPrompt = 0;
      memset(input, 0, sizeof(input));
    }

    int const queue_rc = xQueueReceive(rx_char_queue, &rx_char, portMAX_DELAY);
    if (queue_rc == pdFALSE) {
        index = 0;
        continue; // Try again
    }

    // Process

    /* Check for simple line control characters */
    if(((rx_char == 010) || (rx_char == 0x7f)) && index) {
      /* User pressed backspace */
      writef("\010 \010");        /* Obliterate character */
      //vt100_putc('\b');
      index--;                    /* Then keep track of how many are left */
      input[index] = '\0';        /* Then remove it from the buffer */

    } else if(rx_char == '!') {   /* '!' repeats the last command */
      if(saved) {                 /* But only if we have something saved */
        strcpy(input, input_b);   /* Restore the command */
        //writef("HIS: %s",input);
        index = strlen(input);
        goto parseme;
      }
    } else if(isprint((unsigned int) rx_char)) {
      /* We are only going to save printable characters */
      if(index >= sizeof(input)) {

        /* We are out of space */
        writef("\x07");   /* Beep */
        continue;         /* TODO: Check this!*/

      } else {
        /* Store char in buffer */
        input[index] = rx_char;
        index++;
        /* Echo it back to the user */
        writef("%c", rx_char);
        //vt100_putc(rx_char);
      }
    } else if(rx_char == 0x1b) { /* ESC Key - forward to terminal only */
        //vt100_putc(rx_char);

    } else if(rx_char == '\r') {
      /* NULL Terminate anything we have received */
      input[index] = '\0';
      /* save current buffer in case we want to re-do the command */
      strcpy(input_b, input);
      saved = 1;

      /* The user pressed enter, parse the command */
    
    parseme:
      /* Send CR to console */
      writef("\r\n");
      //vt100_putc('\r'); 
      index = 0;
      
      #if 1
      do {
        xReturned = FreeRTOS_CLIProcessCommand( input, pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE );
        writef("%s", pcOutputString);
        //vt100_puts(pcOutputString);

      } while( xReturned != pdFALSE );

      //vt100_putc('\r');
      #endif

      printPrompt = 1;
    }

    /* We have a character to process */
    //writef("got:'%c' %d %d \r\n", c, c, count);

  }

}



#endif









#if 1

static void ADC_thread (void const * arg)
{

  (void) arg;

  char adc_string[16];
  char adc_temp[16];


#if 0
  /*##-3- Start the conversion process and enable interrupt ##################*/  
  if(HAL_ADC_Start_IT(&AdcHandle) != HAL_OK)
  {
    /* Start Conversation Error */
    Error_Handler();
  }
#endif

  while (1) {

#if 0
    // Polling mode

    /*##-3- Start the conversion process #######################################*/  
    if(HAL_ADC_Start(&AdcHandle) != HAL_OK)
    {
      /* Start Conversation Error */
      Error_Handler();
    }

    /*##-4- Wait for the end of conversion #####################################*/  
     /*  Before starting a new conversion, you need to check the current state of 
          the peripheral; if its busy you need to wait for the end of current
          conversion before starting a new one.
          For simplicity reasons, this example is just waiting till the end of the 
          conversion, but application may perform other tasks while conversion 
          operation is ongoing. */
    HAL_ADC_PollForConversion(&AdcHandle, 10);

    /* Check if the continuous conversion of regular channel is finished */
    if((HAL_ADC_GetState(&AdcHandle) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
    {
      /*##-5- Get the converted value of regular channel #######################*/
      uhADCxConvertedValue = HAL_ADC_GetValue(&AdcHandle);

      float volt;

      volt = (((float)uhADCxConvertedValue * 3300) / 4096) / 1000;

      sprintf(adc_string, "%.3f", volt);

      if(uhADCxConvertedValue < 2048) {
        gwinHide(ghLabelADCindicOK);
        gwinShow(ghLabelADCindicFAIL);
      } else {
        gwinShow(ghLabelADCindicOK);
        gwinHide(ghLabelADCindicFAIL);
      }

      gwinSetText(ghLabelADCvalue, adc_string, 1);
      //gwinRedraw(ghLabelADCval);

      gwinSetDefaultFont(fixed_7x14);
      gwinSetDefaultColor(White);
      gwinFillString(ghLabelADCindicOK, 2, 1, "OK");
      gwinFillString(ghLabelADCindicFAIL, 2, 1, "ERR");

    }

#endif

#if 0
    /*##-3- Start the conversion process and enable interrupt ##################*/  
    if(HAL_ADC_Start_IT(&AdcHandle) != HAL_OK)
    {
      /* Start Conversation Error */
      Error_Handler();
    }
#endif


#if 0
  /*##-3- Start the conversion process #######################################*/  
  if(HAL_ADC_Start(&AdcTempHandle) != HAL_OK)
  {
    /* Start Conversation Error */
    Error_Handler();
  }
  
  /*##-4- Wait for the end of conversion #####################################*/  
   /*  Before starting a new conversion, you need to check the current state of 
        the peripheral; if its busy you need to wait for the end of current
        conversion before starting a new one.
        For simplicity reasons, this example is just waiting till the end of the 
        conversion, but application may perform other tasks while conversion 
        operation is ongoing. */
  HAL_ADC_PollForConversion(&AdcTempHandle, 10);
  
  /* Check if the continuous conversion of regular channel is finished */
  if((HAL_ADC_GetState(&AdcTempHandle) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
  {

    HAL_ADC_Stop(&AdcTempHandle);

    /*##-5- Get the converted value of regular channel #######################*/
    adc_temp_value = (float)HAL_ADC_GetValue(&AdcTempHandle);
  }

  temperature = 25.0f+(3.3f*(float)adc_temp_value/(float)4096-0.76f)/0.0025f;

  sprintf(adc_temp, "%.3f%cC", temperature, 176);

  gwinSetText(ghScaleADCvalue, adc_temp, 1);
#endif



#if 0
    //float volt;
    //volt = (((float)uhADCxConvertedValue * 3300) / 4096) / 1000;
    //sprintf(adc_string, "%.3f", volt);
    //sprintf(adc_string, "%d", gwinSliderGetPosition(ghSliderADCvalue));

/*
    if(uhADCxConvertedValue < 2048) {
      gwinHide(ghLabelADCindicOK);
      gwinShow(ghLabelADCindicFAIL);
    } else {
      gwinShow(ghLabelADCindicOK);
      gwinHide(ghLabelADCindicFAIL);
    }
*/
    //gwinSetText(ghLabelADCvalue, adc_string, 1);
    //gwinRedraw(ghLabelADCval);

    gwinSetDefaultFont(fixed_7x14);
    gwinSetDefaultColor(White);
    gwinFillString(ghLabelADCindicOK, 2, 1, "OK");
    gwinFillString(ghLabelADCindicFAIL, 2, 1, "ERR");
#endif

    osDelay(250);
  }

}



/**
  * @brief  Conversion complete callback in non blocking mode 
  * @param  AdcHandle : AdcHandle handle
  * @note   This example shows a simple way to report end of conversion, and 
  *         you can add your own implementation.    
  * @retval None
  */
#if 0
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{

  /* Get the converted value of regular channel */
  uhADCxConvertedValue = HAL_ADC_GetValue(AdcHandle);

  /* Stop the ISR, otherwise a loop would permanently call this !!! */
  HAL_ADC_Stop_IT(AdcHandle);

#if 0
  // Does not work here...     
  char adc_string[16];

  float volt;

  volt = (((float)uhADCxConvertedValue * 3300) / 4096) / 1000;

  sprintf(adc_string, "%.3f", volt);

  if(uhADCxConvertedValue < 2048) {
    gwinHide(ghLabelADCindicOK);
    gwinShow(ghLabelADCindicFAIL);
  } else {
    gwinShow(ghLabelADCindicOK);
    gwinHide(ghLabelADCindicFAIL);
  }

  gwinSetText(ghLabelADCvalue, adc_string, 1);
  //gwinRedraw(ghLabelADCval);

  gwinSetDefaultFont(fixed_7x14);
  gwinSetDefaultColor(White);
  gwinFillString(ghLabelADCindicOK, 2, 1, "OK");
  gwinFillString(ghLabelADCindicFAIL, 2, 1, "ERR");
#endif


}
#endif

#endif
















#if 1


#include <time.h>
#include <sys/time.h>

#include <locale.h>

#define TIMEBUF   64

static void RTC_thread (void const * arg)
{

  (void) arg;

  struct tm tm_set;

  /* Set time to 1.1.2017 00:00 */

  tm_set.tm_sec  = 1;         /* seconds,  range 0 to 59          */
  tm_set.tm_min  = 0;         /* minutes, range 0 to 59           */
  tm_set.tm_hour = 0;         /* hours, range 0 to 23             */

  tm_set.tm_mon  = 0;             /* month, range 0 to 11             */
  tm_set.tm_year = 2017 - 1900;   /* The number of years since 1900   */
  tm_set.tm_mday = 1;             /* day of the month, range 1 to 31  */
  tm_set.tm_isdst = -1;           /* daylight saving time             */

#if 0
  /*
  The mktime function ignores the specified contents of the tm_wday and tm_yday members of the broken-down
  time structure. It uses the values of the other components to compute the calendar time; it's permissible
  for these components to have unnormalized values outside of their normal ranges. The last thing that mktime 
  does is adjust the components of the brokentime structure (including the tm_wday and tm_yday).
  */ 
  tm_set.tm_yday;        /* day in the year, range 0 to 365  */
  tm_set.tm_wday;        /* day of the week, range 0 to 6    */

  //debug("Size of time_t: %d\n", sizeof(time_t)); /* Returns: 4 = 32bit => we have a year 2038 Problem... */
#endif

  /* Create time from settings provided in tm_set */
  time_t set_timer = mktime(&tm_set) ;

  //debug("%lu \n", set_timer );  // Print timestamp

  /* Set system time to milliseconds, reduce by one second */
  clock_timestamp = (uint64_t) set_timer * 1000U;

  //setlocale(LC_TIME, "de_DE.UTF-8");

  while (1) {

    static char buffer[TIMEBUF];
    
    static time_t timer;
    static struct tm* tm_info;

    //static char buffer_del[TIMEBUF];
    //static struct tm tm_del;

/*
       1  2  3  4  5  6  7
rtc    mo di mi do fr sa so

    0  1  2  3  4  5  6
lib so mo di mi do fr sa
*/

    /* Read time info from clock (UTC) via _gettimeofday() */
    time(&timer);

    /* Break up time info as local time */
    tm_info = localtime(&timer);
    
    //strftime (buffer, TIMEBUF, "%a, %d. %B %Y %H:%M:%S", tm_info);
    strftime (buffer, TIMEBUF, "%d.%m.%Y %H:%M:%S", tm_info);

    gwinSetText(ghLabelClockTime, buffer, 1);


#if 0
    //taskENTER_CRITICAL();

    strftime (buffer, TIMEBUF, "%a, %d. %B %Y\n", tm_info); /* So, 31. Januar 1970 */
    gdispFillStringBox( 5, 60, 200, 20, buffer, dejavu_sans_10, White, Black, justifyLeft);
    memset(buffer, 0, TIMEBUF);
    //gfxSleepMilliseconds(50);

    strftime (buffer, TIMEBUF, "%H:%M:%S", tm_info);
    gdispFillStringBox( 5, 80, 200, 20, buffer, dejavu_sans_10, White, Black, justifyLeft);
    memset(buffer, 0, TIMEBUF);
    //gfxSleepMilliseconds(50);
#endif


#if 0
    // compare/store old value
    if (tm_del.tm_sec != tm_info->tm_sec) {

      do {
        /* Delete old value from screen */
        sprintf(buffer_del, "%s\n", asctime(&tm_del));
        gdispDrawString( 5, 40, buffer_del, dejavu_sans_16, Black);
        
        /* Draw new value */
        sprintf(buffer, "%s\n", asctime(tm_info));
        gdispDrawString( 5, 40, buffer, dejavu_sans_16, White);
        
        /* store new value as old one.. */
        tm_del = *tm_info;

      } while (0);

    }

    sprintf(buffer, "DST: %d DOW: %d DOY: %d \n", tm_info->tm_isdst, tm_info->tm_wday, tm_info->tm_yday);
    gdispFillStringBox( 5, 100, 200, 20, buffer, dejavu_sans_10, White, Black, justifyLeft);
    memset(buffer, 0, TIMEBUF);
    //gfxSleepMilliseconds(50);

#endif
    //taskEXIT_CRITICAL();

    /*
    vt100_puts("\e[H");
    vt100_puts(aShowTime); vt100_putc('\r');
    vt100_puts(aShowDate); vt100_putc('\r');
    */

    osDelay(1000);
  }

}



#endif











static void LED1_thread (void const * arg)
{

  (void) arg;

  while(1) {
    BSP_LED_On(LED1);
    osDelay(LED1_UPDATE_DELAY);
    BSP_LED_Off(LED1);
    osDelay(LED1_UPDATE_DELAY);
  }
}


static void LED2_thread (void const * arg)
{

  (void) arg;

  while(1) {
    BSP_LED_On(LED2);
    osDelay(LED2_UPDATE_DELAY);
    BSP_LED_Off(LED2);
    osDelay(LED2_UPDATE_DELAY);
  }
}





















static void Register_printout(void)
{

    /* Put some information on the starting screen */
  writef("\033[2J"); // Clear screen
  writef("\r\n");

  /* Read some Hardware Registers for information and fun ;) */
  char rev_id[42];
  sprintf((char*)rev_id,  "XCore: 0x%04lx, Rev. 0x%04lx", (DBGMCU->IDCODE & 0x00000FFF), ((DBGMCU->IDCODE >> 16) & 0x0000FFFF));
  writef("%s", rev_id);
  writef("\r\n");

  /*
     * \par Revisions possible:
     *  - 0x1000: Revision A
     *  - 0x1001: Revision Z
     *  - 0x1003: Revision Y
     *  - 0x1007: Revision 1
     *  - 0x2001: Revision 3
     *
     * \par Device signatures:
     *  - 0x0413: STM32F405xx/07xx and STM32F415xx/17xx)
     *  - 0x0419: STM32F42xxx and STM32F43xxx
     *  - 0x0423: STM32F401xB/C
     *  - 0x0433: STM32F401xD/E
     *  - 0x0431: STM32F411xC/E
     *  - 0x0421: STM32F446xx
     *  - 0x0449: STM32F7x6xx
     *  - 0x0444: STM32F03xxx
     *  - 0x0445: STM32F04xxx
     *  - 0x0440: STM32F05xxx
     *  - 0x0448: STM32F07xxx
     *  - 0x0442: STM32F09xxx
  */

  /* Read UUID */
  uint32_t idPart1 = STM32_UUID[0];
  uint32_t idPart2 = STM32_UUID[1];
  uint32_t idPart3 = STM32_UUID[2];
  char  uuid0_tmp[32];
  sprintf((char*)uuid0_tmp,  "UUID:  %08lx-%08lx-%08lx", idPart1, idPart2, idPart3);
  writef("%s", uuid0_tmp);
  writef("\r\n");

  /* Print flash size */
  uint32_t flashSize = STM32_UUID_FLASH[0];
  char flash_tmp[32];
  sprintf((char*)flash_tmp, "%lx", flashSize);

  char flash[6];
  sprintf ((char*)flash, &(flash_tmp[strlen(flash_tmp) - 4]));

  uint16_t size = strtol((char*)flash, NULL, 16);
  sprintf((char*)flash_tmp, "Flash: %d kB", size);
  writef("%s", flash_tmp);
  writef("\r\n");

  /* Print package code (hardware chip case form) */
  uint32_t flashPack = (((*(__IO uint16_t *) (STM32_UUID_PACK)) & 0x0700) >> 8);

  /*
   *  - 0b01xx: LQFP208 and TFBGA216 package
   *  - 0b0011: LQFP176 and UFBGA176 package
   *  - 0b0010: WLCSP143 and LQFP144 package
   *  - 0b0001: LQFP100 package
   */

  uint8_t pack_tmp[10];
  sprintf((char*)pack_tmp,  "Pack:  %lx", flashPack);
  writef("%s", pack_tmp);
  writef("\r\n");

  /* System clock runnning at ... */
  writef("CLK:   %d MHz", SystemCoreClock / 1000000UL);
  writef("\r\n");

  /* We run our code as ... */
  writef("Firmware %s", VERSION_STRING_LONG);
  writef("\r\n");

  /* Welcome to our guests ;) */
  writef("CMSIS - FreeRTOS - LwIP - BSP - uGFX");
  writef("\r\n");
  writef("\r\n");

  /* NAND flash drive */
  flashdrive_init();

  static NAND_IDTypeDef flash_id;

  HAL_NAND_Read_ID(&hNAND, &flash_id);

  writef("NAND Flash ID = 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", flash_id.Maker_Id, flash_id.Device_Id,
                                                               flash_id.Third_Id, flash_id.Fourth_Id );

  if ((flash_id.Maker_Id == 0xEC) && (flash_id.Device_Id == 0xF1)
    && (flash_id.Third_Id == 0x80) && (flash_id.Fourth_Id == 0x15))
  {
   writef("Type = K9F1G08U0A\r\n");
  }
  else if ((flash_id.Maker_Id == 0xEC) && (flash_id.Device_Id == 0xF1)
    && (flash_id.Third_Id == 0x00) && (flash_id.Fourth_Id == 0x95))
  {
   writef("Type = K9F1G08U0B\r\n");   
  }
  else if ((flash_id.Maker_Id == 0xAD) && (flash_id.Device_Id == 0xF1)
    && (flash_id.Third_Id == 0x80) && (flash_id.Fourth_Id == 0x1D))
  {
   writef("Type = HY27UF081G2A\r\n");   
  }
  else
  {
   writef("Type = Unknow\r\n");
  }
  writef("\r\n");

}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

