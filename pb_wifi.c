/*
 * turnwifion.c
 *
 *  Created on: 26 θώνÿ 2016 γ.
 *      Author: santi
 */

//#define DEBUG_WIFI

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "pb_wifi.h"
#include "config.h"

xSemaphoreHandle xBinarySemaphore_STAMODE_GOT_IP;
struct ip_info pb_ip_info;


xQueueHandle xBinaryQueueScanDone = 0;
struct bss_info *bss_link = 0;

enum WIFI_INIT_RESULT Wifi_On (WIFI_MODE mode)
{
//	wifi_station_set_auto_connect(0); 	//moved this to user_main.c
	wifi_set_opmode(mode);

	if (mode == SOFTAP_MODE)
	{
		struct softap_config * ap_config = (struct softap_config *) zalloc(sizeof(struct softap_config));
//		wifi_softap_get_config(ap_config); // Get soft-AP config first.
		sprintf(ap_config->ssid, AP_SSID);
		sprintf(ap_config->password, AP_PASSWORD);
		ap_config->authmode = AUTH_WPA_WPA2_PSK;
		ap_config->ssid_len = 0; // or its actual SSID length
		ap_config->max_connection = AP_MAX_CONN_NUM;
		ap_config->channel = AP_CHANNEL;
		ap_config->ssid_hidden = 0;
		ap_config->beacon_interval = 100;

		wifi_softap_set_config(ap_config); // Set ESP8266 soft-AP config
		free(ap_config);
		return WIFI_INIT_OK;
	}

	/*
	 //DHCP...
	 //wifi_station_dhcpc_start(void);		//DHCP is enabled by default.
	 //This configuration interacts with static IP API (wifi_set_ip_info):
	 //If DHCP is enabled, static IP will be disabled;
	 //If static IP is enabled, DHCP will be disabled;
	 */

	vSemaphoreCreateBinary(xBinarySemaphore_STAMODE_GOT_IP);
	//TODO portMAX_DELAY - set delay
	xSemaphoreTake(xBinarySemaphore_STAMODE_GOT_IP, portMAX_DELAY);
	wifi_set_event_handler_cb(wifi_handle_event_cb);


	if (!sysCfg.sta_dhcp)	//no DHCP, use static ip
	{
		wifi_station_dhcpc_stop();
		//Set static IP address, gateway, mask
		wifi_set_ip_info(STATION_IF, &sysCfg.sta_ip_info);
		// Set DNS-Server
		dns_setserver(0, &sysCfg.sta_dns[0]);
		dns_setserver(0, &sysCfg.sta_dns[1]);
	}
	else
	{
		wifi_station_dhcpc_start();
	}
	struct station_config * st_config = (struct station_config *) zalloc(sizeof(struct station_config));
	sprintf(st_config->ssid, sysCfg.sta_ssid);\
	sprintf(st_config->password, sysCfg.sta_pswd);
	wifi_station_set_config(st_config);

	wifi_station_connect();
	free(st_config);

	if ( xSemaphoreTake(xBinarySemaphore_STAMODE_GOT_IP, MAX_WIFI_WAIT_MS / portTICK_RATE_MS) == errQUEUE_FULL)
	{
		//Error, no WiFi connection
#ifdef DEBUG_WIFI
		printf("\nTurnWifiOn() -> Error - NO WiFi connection in %dms", MAX_WIFI_WAIT_MS);
#endif
		return WIFI_INIT_FAIL;
	}
	return WIFI_INIT_OK;
}

void wifi_handle_event_cb(System_Event_t *evt)
{
#ifdef DEBUG_WIFI
	printf("event %x\n", evt->event_id);
#endif
	switch (evt->event_id)
	{
	case EVENT_STAMODE_GOT_IP:
#ifdef DEBUG_WIFI
		printf("EVENT_STAMODE_GOT_IP:\nip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
				IP2STR(&evt->event_info.got_ip.ip),
				IP2STR(&evt->event_info.got_ip.mask),
				IP2STR(&evt->event_info.got_ip.gw));
		printf("\n");
#endif
		//if we enabled dhcp we need real IP
		//if (sysCfg.sta_dhcp) wifi_get_ip_info(STATION_IF, &sta_ip_info);  //if IP is not static, get obtained IP
		sta_ip_info.ip = evt->event_info.got_ip.ip;
		sta_ip_info.netmask = evt->event_info.got_ip.mask;
		sta_ip_info.gw = evt->event_info.got_ip.gw;
		xSemaphoreGive(xBinarySemaphore_STAMODE_GOT_IP);
		break;
/*
	case EVENT_STAMODE_CONNECTED:
#ifdef DEBUG_WIFI
		printf("connect to ssid %s, channel %d\n",
				evt->event_info.connected.ssid,
				evt->event_info.connected.channel);
#endif
		break;
	case EVENT_STAMODE_DISCONNECTED:
#ifdef DEBUG_WIFI
		printf("disconnect from ssid %s, reason %d\n",
				evt->event_info.disconnected.ssid,
				evt->event_info.disconnected.reason);
#endif
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		printf("mode: %d -> %d\n", evt->event_info.auth_change.old_mode,
				evt->event_info.auth_change.new_mode);
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
#ifdef DEBUG_WIFI
		printf("station: " MACSTR "join, AID = %d\n",
				MAC2STR(evt->event_info.sta_connected.mac),
				evt->event_info.sta_connected.aid);
#endif
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
#ifdef DEBUG_WIFI
		printf("station: " MACSTR "leave, AID = %d\n",
				MAC2STR(evt->event_info.sta_disconnected.mac),
				evt->event_info.sta_disconnected.aid);
#endif
		break;
	default:
		break;
*/
	}
}


void free_BssLink (void)
{
struct bss_info *bss_tmp;
while (bss_link)
{
	bss_tmp = bss_link->next.stqe_next;
	free (bss_link);
	bss_link = bss_tmp;
}
}

void scan_done_cb (void *arg, STATUS status)
{
	uint8 return_val = 0;
	if (status == OK)
	{
		free_BssLink();
		bss_link = malloc(sizeof(struct bss_info));
		*bss_link = *((struct bss_info *)arg);
		struct bss_info *bss_tmp;
		struct bss_info *bss_tmp1;
		bss_tmp = bss_link;

		while (bss_tmp->next.stqe_next)
		{
#ifdef DEBUG_WIFI
			printf ("\n\n ssid p = %p", (void*)bss_tmp );
			printf ("\n ssid len = %u", bss_tmp->ssid_len );
			printf ("\n ssid = %s", bss_tmp->ssid );
#endif
			bss_tmp1 = bss_tmp->next.stqe_next;
			bss_tmp->next.stqe_next = malloc(sizeof(struct bss_info));
			*(bss_tmp->next.stqe_next) = *bss_tmp1;
			bss_tmp = bss_tmp->next.stqe_next;
		}
		return_val = 1;
	}

	xQueueSend (xBinaryQueueScanDone, (void*)&return_val, portMAX_DELAY);
}

uint8 ScanWifi (struct scan_config conf)
{
	if (!xBinaryQueueScanDone)  //if queue hasn't been created yet
	{
		//create queue
		xBinaryQueueScanDone = xQueueCreate( 1, sizeof (uint8));
		//xSemaphoreTake(xBinaryQueueScanDone, portMAX_DELAY);
	}
	if ( !wifi_station_scan (NULL, scan_done_cb)) //if scan failed
		return 0;
	uint8 scan_done_cb_res;
	if ((xQueueReceive(xBinaryQueueScanDone, &scan_done_cb_res, MAX_SCAN_WAIT_MS)) == pdFALSE)
		return 0;
	if (scan_done_cb_res == 0)
		return 0;
	return 1;
}



