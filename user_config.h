#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_


#define ICACHE_STORE_TYPEDEF_ATTR __attribute__((aligned(4),packed))
#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR __attribute__((section(".iram0.text")))
#define ICACHE_RODATA_ATTR __attribute__((section(".irom.text")))


#define CFG_HOLDER	0x00FF55A2
//#define CFG_LOCATION	0x3C	//512 (Table 5-1, 2a-esp8266-sdk_getting_started_guide_en.pdf)
#define CFG_LOCATION	0x7C	//1024, 512+512


#define FWVER "0.1"
#define NUM_OF_SENSORS	5  	//number of optic pairs, connected to 74HC238 - 6 max
#define NTP_SERVERS_NUM 4	//number of NTP servers
#define ADC_THRESHOLD_IR	350	//
#define MAX_WIFI_WAIT_MS    15000 //15 sec
#define MAX_INET_WAIT_MS 	10000 //10 sec
#define MAX_DNS_WAIT_MS		1000
#define  MAX_SCAN_WAIT_MS	5000  //5 sec


/*DEFAULT CONFIGURATIONS*/
#define ACTIVE_START 3600	    //sec from midnight
#define ACTIVE_END 82800		//sec from midnight
#define TIMEZONE 10800 //in seconds TODO change to 0
#define PERIOD_CHECKING 20		//sec

#define STA_SSID     "Santitonis_2.4GHz"  //TODO change this
#define STA_PSWD     "karamelka"		//TODO change this
#define STA_DHCP 	 0  			//0 - static IP, DHCP client OFF  1 - DHCP client ON
#define STA_IP       "192.168.1.111" //TODO change this
#define STA_NETMASK     "255.255.255.0"
#define STA_GW     "192.168.1.1"
//#define STA_TYPE     AUTH_WPA2_PSK

#define STA_DNS_0 "192.168.1.1"
#define STA_DNS_1 "8.8.8.8"

// NTP
#define NTP_SERVER_0 "0.pool.ntp.org"
#define NTP_SERVER_1 "1.pool.ntp.org"
#define NTP_SERVER_2 "2.pool.ntp.org"
#define NTP_SERVER_3 "2.pool.ntp.org"
#define LOCAL_PORT_NTP   0b0111101100000000 //port 123 in network order = htons(123)

#define SERVER_PORT_NTP 0b0111101100000000 //port 123 in network order = htons(123)

//IFTTT
#define LOCAL_PORT_IFTTT  0b1001000000011111  //port 8080
#define IFTTT_KEY "mMLctWfzRiE7EcPc7gJgboIeZwTuFHpfGbs9Q6HATDV"
#define IFTTT_EVENT_NAME "mail_received"

#define SERVER_IFTTT "maker.ifttt.com"
#define SERVER_PORT_IFTTT 0b0101000000000000 //port 80 in network order = htons(80)

/*
#define AP_IP        "192.168.4.1"
#define AP_MASK      "255.255.255.0"
#define AP_GW        "192.168.4.1"

#define HTTPD_PORT      80
#define HTTPD_AUTH      0
#define HTTPD_USER      "admin"
#define HTTPD_PASS      "pass"

#define BROADCASTD_ENABLE	0
#define BROADCASTD_PORT     80
#define BROADCASTD_HOST		"api.thingspeak.com"
#define BROADCASTD_URL		"/update?key=**RWAPI**&field1=%d&field2=%d&field3=%d&field4=%s&field5=%s&field6=%s"
#define BROADCASTD_THINGSPEAK_CHANNEL 0
#define BROADCASTD_RO_APIKEY "**ROAPI**"

#define NTP_ENABLE    1
#define NTP_TZ  	  2

#define MQTT_ENABLE			0
#define MQTT_HOST			"192.168.1.6" //host name or IP "192.168.11.1"
#define MQTT_PORT			1883
#define MQTT_KEEPALIVE		120	 //seconds
#define MQTT_DEVID			"ESP_%08X"
#define MQTT_USER			""
#define MQTT_PASS			""
#define MQTT_USE_SSL		0
#define MQTT_RELAY_SUBS_TOPIC 		"esp_%08X/out/relay/#"
#define MQTT_DHT22_TEMP_PUB_TOPIC  	"esp_%08X/in/dht22/temperature"
#define MQTT_DHT22_HUMI_PUB_TOPIC   "esp_%08X/in/dht22/humidity"
#define MQTT_DS18B20_TEMP_PUB_TOPIC "esp_%08X/in/ds18b20/temperature"

#define MQTT_BUF_SIZE		255
#define MQTT_RECONNECT_TIMEOUT 	5	//second
#define MQTT_CONNTECT_TIMER 	5
*/
#endif
