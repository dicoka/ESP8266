/*
 * ESPRSSIF MIT License

 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
//#include <c_types.h>
//#include "esp_common.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/queue.h"

#define DEBUG_MAIN

#include "config.h"
#include "user_config.h"
#include "pb_sensors.h"
#include "pb_wifi.h"
#include "pb_ntp.h"
#include "pb_ifttt.h"
#include "pb_websetup.h"
#include "pb_wifi.h"
#include "pb_coordination.h"
#include "pb_led.h"

void Phase_Start(void);
void Phase_ActiveConnect(void);
void GoToDeepSleep(void);
void SetCurrentPhase(void);

void task_Main(void *pvParameters);

RTC_MEM_DATA RtcMemData;
uint8 was_mail;

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void
user_init (void)
{
	  //set the uart baudrate to 115200
	  //uart_div_modify(0, 80*1000000 / 115200);
	  //printf("SDK version:%s\n", system_get_sdk_version());
	  printf("\n-------------\nMail Box %s\n-------------", FWVER);
#ifdef DEBUG_WEB

	  printf("\nSDK version:%s\n", system_get_sdk_version());
	  printf("\nFlash size map:%d", system_get_flash_size_map ());
#endif

	  //Init GPIOs
	  GpioInit();
	  //Get the reason of restart
	  struct rst_info* MBox_rst_info = system_get_rst_info ();
	  printf("\nRST reason:%d\n", MBox_rst_info->reason);

	if (MBox_rst_info->reason == REASON_DEEP_SLEEP_AWAKE)
	//DEEP SLEEP awake
	{
		//Read phase and time
		system_rtc_mem_read(RTC_MEM_DATA_ADDR, &RtcMemData, (uint32) sizeof(RtcMemData));
		if ((RtcMemData.phase == SLEEP) || (!SensorRead())) //if sleep or no new mail
			GoToDeepSleep();
		else
			xTaskCreate(task_Main, "Main", 256, NULL, 10, NULL);
	}
	else  //POWER ON
	{
		wifi_station_set_auto_connect(0);
		RtcMemData.mail_is_old = 0;
//		if (SensorRead() == 0b00010001)  //Left and Right IR sensors are blocked - Setup is needed
		//TODO may be change to button press for setup
//		if (  GPIO_INPUT_GET( 5 ) )
			RtcMemData.phase = SETUP;
//		else
//			RtcMemData.phase = START;
		xTaskCreate(task_Main, "Main", 256, NULL, 10, NULL);
	}
	//free(PBox_rst_info);
}

void task_Main(void *pvParameters)
{
	CFG_Load();
	switch (RtcMemData.phase)
	{
	case ACTIVE_FIRST:
	case ACTIVE_MIDDLE:
	case ACTIVE_LAST:
		Phase_ActiveConnect();
		break;
	case START:
		Phase_Start();
		break;
	case SETUP:
		Phase_WebSetup();
		//Phase_Start();
		//Phase_ActiveConnect();
	}

	GoToDeepSleep();
	vTaskDelete(NULL);
}



void Phase_Start(void)
{
	LedBlink(POWER_OK);
	//TODO LedBlink could be realized as task, not function
	printf("\nPhase_Start() -> task started");
	printf("\nPhase_Start() -> loading configuration from flash");

	RtcMemData.period_checking = sysCfg.period_checking;
	RtcMemData.active_start = sysCfg.active_start;
	RtcMemData.active_end = sysCfg.active_end;
	//TODO Should "mail_is_old" be 0 by default?!
	RtcMemData.mail_is_old = 0;
	printf("\nPhase_Start() -> turning WiFi ON");

	//Here wait for WiFi connection
	if ( Wifi_On (STATION_MODE) == WIFI_INIT_FAIL)
	{
		//Error, no WiFi connection
		printf("\nPhase_Start() -> Error - NO WiFi, entering Deep Sleep mode");
		//TODO LED indicator for failed WiFi ???
		//TODO start WiFi AP mode ???
		return;
	}
	LedBlink(WIFI_OK);

	//Create Queue of messages for networking tasks coordination
	printf("\nPhase_Start() -> Creating Queue of messages for coordination");
//	xQueueMessage = xQueueCreate(2, sizeof(enum MESSAGE));
	printf("\nPhase_Start() -> Queue creation OK");

	if (NTPGetTime() == CONN_FAIL )
	{
		//Error, NTPGetTime() failed...
		//TODO fixate ifttt error for retrying in the future
		printf("\nPhase_Start() -> Error - NTPGetTime() failed...");
		return;
	}
	SetCurrentPhase();
	LedBlink(INIT_OK);
	return;
}

void Phase_ActiveConnect(void)
{
	printf("\nPhase_ActiveConnect() -> task started");
	printf("\nPhase_ActiveConnect() -> loading configuration from flash");

	printf("\nPhase_ActiveConnect() -> turning WiFi ON");
	//Here wait for WiFi connection
	if ( Wifi_On (STATION_MODE) == WIFI_INIT_FAIL)
	{
		//Error, no WiFi connection
		printf("\nPhase_ActiveConnect() -> Error - NO WiFi, entering Deep Sleep mode");
		return;
	}

	if (IFTTTPost() == CONN_FAIL )
	{
		//Error, IFTTTPost failed...
		//TODO fixate ifttt error for retrying in the future
		printf("\nPhase_ActiveConnect() -> Error - IFTTTPost failed...");
		return;
	}
	if (NTPGetTime() == CONN_FAIL )
	{
		//Error, NTPGetTime() failed...
		//TODO fixate ifttt error for retrying in the future
		printf("\nPhase_ActiveConnect() -> Error - NTPGetTime() failed...");
		return;
	}
	//TODO set exact time by NTP, but should I do it every time or not ????
	return;
}

void GoToDeepSleep(void)
{
	printf("\nEntering Deep Sleep mode...");
	uint32 period;
	system_deep_sleep_set_option(2);

	switch (RtcMemData.phase)
	{
	case ACTIVE_FIRST:
	case ACTIVE_MIDDLE:
		RtcMemData.start_time += RtcMemData.period_checking;
		if (RtcMemData.start_time < RtcMemData.active_end)
		{
			RtcMemData.phase = ACTIVE_MIDDLE;
			if (RtcMemData.start_time > SECONDS_IN_DAY)
				RtcMemData.start_time -= SECONDS_IN_DAY;
			period = RtcMemData.period_checking;
		}
		else
		{
			RtcMemData.phase = ACTIVE_LAST;
			RtcMemData.start_time = RtcMemData.active_end;
			if (RtcMemData.active_end > RtcMemData.start_time)
				period = RtcMemData.active_end - RtcMemData.start_time;
			else
			{
				period = SECONDS_IN_DAY - RtcMemData.start_time + RtcMemData.active_end;
			}
		}
		break;
	case ACTIVE_LAST	:
	case SLEEP			:
		RtcMemData.start_time += SLEEP_PERIOD_SEC;
		if (RtcMemData.start_time < RtcMemData.active_start)
		{
			RtcMemData.phase = SLEEP;
			if (RtcMemData.start_time > SECONDS_IN_DAY)
				RtcMemData.start_time -= SECONDS_IN_DAY;
			period = SLEEP_PERIOD_SEC;
		}
		else
		{
			RtcMemData.phase = ACTIVE_FIRST;
			RtcMemData.mail_is_old = 0;
			RtcMemData.start_time = RtcMemData.active_start;
			if (RtcMemData.active_start > RtcMemData.start_time)
				period = RtcMemData.active_start - RtcMemData.start_time;
			else
			{
				period = SECONDS_IN_DAY - RtcMemData.start_time + RtcMemData.active_start;
			}
		}
		break;
	case START:
		period = SLEEP_PERIOD_SEC;
		break;
	case SETUP:
		RtcMemData.phase = START;
		period = SLEEP_PERIOD_SEC;
		break;
	}

	system_rtc_mem_write (RTC_MEM_DATA_ADDR, (void *) &RtcMemData, (uint32) sizeof(RtcMemData));
//  TODO turn this on
//	SET_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);//RESET FIFO
//  CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);//all UART FIFO should be erased before the chip enters Deep-sleep mode, otherwise the system will not go into Deep-sleep
	system_deep_sleep( period * 1000000 - system_get_time() );
}

void SetCurrentPhase(void)
{
	if (RtcMemData.start_time == sysCfg.active_start)
	{
		RtcMemData.phase = ACTIVE_FIRST;
	    printf("SetCurrentPhase > ACTIVE_FIRST\n");
		return;
	}

	if (RtcMemData.start_time == sysCfg.active_end)
	{
		RtcMemData.phase = ACTIVE_LAST;
	    printf("SetCurrentPhase > ACTIVE_LAST\n");
		return;
	}

	if (RtcMemData.active_start < RtcMemData.active_end)
	{
		if (RtcMemData.start_time > RtcMemData.active_start && RtcMemData.start_time < RtcMemData.active_end)
		{
			RtcMemData.phase = ACTIVE_MIDDLE;
		    printf("SetCurrentPhase > ACTIVE_MIDDLE\n");
		}
		else
		{
			RtcMemData.phase = SLEEP;
		    printf("SetCurrentPhase > SLEEP\n");
		}
	}
	else
	{
		if (RtcMemData.start_time < RtcMemData.active_end || RtcMemData.start_time > RtcMemData.active_start)
		{
			RtcMemData.phase = ACTIVE_MIDDLE;
		    printf("SetCurrentPhase > ACTIVE_MIDDLE\n");
		}
		else
		{
			RtcMemData.phase = SLEEP;
			printf("SetCurrentPhase > SLEEP\n");
		}
	}
}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



