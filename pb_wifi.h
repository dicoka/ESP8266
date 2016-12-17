/*
 * pb_wifi.h
 *
 *  Created on: 26 θώνÿ 2016 γ.
 *      Author: santi
 */
#ifndef USER_PB_WIFI_H_
#define USER_PB_WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern xSemaphoreHandle xBinarySemaphore_STAMODE_GOT_IP;

enum WIFI_INIT_RESULT
{
	WIFI_INIT_OK,
	WIFI_INIT_FAIL
};
enum WIFI_INIT_RESULT Wifi_On( WIFI_MODE);
void wifi_handle_event_cb(System_Event_t *evt);

struct bss_info *bss_link;

#endif /* USER_PB_WIFI_H_ */
