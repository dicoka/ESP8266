/*
 * sensors.h
 *
 *  Created on: 29 мая 2016 г.
 *      Author: santi
 */

#ifndef USER_PB_SENSORS_H_
#define USER_PB_SENSORS_H_

#include "esp_common.h"
#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "user_config.h"

#define SENS_PAUSE_US 10	//phototransistor rise time compensation, us
//#define NUM_OF_SENSORS 	//deifined in user_config.h - number of optic pairs, connected to 74HC238 - 5 max
#define PIN_A GPIO_Pin_12	//74HC238 pin A
#define PIN_B GPIO_Pin_13	//74HC238 pin B
#define PIN_C GPIO_Pin_14	//74HC238 pin C

//uint16 adc_read_IR[NUM_OF_SENSORS];
uint16 adc_read_voltage;
uint8 SensorRead(void);
uint16 VoltageRead(void);
void GpioInit(void);

xSemaphoreHandle xBinarySemaphore_hw_timer; //phototransistor rise time compensation timer semaphore


#endif /* USER_PB_SENSORS_H_ */
