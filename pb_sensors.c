
//#define DEBUG_SNS

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include "gpio.h"
#include "hw_timer.h"
#include "pb_sensors.h"
#include "user_config.h"
#include "config.h"

// GPIOs combination patterns:   CBA-001 (sensor0)   CBA-010 (sensor1)   CBA-011 (sensor2)   CBA-100 (sensor3)   CBA-101 (sensor4)   CBA-110 (sensor5 - Voltage)
const uint16 Sensor_ON_Pins [] = {0b0001000000000000, 0b0010000000000000, 0b0011000000000000, 0b0100000000000000, 0b0101000000000000, 0b0110000000000000};
								//CBA - 000 - all sensors OFF


void hw_timer_cb (void *arg)	//phototransistor rise time compensation timer callback function
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	//Give semaphore back to task
	//TODO Note the functionality shown below can often be achieved in a more efficient way by using a direct to task notification in place of a semaphore (Available From FreeRTOS V8.2.0)*/
	xSemaphoreGiveFromISR ( xBinarySemaphore_hw_timer, &xHigherPriorityTaskWoken );
	//switch context (give back to the task, even) if ISR was woken by higher priority task
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void GpioInit (void)
{
	// 74HC238 pins A,B,C
	//GPIO 12,13,14 - configure output
	GPIO_ConfigTypeDef gpio_out_cfg;                		//Define GPIO Init Structure
	gpio_out_cfg.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;     //disable GPIO interrupt
	gpio_out_cfg.GPIO_Mode = GPIO_Mode_Output;              // mode
	gpio_out_cfg.GPIO_Pin = PIN_A;                          // Enable GPIO - 74HC238 pin A
	gpio_config(&gpio_out_cfg);                        		//Initialization function
	gpio_out_cfg.GPIO_Pin = PIN_B;                          // Enable GPIO - 74HC238 pin B
	gpio_config(&gpio_out_cfg);                        		//Initialization function
	gpio_out_cfg.GPIO_Pin = PIN_C;                          // Enable GPIO - 74HC238 pin C
	gpio_config(&gpio_out_cfg);                        		//Initialization function

    //LED
	//GPIO 04 - configure output
	gpio_out_cfg.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;     //disable GPIO interrupt
	gpio_out_cfg.GPIO_Mode = GPIO_Mode_Output;              // mode
	gpio_out_cfg.GPIO_Pin = PIN_LED;                        // Enable LED pin
	gpio_config(&gpio_out_cfg);

	//Setup button - PIO 5 as input
/*	gpio_out_cfg.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;     //disable GPIO interrupt
	gpio_out_cfg.GPIO_Mode = GPIO_Mode_Input;              //Input mode
	gpio_out_cfg.GPIO_Pin = GPIO_Pin_5;                        // Enable LED pin
	gpio_config(&gpio_out_cfg);
*/
//	GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, GPIO_Pin_5);
}


//void task_SensorsPoll(void *pvParameters)
uint8  SensorRead(void)
{
	//HW timer preparations
	hw_timer_init(0);
	hw_timer_set_func( (void*) hw_timer_cb);

	vSemaphoreCreateBinary(xBinarySemaphore_hw_timer);
	xSemaphoreTake(xBinarySemaphore_hw_timer, portMAX_DELAY);

	//CLEAR pins 14,13,12 all sensors OFF |15|(14)|(13)|(12)|11|10|09|08|07|06|05|(04)|03|02|01|00|
	//TODO??is it really needed??
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0b0111000000000000);
	uint8 sensor_read = 0b00000000;
	uint8 sensor;
	uint16 adc_read_IR;
	for (sensor=0; sensor<NUM_OF_SENSORS; sensor++)
	{
		//SET pins 14,13,12 to 001 - sensor 1 ON
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, Sensor_ON_Pins[sensor]);
		//PhotoTransistor rise time compensation
		hw_timer_arm(SENS_PAUSE_US);
		//Wait for timer
		xSemaphoreTake(xBinarySemaphore_hw_timer, portMAX_DELAY);
//		adc_read_IR[sensor] = system_adc_read();
		adc_read_IR 		= system_adc_read();
		//CLEAR pins 14,13,12 all sensors OFF |15|(14)|(13)|(12)|11|10|09|08|07|06|05|(04)|03|02|01|00|
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0b0111000000000000);
//		printf("task_SensorsPoll > ADCread(%d) = %04d", sensor, adc_read_IR[sensor]);
#ifdef DEBUG_SNS
		printf("\nSensorRead() > ADCread(%d) = %04d", sensor, adc_read_IR		 );
#endif
		sensor_read <<= 1;
//		if (adc_read_IR[sensor] > ADC_THRESHOLD_IR) sensor_read |= 0b00000001;
		if (adc_read_IR         > ADC_THRESHOLD_IR) sensor_read |= 0b00000001;
#ifdef DEBUG_SNS
		printf(" ( %08d )", sensor_read);
#endif
	}

	if ( !RtcMemData.mail_is_old && sensor_read)  //New mail (mail box was empty at prev. phase and now it's full)
			{
				printf("\nSensorRead() > New mail detected!\n");
				RtcMemData.mail_is_old = 1;
			}
			else
			{
				if (RtcMemData.mail_is_old && !sensor_read)  //Mail box was withdrawn (mail box was full prev. phase and now it's empty)
					RtcMemData.mail_is_old = 0;
				sensor_read = 0;
			}
	return sensor_read;
}

uint16 VoltageRead(void)
{
	//Voltage sensor ON
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, Sensor_ON_Pins[5]);
	//Voltage sensor READ
	uint16 adc_read_voltage = system_adc_read();
	//CLEAR pins 14,13,12 all sensors OFF |15|(14)|(13)|(12)|11|10|09|08|07|06|05|(04)|03|02|01|00|
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0b0111000000000000);
	printf("\nVoltageRead() > ADC read voltage = %04d", adc_read_voltage);

	return adc_read_voltage;
}
