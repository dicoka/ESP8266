/* config.h
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

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "user_config.h"
#include <c_types.h>
#include "user_config.h"
// AP
#define AP_SSID "MAIL_BOX_AP"
#define AP_PASSWORD "12345678"
#define AP_MAX_CONN_NUM 2
#define AP_CHANNEL 6

//RTC Memory
#define RTC_MEM_DATA_ADDR		120  //TODO: why addr is not 64 as in api reference PDF?
#define SLEEP_PERIOD_SEC		5 * 60 * 60 //5 hours in seconds
#define HTTP_PORT 0b0101000000000000 //port 80 in network order = htons(80)

#define PIN_LED GPIO_Pin_4			//pin 4

//Phases of PBox lifecycle
enum PHASE
{
	START = 0,	//after power off
	ACTIVE_FIRST = 1,	//first wake up in active phase after sleep
	ACTIVE_MIDDLE = 2,
	ACTIVE_LAST = 3,	//last wake up in active phase before sleep
	SLEEP = 4,
	SETUP = 5
};
struct ip_info sta_ip_info; //for real ip, gw, nm (obtained by dhcp or static)

typedef struct	//RTC_MEM_DATA, for data saved in RTC memory during Deep Sleep
{
	enum PHASE phase;					//PBox lifecycle phase after deep sleep wake
	uint8 mail_is_old;							//PBox registered mail at previous wake up
	uint32 start_time;							//PBox time after deep sleep wake
	uint32 active_start;
	uint32 active_end;
	uint32 period_checking;
} RTC_MEM_DATA;

typedef uint8 hostname[64];

typedef struct //SYSCFG
{

//4 byte alignment, hence uint32_t
	uint32 cfg_holder;

//RTC_MEM_DATA rtc_mem_data;
	int32 active_start;
	int32 active_end;
	int32 timezone;
	int32 period_checking;
	/*
	 uint8 sta_mode[8];
	 uint8 sta_ip[16];
	 uint8 sta_mask[16];
	 uint8 sta_gw[16];
	 uint8 sta_ssid[32];
	 uint8 sta_pass[32];
	 uint32 sta_type;
	 uint8 sta_dhcp;
	 */
	struct ip_info sta_ip_info;	//station IP, NETMASK, GATEWAY
	uint8 sta_ssid[32];
	uint8 sta_pswd[32];
//	uint32 sta_type;
	uint8 sta_dhcp;
	ip_addr_t sta_dns[2]; //DNS servers in static IP mode

	hostname ntp_server[NTP_SERVERS_NUM];
	uint16 ntp_local_port;

	uint16 ifttt_local_port;//TODO is it needed
	uint8 ifttt_key[64];
	uint8 ifttt_event[64];

/*
 uint8_t ap_ip[32];
 uint8_t ap_mask[32];
 uint8_t ap_gw[32];

 uint32_t httpd_port;
 uint32_t httpd_auth;
 uint8_t httpd_user[16];
 uint8_t httpd_pass[16];

 uint32_t broadcastd_enable;
 uint32_t broadcastd_port;
 uint8_t broadcastd_host[32];
 uint8_t broadcastd_url[256];
 uint32_t broadcastd_thingspeak_channel;
 uint8_t broadcastd_ro_apikey[32];

 uint32_t ntp_enable;
 int32_t ntp_tz;

 uint32_t mqtt_enable;
 uint8_t mqtt_host[64];
 uint32_t mqtt_port;
 uint32_t mqtt_keepalive;
 uint8_t mqtt_devid[32];
 uint8_t mqtt_user[32];
 uint8_t mqtt_pass[64];
 uint32_t mqtt_use_ssl;
 uint8_t mqtt_relay_subs_topic[64];
 uint8_t mqtt_dht22_temp_pub_topic[64];
 uint8_t mqtt_dht22_humi_pub_topic[64];
 uint8_t mqtt_ds18b20_temp_pub_topic[64];
 */
//
} SYSCFG;

typedef struct
{
	uint8 flag;
	uint8 pad[3];
} SAVE_FLAG;

void CFG_Save();
void CFG_Load();
void CFG_Load_Defaults();

extern SYSCFG sysCfg;
extern RTC_MEM_DATA RtcMemData;

#endif /* USER_CONFIG_H_ */
