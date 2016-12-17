/*
 * ntp.c
 *
 *  Created on: 29 мая 2016 г.
 *      Author: santi
 */

//#define DEBUG_NTP

#include "esp_common.h"
//#include "gpio.h"
//#include "hw_timer.h"
#include "pb_ntp.h"
#include "pb_wifi.h"
#include "pb_coordination.h"
#include "pb_dns.h"
#include "config.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

//#include <c_types.h>
//#include <espconn.h>
//#include <osapi.h>
//#include <mem.h>

enum CONN_status NTPGetTime(void)
{
#ifdef DEBUG_NTP
	printf("\ntask_NTPGetTime > task started!\n\n");
#endif

	int recbytes;		//used in recvfrom
	char *ntp_send_buf = (char *) zalloc(48);
	*ntp_send_buf = 0b00100011;
	char *ntp_recv_buf = (char *) zalloc(48);

	struct sockaddr ntp_return_addr;
	socklen_t ntp_return_addr_len = sizeof(ntp_return_addr);

	//		Create socket:
	int ntp_socket = lwip_socket(PF_INET, SOCK_DGRAM, 0);
	if (-1 == ntp_socket)
	{
		close(ntp_socket);
#ifdef DEBUG_NTP
		printf("taskNTPGetTime > socket fail!\n");     //TODO: do something to prevent further gethostbyname and NTP server call in case of this error
#endif
		return (CONN_FAIL);
	}
#ifdef DEBUG_NTP
	printf("task_NTPGetTime > socket ok!\n");
#endif

	//		set socket receive timeout
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(ntp_socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > socket setsockopt timeout SO_RCVTIME fail!\n");
#endif
		return (CONN_FAIL);
	}


	//UDP bind socket to local IP/Port:
	struct sockaddr_in local_ip;
	bzero(&local_ip, sizeof(struct sockaddr_in));
	local_ip.sin_family = AF_INET;
	local_ip.sin_addr.s_addr = sta_ip_info.ip.addr;
	local_ip.sin_port = sysCfg.ntp_local_port;
	if (-1 == bind(ntp_socket, (struct sockaddr * )(&local_ip),
					sizeof(struct sockaddr)))
	{
		close(ntp_socket);	//TODO: do something to prevent further gethostbyname and NTP server call
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > bind fail!\n");
#endif
		return (CONN_FAIL);
	}
#ifdef DEBUG_NTP
	printf("task_NTPGetTime > bind ok!\n");
#endif
	//Prepare struct sockaddr_in for sending UDP NTP request
	struct sockaddr_in remote_ip;
	bzero(&remote_ip, sizeof(struct sockaddr_in));
	remote_ip.sin_family = AF_INET;
	remote_ip.sin_port = SERVER_PORT_NTP;

	// DNS Resolve NPT servers IP
	ip_addr_t NTP_DNS_resolved_ip;

	int i;
	for (i = 0; i < NTP_SERVERS_NUM; i++)
	{
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > dns get host by name: %s\n", sysCfg.ntp_server[i]);
#endif

		DNS_resolve (sysCfg.ntp_server[i], &NTP_DNS_resolved_ip);

		if (!NTP_DNS_resolved_ip.addr)     //if 0 - error, address resolve failed
		{
			//Try another NTP server name, if there's one more left
#ifdef DEBUG_NTP
			printf("task_NTPGetTime > resolving #%d '%s' fail", i, sysCfg.ntp_server[i]);
#endif
			continue;
		}
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > resolved ip address: %s \n", inet_ntoa(NTP_DNS_resolved_ip.addr));
#endif

		//UDP communication, sending NTP data packets:
		remote_ip.sin_addr.s_addr = NTP_DNS_resolved_ip.addr;
		if ( sendto(ntp_socket, ntp_send_buf, 48, 0,
				(struct sockaddr * )&remote_ip, sizeof(remote_ip)) < 0)
		{
#ifdef DEBUG_NTP
			printf("task_NTPGetTime > send fail #%d\n", i);
#endif
			//TODO: may be if send failed there's no sense to continue trying other addresses
			continue;
		}
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > send success\n");
#endif
		//Receiving NPT server answer packets
		vTaskDelay(100 / portTICK_RATE_MS);
		recbytes = recvfrom(ntp_socket, ntp_recv_buf, 48, 0, &ntp_return_addr, &ntp_return_addr_len);
		if (recbytes != 48)
		{
			//second try
			vTaskDelay(100 / portTICK_RATE_MS);
			recbytes = recvfrom(ntp_socket, ntp_recv_buf, 48, 0, &ntp_return_addr, &ntp_return_addr_len);
			if (recbytes != 48)
			{
				//receive failed - try another NTP server name, if there's one more left
#ifdef DEBUG_NTP
				printf("task_NTPGetTime > receive fail #%d\n", i);
#endif
				continue;
			}
		}
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > receive success\n");
#endif
		break;
	}
	close(ntp_socket);
	//vSemaphoreDelete(xBinarySemaphoreDNSinprogress); //TODO is it needed
	free(ntp_send_buf);

	if (recbytes == 48)     //time received
	{
		//Calculate current time in seconds from midnight and day of week number
		uint32 *timestamp = (uint32 *) zalloc(32);
		*timestamp = ntp_recv_buf[40] << 24 | ntp_recv_buf[41] << 16
				| ntp_recv_buf[42] << 8 | ntp_recv_buf[43];
		free(ntp_recv_buf);
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > recv data %d bytes!\n", recbytes);
#endif
		//Adjust Time Zone
		*timestamp += sysCfg.timezone; // was += sysCfg.timezone * SECONDS_IN_HOUR;
		//Time from midnight in seconds
		uint32 timefmn = *timestamp % SECONDS_IN_DAY;
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > seconds from midnight: %u\n", timefmn);

		//Setting exact time, adjusting it to current phase startup
		RtcMemData.start_time = timefmn - system_get_time()/1000000;

		//Following is needed for future use only (maybe) and for time demonstration
		//...calculating hours, minutes and seconds
		int dayofweek = (((*timestamp - timefmn) / SECONDS_IN_DAY) % 7) + 1;
		int seconds = timefmn % 60;
		timefmn = (timefmn - seconds) / 60;
		int minutes = timefmn % 60;
		int hours = (timefmn - minutes) / 60;
		printf("task_NTPGetTime > time: %02d:%02d:%02d - Day of week:%d\n", hours,
				minutes, seconds, dayofweek);
#endif
		free(timestamp);
	}
	else    //all tries to get NPT time has failed
	{
#ifdef DEBUG_NTP
		printf("task_NTPGetTime > NTP time receive failed for ALL %d NTP SERVERS :(\n", NTP_SERVERS_NUM);
#endif
		return (CONN_FAIL);
	}

	return (CONN_OK);
}
