#include "stm32f10x.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>

#include "usart.h"
#include "w25q64.h"
#include "ota_info.h"
#include "ota_protocol.h"
#include "ota_uart.h"
#include "app_jump.h"
#include "Delay.h"
#include "esp8266_http.h"

#define WIFI_SSID       "abc"
#define WIFI_PASSWORD   "a9y8zyd5m5qcmzr"
#define SERVER_IP       "10.48.190.232"
#define APP_START_ADDR  0x08005000

OTA_Info_t ota_info;

uint32_t server_version;
uint8_t ret;
uint32_t Local_Version=1;

uint8_t APP_Check(void);

int main(void)
{
    SystemInit();
    USART1_Init(115200);
	USART2_Init(115200);
	W25Q64_Init();
	
    USART2_SendString("BOOT READY\r\n");
    if(ESP8266_WiFi_Init(WIFI_SSID,WIFI_PASSWORD)==0)
	{
		USART2_SendString("WIFI INIT FAIL\r\n");
		Jump_To_App(APP_START_ADDR);
	}
	
    if(ESP8266_HTTP_Get_Version(SERVER_IP,&Server_Version))
    {
        if(Server_Version > Local_Version)
        {
            ESP8266_HTTP_Download_APP(SERVER_IP);
        }
    }
    Jump_To_App(APP_START_ADDR);
}

uint8_t APP_Check(void)
{
    uint32_t sp;
    uint32_t reset;

    sp = *(__IO uint32_t *)APP_START_ADDR;
    reset = *(__IO uint32_t *)(APP_START_ADDR+4);

    if((sp>=0x20000000) && (sp<=0x20005000) && (reset>=APP_START_ADDR) && (reset<0x08020000))
    {
        return 1;
    }
    return 0;
}
