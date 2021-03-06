/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
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
#include "main.h"
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

#include "../ioLibrary_Driver-master/Ethernet/wizchip_conf.h"
#include "../ioLibrary_Driver-master/Ethernet/socket.h"
#include "../ioLibrary_Driver-master/Internet/SNTP/sntp.h"
#include <string.h>

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

#define http_debug

#define SEPARATOR							"=============================================\r\n"
#define WELCOME_MSG			 			"Welcome to W5100 configuration\r\n"
#define NETWORK_MSG					 	"Network configuration:\r\n"
#define IP_MSG 		 		 				" IP ADDRESS:  %d.%d.%d.%d\r\n"
#define NETMASK_MSG					 	" NETMASK:     %d.%d.%d.%d\r\n"
#define GW_MSG 		 		 				" GATEWAY:     %d.%d.%d.%d\r\n"
#define MAC_MSG		 		 				" MAC ADDRESS: %x:%x:%x:%x:%x:%x\r\n"
#define CONN_ESTABLISHED_MSG 	"Connection established with remote IP: %d.%d.%d.%d:%d\r\n"
#define SENT_MESSAGE_MSG	 		"Sent a message. Let's close the socket!\r\n"
#define WRONG_RETVAL_MSG	 		"Something went wrong; return value: %d\r\n"
#define WRONG_STATUS_MSG	 		"Something went wrong; STATUS: %d\r\n"
#define LISTEN_ERR_MSG		 		"LISTEN Error!\r\n"

char http_data[1024];
char http_request[1024];
uint16_t http_request_len=0;

uint8_t hour=0;
uint8_t minute=0;
uint8_t second=0;

uint16_t year=0;
uint8_t month=0;
uint8_t day=0;

uint8_t ntp_buf[512];

uint8_t output[4];
uint8_t input[12];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_RTC_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

void cs_sel(void);
void cs_desel(void);
uint8_t spi_rb(void);
void spi_wb(uint8_t b);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  MX_SPI2_Init();
  MX_RTC_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
	
#ifdef http_debug
	char msg[60];
#endif
	
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;

	uint8_t retVal, sockStatus;
	uint8_t bufSize[] = {2, 2, 2, 2};
	
#ifdef http_debug	
	HAL_UART_Transmit(&huart6, (uint8_t*)SEPARATOR, strlen(SEPARATOR), 100);
	HAL_UART_Transmit(&huart6, (uint8_t*)WELCOME_MSG, strlen(WELCOME_MSG), 100);
	HAL_UART_Transmit(&huart6, (uint8_t*)SEPARATOR, strlen(SEPARATOR), 100);
#endif

	reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
	reg_wizchip_spi_cbfunc(spi_rb, spi_wb);

	wizchip_init(bufSize, bufSize);
	wiz_NetInfo netInfo = { .mac 	= {0x02, 0x08, 0xdc, 0xab, 0xcd, 0xef},	// Mac address
													.ip 	= {192, 168, 2, 5},					// IP address
													.sn 	= {255, 255, 255, 0},					// Subnet mask
													.gw 	= {192, 168, 2, 2},
													.dns = {8, 8, 8, 8},
													.dhcp = NETINFO_STATIC};					// Gateway address
	wizchip_setnetinfo(&netInfo);
													
	uint8_t NTP_addr[4] = {194,146,251,100};
	datetime time;
	
#ifdef http_debug
	wizchip_getnetinfo(&netInfo);
	HAL_UART_Transmit(&huart6, (uint8_t*)NETWORK_MSG, strlen(NETWORK_MSG), 100);											
	sprintf(msg, MAC_MSG, netInfo.mac[0], netInfo.mac[1], netInfo.mac[2], netInfo.mac[3], netInfo.mac[4], netInfo.mac[5]);
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);															
	sprintf(msg, IP_MSG, netInfo.ip[0], netInfo.ip[1], netInfo.ip[2], netInfo.ip[3]);					
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);															
	sprintf(msg, NETMASK_MSG, netInfo.sn[0], netInfo.sn[1], netInfo.sn[2], netInfo.sn[3]);		
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);															
	sprintf(msg, GW_MSG, netInfo.gw[0], netInfo.gw[1], netInfo.gw[2], netInfo.gw[3]);					
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);															
#endif

	SNTP_init(1,NTP_addr,24,ntp_buf);	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
/* Open socket 0 as TCP_SOCKET with port 80 */
	if((retVal = socket(0, Sn_MR_TCP, 80, 0)) == 0) {
		/* Put socket in LISTEN mode. This means we are creating a TCP server */
		if((retVal = listen(0)) == SOCK_OK) {
			/* While socket is in LISTEN mode we wait for a remote connection */
			while(sockStatus = getSn_SR(0) == SOCK_LISTEN)
				HAL_Delay(250);
			/* OK. Got a remote peer. Let's send a message to it */
			while(1) {
				/* If connection is ESTABLISHED with remote peer */
				sockStatus = getSn_SR(0);
				if(sockStatus == SOCK_ESTABLISHED) {
#ifdef http_debug
					uint8_t remoteIP[4];
					uint16_t remotePort;
					/* Retrieving remote peer IP and port number */
					getsockopt(0, SO_DESTIP, remoteIP);
					getsockopt(0, SO_DESTPORT, (uint8_t*)&remotePort);
					sprintf(msg, CONN_ESTABLISHED_MSG, remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);
					HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
#endif					
					http_request_len = recv(0,(uint8_t *)http_request,2048);
#ifdef http_debug
					HAL_UART_Transmit(&huart6, (uint8_t*)http_request, http_request_len, 100);
#endif
					while(strstr(http_request,"\r\n\r\n") == NULL){
						http_request_len = recv(0,(uint8_t *)http_request,2048);
#ifdef http_debug
						HAL_UART_Transmit(&huart6, (uint8_t*)http_request, http_request_len, 100);
#endif
					}
					if(strstr(http_request,"GET / HTTP/1.1\r\n") != NULL){
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Clock</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"<a href=""wyjscia"">Wyjscia</a>\r\n"
															"<br>\r\n"
															"<a href=""inputs"">Wejscia</a>\r\n"
															"<br>\r\n"
															"<a href=""sensors"">Czujniki</a>\r\n"
															"<br>\r\n"
															"<a href=""config"">Konfiguracja</a>\r\n"
															"<br>\r\n"
															"</BODY>\r\n"
															"</HTML>");
					}
					else if(strstr(http_request,"GET /config HTTP/1.1\r\n") != NULL){
						/* Let's send a welcome message and closing socket */
						HAL_RTC_GetTime(&hrtc,&RTC_Time,RTC_FORMAT_BIN);
						HAL_RTC_GetDate(&hrtc,&RTC_Date,RTC_FORMAT_BIN);
						hour=RTC_Time.Hours;
						minute=RTC_Time.Minutes;
						second=RTC_Time.Seconds;
		
						year=RTC_Date.Year+2000;
						month=RTC_Date.Month;
						day=RTC_Date.Date;
						if( (RTC_Date.Month > 2 && RTC_Date.Month < 9)	// miesiace od kwietnia do wrzesnia wlacznie
										|| (RTC_Date.Month == 2 && RTC_Date.Month > ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7 ) % 7 ) ) )	// dni marca po ostaniej niedzieli
										|| (RTC_Date.Month == 2 && RTC_Date.Month == ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7 ) % 7 ) ) && RTC_Time.Hours > 0 )	// ostatnia niedziela marca, po godzinie 1.00
										|| (RTC_Date.Month == 9 && RTC_Date.Month < ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7) % 7 ) ) )	// dni pazdziernika przed ostatnia niedziela
										|| (RTC_Date.Month == 9 && RTC_Date.Month == ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7) % 7 ) ) && RTC_Time.Hours < 1 ) )	// ostatnia niedziela pazdziernika, przed godzina 1.00
						{
//								if((hour=(hour+1)%24) == 0){
//									month=month+1%12;
//								};
						}
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"Refresh: 1\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Konfiguracja</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"<h3>Konfiguracja sieciowa</h1>\r\n"
															"Adres IP: %d.%d.%d.%d\r\n"
															"<br>\r\n"
															"Maska podsieci: %d.%d.%d.%d\r\n"
															"<br>\r\n"
															"Port: 80\r\n"
															"<h3>Czas</h1>"
															"Data: %02d-%02d-%04d\r\n"
															"<br>\r\n"
															"Czas: %02d:%02d:%02d\r\n"
															"<br>\r\n"
															"Adres serwera SNTP: %d.%d.%d.%d\r\n"
															"<br>\r\n"
															"<a href=""sync_time"">Synchronizuj czas z NTP</a>\r\n"
															"</BODY>\r\n"
															"</HTML>",
															netInfo.ip[0],netInfo.ip[1],netInfo.ip[2],netInfo.ip[3],
															netInfo.sn[0], netInfo.sn[1], netInfo.sn[2], netInfo.sn[3],
															day,month,year,
															hour,minute,second,
															NTP_addr[0],NTP_addr[1],NTP_addr[2],NTP_addr[3]);
					}
					else if(strstr(http_request,"sync_time") != NULL){
						while (SNTP_run(&time) != 1);
						
						RTC_Time.Hours=time.hh;
						RTC_Time.Minutes=time.mm;
						RTC_Time.Seconds=time.ss;
						HAL_RTC_SetTime(&hrtc,&RTC_Time,RTC_FORMAT_BIN);

						RTC_Date.Year=time.yy-2000;
						RTC_Date.Month=time.mo;
						RTC_Date.Date=time.dd;
						HAL_RTC_SetDate(&hrtc,&RTC_Date,RTC_FORMAT_BIN);

						HAL_RTC_GetTime(&hrtc,&RTC_Time,RTC_FORMAT_BIN);
						HAL_RTC_GetDate(&hrtc,&RTC_Date,RTC_FORMAT_BIN);
						hour=RTC_Time.Hours;
						minute=RTC_Time.Minutes;
						second=RTC_Time.Seconds;
		
						year=RTC_Date.Year+2000;
						month=RTC_Date.Month;
						day=RTC_Date.Date;						
						
						if( (RTC_Date.Month > 2 && RTC_Date.Month < 9)	// miesiace od kwietnia do wrzesnia wlacznie
										|| (RTC_Date.Month == 2 && RTC_Date.Month > ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7 ) % 7 ) ) )	// dni marca po ostaniej niedzieli
										|| (RTC_Date.Month == 2 && RTC_Date.Month == ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7 ) % 7 ) ) && RTC_Time.Hours > 0 )	// ostatnia niedziela marca, po godzinie 1.00
										|| (RTC_Date.Month == 9 && RTC_Date.Month < ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7) % 7 ) ) )	// dni pazdziernika przed ostatnia niedziela
										|| (RTC_Date.Month == 9 && RTC_Date.Month == ( 31 - ( ( 31 - RTC_Date.Date + RTC_Date.WeekDay % 7) % 7 ) ) && RTC_Time.Hours < 1 ) )	// ostatnia niedziela pazdziernika, przed godzina 1.00
						{
//								hour=(hour+1)%24;
						}
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Synchronizacja czasu</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"Synchronizacja czasu z serwerem NTP przebiegla pomyslnie</BODY>\r\n"
															"<br>\r\n"
															"Data: %02d-%02d-%04d\r\n"
															"<br>\r\n"
															"Czas: %02d:%02d:%02d\r\n"
															"<br>\r\n"
															"<a href=""config"">Powr�t</a>\r\n"
															"</BODY>\r\n"
															"</HTML>",
															day,month,year,
															hour,minute,second);
					}
					else if(strstr(http_request,"inputs") != NULL){
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"Refresh: 1\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Wejscia</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"Wejscie1: %d\r\n"
															"<br>\r\n"
															"Wejscie2: %d\r\n"
															"<br>\r\n"
															"Wejscie3: %d\r\n"
															"<br>\r\n"
															"Wejscie4: %d\r\n"
															"<br>\r\n"
															"Wejscie5: %d\r\n"
															"<br>\r\n"
															"Wejscie6: %d\r\n"
															"<br>\r\n"
															"Wejscie7: %d\r\n"
															"<br>\r\n"
															"Wejscie8: %d\r\n"
															"<br>\r\n"
															"Wejscie9: %d\r\n"
															"<br>\r\n"
															"Wejscie10: %d\r\n"
															"<br>\r\n"
															"Wejscie11: %d\r\n"
															"<br>\r\n"
															"Wejscie12: %d\r\n"
															"<br>\r\n"
															"</BODY>\r\n"
															"</HTML>",
															input[0],input[1],input[2],input[3],
															input[4],input[5],input[6],input[7],
															input[8],input[9],input[10],input[11]);
					}
					else if(strstr(http_request,"wyjscia") != NULL){
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Wyjscia</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"<br>\r\n"
															"Wyjscie1: %d <a href=""OUT1=0"">Wlacz</a> <a href=""OUT1=1"">Wylacz</a>\r\n"
															"<br>\r\n"
															"Wyjscie2: %d <a href=""OUT2=0"">Wlacz</a> <a href=""OUT2=1"">Wylacz</a>\r\n"
															"<br>\r\n"
															"Wyjscie3: %d <a href=""OUT3=0"">Wlacz</a> <a href=""OUT3=1"">Wylacz</a>\r\n"
															"<br>\r\n"
															"Wyjscie4: %d <a href=""OUT4=0"">Wlacz</a> <a href=""OUT4=1"">Wylacz</a>\r\n"
															"</BODY>\r\n"
															"</HTML>",
															output[0],output[1],output[2],output[3]);
					}
					else if(strstr(http_request,"sensors") != NULL){
						sprintf(http_data,"HTTP/1.1 200 OK\r\n"
															"Content-Type: text/html\r\n"
															"Connection: close\r\n"
															"\r\n"
															"<!DOCTYPE HTML>\r\n"
															"<HTML>\r\n"
															"<HEAD>\r\n"
															"<TITLE>Czujniki</TITLE>\r\n"
															"</HEAD>\r\n"
															"<BODY>\r\n"
															"</HTML>");
					}
					else if(strstr(http_request,"OUT1=1") != NULL){
						output[0]=0;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
					}
					else if(strstr(http_request,"OUT1=0") != NULL){
						output[0]=1;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
					}
					else if(strstr(http_request,"OUT2=1") != NULL){
						output[1]=0;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);
					}
					else if(strstr(http_request,"OUT2=0") != NULL){
						output[1]=1;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
					}
					else if(strstr(http_request,"OUT3=1") != NULL){
						output[2]=0;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);
					}
					else if(strstr(http_request,"OUT3=0") != NULL){
						output[2]=1;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
					}
					else if(strstr(http_request,"OUT4=1") != NULL){
						output[3]=0;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET);
					}
					else if(strstr(http_request,"OUT4=0") != NULL){
						output[3]=1;
						HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
					}
					else{
						sprintf(http_data,"HTTP/1.1 404 Not Found\r\n"
															"Content-Type: text/plain; charset=utf-8\r\n"
															"Content-Length: 9\r\n"
															"Connection: close\r\n"
															"\r\n"
															"Not found\r\n");				
					}
					
					retVal = send(0, (uint8_t *)http_data,strlen(http_data));
					if((int16_t)retVal == (int16_t)strlen(http_data))
					{
#ifdef http_debug
						HAL_UART_Transmit(&huart6, (uint8_t*)SENT_MESSAGE_MSG, strlen(SENT_MESSAGE_MSG), 100);
#endif
					}
					else { /* Ops: something went wrong during data transfer */
#ifdef http_debug
						sprintf(msg, WRONG_RETVAL_MSG, retVal);
						HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
#endif
					}
					break;
				}
				else { /* Something went wrong with remote peer, maybe the connection was closed unexpectedly */
#ifdef http_debug
					sprintf(msg, WRONG_STATUS_MSG, sockStatus);
					HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
#endif
					break;
				}
			}
			
		} else /* Ops: socket not in LISTEN mode. Something went wrong */
			{
#ifdef http_debug
			HAL_UART_Transmit(&huart6, (uint8_t*)LISTEN_ERR_MSG, strlen(LISTEN_ERR_MSG), 100);
#endif
			}
	} else { /* Can't open the socket. This means something is wrong with W5100 configuration: maybe SPI issue? */
#ifdef http_debug
		sprintf(msg, WRONG_RETVAL_MSG, retVal);
		HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
#endif
	}
 
	/* We close the socket and start a connection again */
	disconnect(0);
	close(0);
		
	}
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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

/* RTC init function */
static void MX_RTC_Init(void)
{

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

    /**Initialize RTC Only 
    */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initialize RTC and set the Time and Date 
    */
  if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2){
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sDate.WeekDay = RTC_WEEKDAY_SUNDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 17;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);
  }

}

/* SPI2 init function */
static void MX_SPI2_Init(void)
{

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_RTS;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART6 init function */
static void MX_USART6_UART_Init(void)
{

  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, OUT1_Pin|OUT2_Pin|OUT3_Pin|OUT4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : IN13_Pin IN14_Pin IN15_Pin IN10_Pin 
                           IN11_Pin IN12_Pin */
  GPIO_InitStruct.Pin = IN13_Pin|IN14_Pin|IN15_Pin|IN10_Pin 
                          |IN11_Pin|IN12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : IN1_Pin */
  GPIO_InitStruct.Pin = IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_CS_Pin */
  GPIO_InitStruct.Pin = SPI_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SPI_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN0_Pin IN4_Pin */
  GPIO_InitStruct.Pin = IN0_Pin|IN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : OUT1_Pin OUT2_Pin OUT3_Pin OUT4_Pin */
  GPIO_InitStruct.Pin = OUT1_Pin|OUT2_Pin|OUT3_Pin|OUT4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : IN2_Pin */
  GPIO_InitStruct.Pin = IN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IN2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN3_Pin IN7_Pin */
  GPIO_InitStruct.Pin = IN3_Pin|IN7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

void cs_sel() {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); //CS LOW
}

void cs_desel() {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET); //CS HIGH
}

uint8_t spi_rb(void) {
	uint8_t rbuf;
	HAL_SPI_Receive(&hspi2, &rbuf, 1, 0xFFFFFFFF);
	return rbuf;
}

void spi_wb(uint8_t b) {
	HAL_SPI_Transmit(&hspi2, &b, 1, 0xFFFFFFFF);
}

void PRINT_STR(char *msg){
		HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
 if(GPIO_Pin == GPIO_PIN_0){
		input[0]=HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
		if(input[0]==0){
				output[0]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);									//enable
		}
		else{
				output[0]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_1){
		input[1]=HAL_GPIO_ReadPin(GPIOH,GPIO_PIN_1);
	 		if(input[1]==1){
				output[1]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);							//enable
		}
		else{
				output[1]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_2){
		input[2]=HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2);
		if(input[2]==1){
				output[2]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);							//enable
		}
		else{
				output[2]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_3){
		input[3]=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3);
		if(input[3]==1){
				output[3]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);							//enable
		}
		else{
				output[3]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_4){
		if(output[0]==1){
				output[0]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);								//enable
		}
		else{
				output[0]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);									//disable
		}
 }
  else if(GPIO_Pin == GPIO_PIN_7){
		if(output[1]==1){
				output[1]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);							//enable
		}
		else{
				output[1]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_10){
		if(output[2]==1){
				output[2]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);							//enable
		}
		else{
				output[2]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_11){
		if(output[3]==1){
				output[3]=0;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);							//enable
		}
		else{
				output[3]=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET);								//disable
		}
 }
 else if(GPIO_Pin == GPIO_PIN_12){
		input[8]=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_12);
 }
 else if(GPIO_Pin == GPIO_PIN_13){
		input[9]=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
 }
 else if(GPIO_Pin == GPIO_PIN_14){
		input[10]=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_14);
 }
 else if(GPIO_Pin == GPIO_PIN_15){
		input[11]=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_15);
 }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1) 
	{
	}
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
		ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
