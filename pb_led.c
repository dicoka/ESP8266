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
#include "pb_led.h"


struct LED_ONOFF_TIME {
	uint8 on_time;
	uint8 off_time;
};

const struct LED_ONOFF_TIME led_pattern [][MAX_NUM_OF_BLINKS] = {
		{{010,000},{000,000},{000,000},{000,000},{000,000}},	//blink
		{{010,010},{010,000},{000,000},{000,000},{000,000}},	//blink blink
		{{010,010},{010,010},{010,000},{000,000},{000,000}},	//blink blink blink
		{{010,010},{010,010},{010,010},{010,000},{000,000}},
};

void LedBlink (enum LED_PATTERN_NUM pattern)
{
	//blink
	int i;
	for (i=0; i<MAX_NUM_OF_BLINKS; i++)
	{
		//if (led_pattern[pattern][i].on_time == 0) break;
		//ON LED
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, PIN_LED);

		//Pause
		vTaskDelay(led_pattern[pattern][i].on_time);
		//OFF LED
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, PIN_LED);
		if(led_pattern[pattern][i].off_time == 0) break;
		//Pause
		vTaskDelay(led_pattern[pattern][i].off_time);
	}
}
