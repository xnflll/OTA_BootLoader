#include "esp8266_http.h"
#include "usart.h"
#include "delay.h"
#include "ota_protocol.h"
#include "ota_update.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    HTTP_MODE_APP = 0,
    HTTP_MODE_VERSION
}HTTP_Mode_t;

/*=========================================================
    参数
=========================================================*/
#define HTTP_HEADER_SIZE      512
#define OTA_DATA_SIZE         128
#define HTTP_TIMEOUT_COUNT    6000

/*=========================================================
    HTTP模式
=========================================================*/
static HTTP_Mode_t Http_Mode;

/*=========================================================
    HTTP缓存
=========================================================*/
static char Http_Header[HTTP_HEADER_SIZE];
static uint16_t Http_Header_Len;
static uint8_t Http_Header_Done;

/*
    APP大小
*/
static uint32_t Http_Content_Length;

/*
    已接收Body长度
*/
static uint32_t Http_Body_Received;

/*
    OTA缓存
*/
static uint8_t OTA_Data_Buffer[OTA_DATA_SIZE];
static uint16_t OTA_Data_Len;

/*
    OTA序号
*/
static uint16_t OTA_Seq;

/*
    CRC32
*/
static uint32_t Http_CRC32;

/*
    version
*/
uint32_t Server_Version;

/*=========================================================
    IPD解析
=========================================================*/
typedef enum
{
    IPD_SEARCH = 0,
    IPD_READ_LEN,
    IPD_PAYLOAD
}IPD_State_t;

static IPD_State_t IPD_State;
static uint32_t IPD_Length;
static uint32_t IPD_Remain;
static uint8_t IPD_Match;

static const char IPD_Prefix[]="+IPD,";

/*=========================================================
    等待ESP字符串
=========================================================*/
uint8_t ESP_WaitString(char *target,uint32_t timeout)
{
    char rxbuf[128];
    uint16_t rxlen=0;
    uint8_t data;

    memset(rxbuf,0,sizeof(rxbuf));

    while(timeout--)
    {
        while(USART1_RxAvailable())
        {
            data=USART1_ReadByte();

            /*
                调试输出
            */
            USART2_SendChar(data);

            if(rxlen < sizeof(rxbuf)-1)
            {
                rxbuf[rxlen++]=data;
                rxbuf[rxlen]=0;
            }
            else
            {
                memmove(rxbuf,rxbuf+32,sizeof(rxbuf)-33);
                rxlen=sizeof(rxbuf)-33;
                rxbuf[rxlen++]=data;
                rxbuf[rxlen]=0;
            }

            if(strstr(rxbuf,target))
            {
                return 1;
            }

            if(strstr(rxbuf,"ERROR"))
            {
                return 0;
            }

            if(strstr(rxbuf,"FAIL"))
            {
                return 0;
            }
        }
        Delay_ms(10);
    }

    return 0;
}

/*=========================================================
    WiFi初始化
=========================================================*/
uint8_t ESP8266_WiFi_Init(const char *ssid,const char *password)
{
    char command[128];

    USART2_SendString("ESP WIFI INIT\r\n");
    USART1_SendString("AT\r\n");
    if(ESP_WaitString("OK",1000)==0)
    {
        return 0;
    }

    USART1_SendString("AT+CWMODE=1\r\n");
    if(ESP_WaitString("OK",1000)==0)
    {
        return 0;
    }
    sprintf(command,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,password);
    USART2_SendString("CONNECT WIFI\r\n");
    USART1_SendString(command);

    /*
       这里不要等 OK
       要等 WIFI GOT IP
    */
    if(ESP_WaitString("WIFI GOT IP",15000)==0)
    {
        USART2_SendString("WIFI ERROR\r\n");
        return 0;
    }
    USART2_SendString("WIFI OK\r\n");
    Delay_ms(1000);
	
    return 1;
}

/*=========================================================
    HTTP Header结束判断
=========================================================*/
static uint8_t HTTP_Header_End(void)
{
    if(Http_Header_Len < 4)
        return 0;

    if(Http_Header[Http_Header_Len-4]=='\r' &&
       Http_Header[Http_Header_Len-3]=='\n' &&
       Http_Header[Http_Header_Len-2]=='\r' &&
       Http_Header[Http_Header_Len-1]=='\n')
    {
        return 1;
    }

    return 0;
}

/*=========================================================
    获取Content-Length
=========================================================*/
static uint8_t HTTP_Parse_Content_Length(void)
{
    char *p;
    p=strstr(Http_Header,"Content-Length:");

    if(p==0)
        return 0;

    p+=strlen("Content-Length:");

    while(*p==' ')
        p++;

    Http_Content_Length=strtoul(p,0,10);

    return 1;
}

/*=========================================================
    TCP连接服务器
=========================================================*/
uint8_t ESP8266_TCP_Connect(const char *server_ip)
{
    char command[100];

    /*
     * 清理ESP8266上一次连接残留
     */
    USART1_ClearRxBuffer();
    Delay_ms(500);
    sprintf(command,"AT+CIPSTART=\"TCP\",\"%s\",80\r\n",server_ip);
    USART2_SendString("TCP CONNECT\r\n");
    USART1_SendString(command);

    if(ESP_WaitString("CONNECT",3000)==0)
    {
        USART2_SendString("TCP ERROR\r\n");
        return 0;
    }
    Delay_ms(500);

    return 1;
}

/*=========================================================
    HTTP GET
=========================================================*/
uint8_t ESP8266_HTTP_GET(const char *server_ip,const char *file)
{

    char request[200];
    char command[64];

    uint16_t len;

    sprintf(request,"GET %s HTTP/1.1\r\n""Host: %s\r\n""Connection: close\r\n""\r\n",file,server_ip);
    len=strlen(request);sprintf(command,"AT+CIPSEND=%d\r\n",len);
    USART1_SendString(command);

    if(ESP_WaitString(">",1000)==0)
    {
        return 0;
    }

    USART2_SendString("HTTP GET SEND\r\n");
    USART1_SendString(request);

    return 1;
}

/*=========================================================
    CRC32初始化
=========================================================*/
static void Http_CRC32_Update(uint8_t data)
{
    Http_CRC32 ^= data;

    for(uint8_t i=0;i<8;i++)
    {

        if(Http_CRC32&1)
			Http_CRC32=(Http_CRC32>>1)^0xEDB88320;

        else
            Http_CRC32>>=1;
    }
}

/*=========================================================
    OTA DATA发送
=========================================================*/
static void OTA_Send_Data_Buffer(void)
{
    OTA_DataPacket_t packet;

    if(OTA_Data_Len==0)
        return;

    packet.seq=OTA_Seq;
    packet.len=OTA_Data_Len;

    memcpy(packet.data,OTA_Data_Buffer,OTA_Data_Len);
    packet.crc=OTA_CRC16(packet.data,packet.len);

    OTA_Data_Process(&packet);
    OTA_Seq++;
    OTA_Data_Len=0;
}

/*=========================================================
    APP数据处理
=========================================================*/
static void HTTP_Process_APP_Data(uint8_t data)
{
    OTA_Data_Buffer[OTA_Data_Len++]=data;
    Http_CRC32_Update(data);
    Http_Body_Received++;

    if(OTA_Data_Len>=OTA_DATA_SIZE)
    {
        OTA_Send_Data_Buffer();
    }
}

/*=========================================================
    IPD Payload处理
=========================================================*/
static void HTTP_Process_IPD_Byte(uint8_t data)
{
	/*
		HTTP头
	*/
	if(Http_Header_Done==0)
	{
		Http_Header[Http_Header_Len++]=data;
		Http_Header[Http_Header_Len]=0;

		if(HTTP_Header_End())
		{
			Http_Header_Done=1;

			/*
				VERSION模式
			*/
			if(Http_Mode==HTTP_MODE_VERSION)
			{
				HTTP_Parse_Content_Length();
				Http_Body_Received=0;
				Server_Version=0;
				return;
			}

			/*
				APP模式
			*/
			if(HTTP_Parse_Content_Length()==0)
			{
				USART2_SendString("LENGTH ERROR\r\n");
				return;
			}

			USART2_SendString("HTTP 200 OK\r\n");
			USART2_SendString("APP SIZE=");
			USART2_SendHex((Http_Content_Length>>24)&0xff);
			USART2_SendHex((Http_Content_Length>>16)&0xff);
			USART2_SendHex((Http_Content_Length>>8)&0xff);
			USART2_SendHex(Http_Content_Length&0xff);
			USART2_SendString("\r\n");

			OTA_StartPacket_t start;

			start.file_size=Http_Content_Length;
			start.crc32=0;
			memset(start.version,0,16);

			strcpy((char*)start.version,"V1.0");
			OTA_Start_Process(&start);
			Http_CRC32=0xffffffff;
			Http_Body_Received=0;
			OTA_Data_Len=0;
			OTA_Seq=1;
		}
		return;
	}

	/*
		Body
	*/
	if(Http_Body_Received<Http_Content_Length)
	{
		if(Http_Mode==HTTP_MODE_APP)
		{
			HTTP_Process_APP_Data(data);
		}

		else
		{
			if(data>='0' && data<='9')
			{
				Server_Version=Server_Version*10+(data-'0');

				USART2_SendString("VERSION BYTE=");
				USART2_SendChar(data);
				USART2_SendString("\r\n");
			}
			Http_Body_Received++;
		}
	}
}

/*=========================================================
    解析ESP接收数据
=========================================================*/
void ESP8266_Process_Rx(void)
{
    uint8_t data;

    while(USART1_RxAvailable())
    {
        data=USART1_ReadByte();

        /*
            查找 +IPD,
        */
        if(IPD_State==IPD_SEARCH)
        {
            if(data==IPD_Prefix[IPD_Match])
            {
                IPD_Match++;
				
                if(IPD_Match>=5)
                {
                    IPD_Match=0;
                    IPD_Length=0;
                    IPD_State=IPD_READ_LEN;
                }
            }

            else
            {
                if(data=='+')
                    IPD_Match=1;

                else
                    IPD_Match=0;
            }
            continue;
        }

        /*
            读取长度
        */
        if(IPD_State==IPD_READ_LEN)
        {
            if(data>='0' && data<='9')
            {
                IPD_Length=IPD_Length*10+data-'0';
                continue;
            }

            if(data==':')
            {
                IPD_Remain=IPD_Length;
                IPD_State=IPD_PAYLOAD;
                continue;
            }
            IPD_State=IPD_SEARCH;
            IPD_Match=0;
            continue;
        }

        /*
            数据
        */
        if(IPD_State==IPD_PAYLOAD)
        {
            HTTP_Process_IPD_Byte(data);

            if(IPD_Remain)
                IPD_Remain--;

            if(IPD_Remain==0)
            {
                IPD_State=IPD_SEARCH;
                IPD_Match=0;
            }
        }
    }
}

/*=========================================================
    TCP关闭
=========================================================*/
void ESP8266_TCP_Close(void)
{
    USART1_SendString("AT+CIPCLOSE\r\n");
//	ESP_WaitString("OK",1000);
    Delay_ms(1000);
	USART1_ClearRxBuffer();
}

/*=========================================================
    获取服务器版本
=========================================================*/
uint8_t ESP8266_HTTP_Get_Version(const char *server_ip,uint32_t *version)
{
    uint32_t timeout=0;

    Http_Mode=HTTP_MODE_VERSION;

    Http_Header_Len=0;
    Http_Header_Done=0;
    Server_Version=0;

    if(ESP8266_TCP_Connect(server_ip)==0)
    {
        return 0;
    }

    if(ESP8266_HTTP_GET(server_ip,"/version.txt")==0)
    {
        return 0;
    }

    while(1)
    {
        ESP8266_Process_Rx();

        if(Server_Version>0)
        {
            *version=Server_Version;
            break;
        }

        Delay_ms(10);
        timeout++;
		
        if(timeout>3000)
        {
            return 0;
        }
    }
	USART1_ClearRxBuffer();

    ESP8266_TCP_Close();

    return 1;
}

/*=========================================================
    APP下载
=========================================================*/
uint8_t ESP8266_HTTP_Download_APP(const char *server_ip)
{
    uint32_t timeout=0;

    Http_Mode=HTTP_MODE_APP;
    Http_Header_Len=0;
    Http_Header_Done=0;
    Http_Content_Length=0;
    Http_Body_Received=0;
    OTA_Data_Len=0;
    OTA_Seq=1;


    /*
        TCP重新连接
    */
    if(ESP8266_TCP_Connect(server_ip)==0)
        return 0;

    if(ESP8266_HTTP_GET(server_ip,"/APP.bin")==0)
        return 0;

    USART2_SendString("DOWNLOAD APP\r\n");

    while(1)
    {
        ESP8266_Process_Rx();

        if(Http_Content_Length>0 && Http_Body_Received>=Http_Content_Length)
        {
            break;
        }

        Delay_ms(10);
        timeout++;

        if(timeout>HTTP_TIMEOUT_COUNT)
        {
            USART2_SendString("DOWNLOAD TIMEOUT\r\n");
            return 0;
        }
    }

    /*
        剩余不足128字节
    */
    if(OTA_Data_Len>0)
    {
        OTA_Send_Data_Buffer();
    }

    /*
        CRC结束
    */
    Http_CRC32^=0xffffffff;
    USART2_SendString("CRC32=");
    USART2_SendHex(Http_CRC32>>24);
    USART2_SendHex(Http_CRC32>>16);
    USART2_SendHex(Http_CRC32>>8);
    USART2_SendHex(Http_CRC32);
    USART2_SendString("\r\n");

    OTA_EndPacket_t end;
    end.crc32=Http_CRC32;
    OTA_End_Process(&end);

    ESP8266_TCP_Close();

    return 1;
}
