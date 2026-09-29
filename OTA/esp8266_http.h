#ifndef __ESP8266_HTTP_H
#define __ESP8266_HTTP_H

#include "stm32f10x.h"

extern uint32_t Server_Version;

/*
 * ESP8266初始化+连接WiFi
 */
uint8_t ESP8266_WiFi_Init(const char *ssid,const char *password);

/*
 * 版本检测
 */
uint8_t ESP8266_HTTP_Get_Version(const char *server_ip,uint32_t *version);

/*
 * 下载APP
 */
uint8_t ESP8266_HTTP_Download_APP(const char *server_ip);

#endif
