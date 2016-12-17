/*
 config.c
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

//#define DEBUG_CFG

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "user_config.h"
#include <c_types.h>

#include "pb_led.h"

#include "config.h"
#include "user_config.h"



SYSCFG sysCfg;
SAVE_FLAG saveFlag;

void CFG_Save()
{
	//TODO LED blink on save!???
	 spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
	                   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));

	if (saveFlag.flag == 0) {
		spi_flash_erase_sector(CFG_LOCATION + 1);
		spi_flash_write((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 1;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	} else {
		spi_flash_erase_sector(CFG_LOCATION + 0);
		spi_flash_write((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	}
	LedBlink (SAVE_CFG_OK);
	printf("/nCFG_Save() -> configuration saved to flash... /n");

}

void CFG_Load()
{
	spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
				   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		spi_flash_read((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	} else {
		spi_flash_read((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&sysCfg, sizeof(SYSCFG));
	}
#ifdef DEBUG_CFG
	printf("/nCFG_Load() -> configuration loaded from flash...(%d bytes)\n", sizeof(SYSCFG));
#endif
	if(sysCfg.cfg_holder != CFG_HOLDER)
	{
#ifdef DEBUG_CFG
		printf("/nCFG_Load() -> loaded configuration is WRONG (CFG holder doesn't match)");
		printf("/nCFG_Load() -> DEFAULT configuration will be loaded instead and saved to flash\n");
#endif
		CFG_Load_Defaults();
		CFG_Save(sysCfg);

	}
}

void CFG_Load_Defaults()
{
memset(&sysCfg, 0x00, sizeof sysCfg);

sysCfg.cfg_holder = CFG_HOLDER;

sysCfg.active_start = ACTIVE_START;
sysCfg.active_end = ACTIVE_END;
sysCfg.timezone = TIMEZONE;
sysCfg.period_checking = PERIOD_CHECKING;

/*
sprintf(sysCfg.sta_mode, "%s", STA_MODE);
sprintf((char*)sysCfg.sta_ip, "%s", STA_IP);
sprintf((char*)sysCfg.sta_mask, "%s", STA_MASK);
sprintf((char*)sysCfg.sta_gw, "%s", STA_GW);
sprintf((char*)sysCfg.sta_ssid, "%s", STA_SSID);
sprintf((char*)sysCfg.sta_pass, "%s", STA_PASS);
sysCfg.sta_type=STA_TYPE;
sysCfg.sta_dhcp=STA_DHCP;
*/
sprintf((char*)sysCfg.sta_ssid, "%s", STA_SSID);
sprintf((char*)sysCfg.sta_pswd, "%s", STA_PSWD);
sysCfg.sta_dhcp=STA_DHCP;
sysCfg.sta_ip_info.ip.addr = inet_addr(STA_IP);
sysCfg.sta_ip_info.netmask.addr = inet_addr(STA_NETMASK);
sysCfg.sta_ip_info.gw.addr = inet_addr(STA_GW);
//sysCfg.sta_type=STA_TYPE;
sysCfg.sta_dns[0].addr = inet_addr(STA_DNS_0);
sysCfg.sta_dns[1].addr = inet_addr(STA_DNS_1);
// NTP
strcpy (sysCfg.ntp_server[0], NTP_SERVER_0);
strcpy (sysCfg.ntp_server[1], NTP_SERVER_1);
strcpy (sysCfg.ntp_server[2], NTP_SERVER_2);
strcpy (sysCfg.ntp_server[3], NTP_SERVER_3);

sysCfg.ntp_local_port = LOCAL_PORT_NTP;
// IFTTT
sysCfg.ifttt_local_port = LOCAL_PORT_IFTTT;
strcpy (sysCfg.ifttt_key, IFTTT_KEY);
strcpy (sysCfg.ifttt_event, IFTTT_EVENT_NAME);

htons(90);
/*		os_sprintf((char *)sysCfg.ap_ip, "%s", AP_IP);
os_sprintf((char *)sysCfg.ap_mask, "%s", AP_MASK);
os_sprintf((char *)sysCfg.ap_gw, "%s", AP_GW);

sysCfg.httpd_port=HTTPD_PORT;
sysCfg.httpd_auth=HTTPD_AUTH;
os_sprintf((char *)sysCfg.httpd_user, "%s", HTTPD_USER);
os_sprintf((char *)sysCfg.httpd_pass, "%s", HTTPD_PASS);

sysCfg.ntp_enable=NTP_ENABLE;
sysCfg.ntp_tz=NTP_TZ;

sysCfg.mqtt_enable=MQTT_ENABLE;
os_sprintf((char *)sysCfg.mqtt_host, "%s", MQTT_HOST);
sysCfg.mqtt_port=MQTT_PORT;
sysCfg.mqtt_keepalive=MQTT_KEEPALIVE;
os_sprintf((char *)sysCfg.mqtt_devid, MQTT_DEVID, system_get_chip_id());
os_sprintf((char *)sysCfg.mqtt_user, "%s", MQTT_USER);
os_sprintf((char *)sysCfg.mqtt_pass, "%s", MQTT_PASS);
sysCfg.mqtt_use_ssl=MQTT_USE_SSL;
os_sprintf((char *)sysCfg.mqtt_relay_subs_topic, MQTT_RELAY_SUBS_TOPIC, system_get_chip_id());
os_sprintf((char *)sysCfg.mqtt_dht22_temp_pub_topic, MQTT_DHT22_TEMP_PUB_TOPIC, system_get_chip_id());
os_sprintf((char *)sysCfg.mqtt_dht22_humi_pub_topic, MQTT_DHT22_HUMI_PUB_TOPIC, system_get_chip_id());
os_sprintf((char *)sysCfg.mqtt_ds18b20_temp_pub_topic, MQTT_DS18B20_TEMP_PUB_TOPIC, system_get_chip_id());

sysCfg.sensor_ds18b20_enable=SENSOR_DS18B20_ENABLE;
sysCfg.sensor_dht22_enable=SENSOR_DHT22_ENABLE;
sysCfg.thermostat1_input=0; //0=DS18b20, 1=DHT22

sysCfg.relay_latching_enable=RELAY_LATCHING_ENABLE;
sysCfg.relay_1_state=0;
sysCfg.relay_2_state=0;
sysCfg.relay_3_state=0;

os_sprintf((char *)sysCfg.relay1name, "%s", RELAY1NAME);
os_sprintf((char *)sysCfg.relay2name, "%s", RELAY2NAME);
os_sprintf((char *)sysCfg.relay3name, "%s", RELAY3NAME);

sysCfg.broadcastd_enable=BROADCASTD_ENABLE;
sysCfg.broadcastd_port=BROADCASTD_PORT;
os_sprintf((char *)sysCfg.broadcastd_host, "%s", BROADCASTD_HOST);
os_sprintf((char *)sysCfg.broadcastd_url, "%s", BROADCASTD_URL);
sysCfg.broadcastd_thingspeak_channel=BROADCASTD_THINGSPEAK_CHANNEL;
os_sprintf((char *)sysCfg.broadcastd_ro_apikey, "%s", BROADCASTD_RO_APIKEY);

sysCfg.thermostat1state=0;
sysCfg.thermostat1manualsetpoint=2100;
sysCfg.thermostat1mode=THERMOSTAT_MANUAL;
sysCfg.thermostat1opmode=THERMOSTAT_HEATING;
sysCfg.thermostat1hysteresishigh=50; //in tenths of a degree, 50 means 0.5 degrees C
sysCfg.thermostat1hysteresislow=50;

//Build default schedule for the thermostat
for(int dow=0; dow<7; dow++) {
	sysCfg.thermostat1schedule.weekSched[dow].daySched[0].start=0; //0am
	sysCfg.thermostat1schedule.weekSched[dow].daySched[0].end=600; //6am, hours are * 100
	sysCfg.thermostat1schedule.weekSched[dow].daySched[0].setpoint=1000; //10.0*C
	sysCfg.thermostat1schedule.weekSched[dow].daySched[0].active=1;

	sysCfg.thermostat1schedule.weekSched[dow].daySched[1].start=600;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[1].end=900;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[1].setpoint=1800;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[1].active=1;

	sysCfg.thermostat1schedule.weekSched[dow].daySched[2].start=900;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[2].end=1700;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[2].setpoint=1600;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[2].active=1;

	sysCfg.thermostat1schedule.weekSched[dow].daySched[3].start=1700;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[3].end=2200;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[3].setpoint=2100;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[3].active=1;

	sysCfg.thermostat1schedule.weekSched[dow].daySched[4].start=2200;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[4].end=2400;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[4].setpoint=1500;
	sysCfg.thermostat1schedule.weekSched[dow].daySched[4].active=1;

	sysCfg.thermostat1schedule.weekSched[dow].daySched[5].active=0; //Terminate
}



sysCfg.thermostat2state=0;
sysCfg.thermostat2manualsetpoint=2100;
sysCfg.thermostat2mode=THERMOSTAT_MANUAL;
sysCfg.thermostat2opmode=THERMOSTAT_HEATING;
sysCfg.thermostat1hysteresishigh=50; //in tenths of a degree, 50 means 0.5 degrees C
sysCfg.thermostat1hysteresislow=50;

//Build default schedule for the thermostat
for(int dow=0; dow<7; dow++) {
	sysCfg.thermostat2schedule.weekSched[dow].daySched[0].start=0; //0am
	sysCfg.thermostat2schedule.weekSched[dow].daySched[0].end=600; //6am, hours are * 100
	sysCfg.thermostat2schedule.weekSched[dow].daySched[0].setpoint=1000; //10.0*C
	sysCfg.thermostat2schedule.weekSched[dow].daySched[0].active=1;

	sysCfg.thermostat2schedule.weekSched[dow].daySched[1].start=600;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[1].end=900;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[1].setpoint=1800;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[1].active=1;

	sysCfg.thermostat2schedule.weekSched[dow].daySched[2].start=900;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[2].end=1700;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[2].setpoint=1600;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[2].active=1;

	sysCfg.thermostat2schedule.weekSched[dow].daySched[3].start=1700;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[3].end=2200;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[3].setpoint=2100;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[3].active=1;

	sysCfg.thermostat2schedule.weekSched[dow].daySched[4].start=2200;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[4].end=2400;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[4].setpoint=1500;
	sysCfg.thermostat2schedule.weekSched[dow].daySched[4].active=1;

	sysCfg.thermostat2schedule.weekSched[dow].daySched[5].active=0; //Terminate
}


sysCfg.thermostat3state=0;
sysCfg.thermostat3manualsetpoint=2100;
sysCfg.thermostat3mode=THERMOSTAT_MANUAL;
sysCfg.thermostat3opmode=THERMOSTAT_HEATING;
sysCfg.thermostat1hysteresishigh=50; //in tenths of a degree, 50 means 0.5 degrees C
sysCfg.thermostat1hysteresislow=50;

//Build default schedule for the thermostat
for(int dow=0; dow<7; dow++) {
	sysCfg.thermostat3schedule.weekSched[dow].daySched[0].start=0; //0am
	sysCfg.thermostat3schedule.weekSched[dow].daySched[0].end=600; //6am, hours are * 100
	sysCfg.thermostat3schedule.weekSched[dow].daySched[0].setpoint=1000; //10.0*C
	sysCfg.thermostat3schedule.weekSched[dow].daySched[0].active=1;

	sysCfg.thermostat3schedule.weekSched[dow].daySched[1].start=600;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[1].end=900;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[1].setpoint=1800;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[1].active=1;

	sysCfg.thermostat3schedule.weekSched[dow].daySched[2].start=900;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[2].end=1700;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[2].setpoint=1600;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[2].active=1;

	sysCfg.thermostat3schedule.weekSched[dow].daySched[3].start=1700;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[3].end=2200;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[3].setpoint=2100;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[3].active=1;

	sysCfg.thermostat3schedule.weekSched[dow].daySched[4].start=2200;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[4].end=2400;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[4].setpoint=1500;
	sysCfg.thermostat3schedule.weekSched[dow].daySched[4].active=1;

	sysCfg.thermostat3schedule.weekSched[dow].daySched[5].active=0; //Terminate
}
*/
#ifdef DEBUG_CFG
printf("/nCFG_Load_Default() -> default configurations loaded... /n");
#endif
}



