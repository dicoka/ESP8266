/*
 * pb_led.h
 *
 *  Created on: 18 июля 2016 г.
 *      Author: santi
 */

#ifndef USER_PB_LED_H_
#define USER_PB_LED_H_

#include "gpio.h"

#define MAX_NUM_OF_BLINKS 5

enum LED_PATTERN_NUM {
	POWER_OK = 0,
	WIFI_OK  = 1,
	INIT_OK  = 2,
	SAVE_CFG_OK  =3
};

void LedBlink (enum LED_PATTERN_NUM pattern);

#endif /* USER_PB_LED_H_ */
