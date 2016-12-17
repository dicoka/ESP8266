/*
 * pb_websetup.c
 *
 *  Created on: 14 рту. 2016 у.
 *      Author: santi
 */

//#define DEBUG_WEB

#include "esp_common.h"
#include "pb_wifi.h"
#include "config.h"
#include "user_config.h"
#include "pb_coordination.h"
#include "pb_cgibin.h"
#include "pb_websetup.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip_addr.h"

#include "string.h"


const PATH CGI_Paths[NUM_OF_CGI] =
{
{ "Home", "/", 				home_cgi,		"#A9BCF5"},
{ "WiFi", "/wifi.cgi", 		wifi_cgi,		"#A9BCF5"},
{ "Time", "/time.cgi", 		time_cgi,		"#A9BCF5"},
{ "IFTTT", "/ifttt.cgi", 	ifttt_cgi,		"#A9BCF5"},
//{ "NTP", "/ntp.cgi", 		ntp_cgi,		"#A9BCF5"},
{	NULL, "/wifi_scan.cgi", wifi_scan_cgi,	NULL	 },
{	NULL, NULL,				not_found_cgi,	NULL	 }  //should be the last
};


void Phase_WebSetup(void)
{
	printf("\nWebSetup -> task started!\n\n");
	if (Wifi_On(SOFTAP_MODE) == WIFI_INIT_FAIL)
	{
		//Error, no WiFi connection
#ifdef DEBUG_WEB
		printf("\nPhase_Start() -> Error - NO WiFi connection");
#endif
		//TODO LED indicator for failed WiFi ???
		//TODO start WiFi AP mode ???
		return;
	}
#ifdef DEBUG_WEB
		printf("WebSetup -> WiFi OK\n");
#endif

	//	Create socket:
	int socket_websetup = lwip_socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == socket_websetup)
	{
		close(socket_websetup);
#ifdef DEBUG_WEB
		printf("WebSetup -> socket fail!\n");
#endif

		return;
	}
#ifdef DEBUG_WEB
	printf("WebSetup -> socket ok!\n");
#endif
	//Bind socket to local IP/Port:
	struct sockaddr_in local_ip;
	bzero(&local_ip, sizeof(struct sockaddr_in));
	local_ip.sin_family = AF_INET;
	local_ip.sin_len = sizeof(struct sockaddr_in);
	local_ip.sin_addr.s_addr = sta_ip_info.ip.addr;
	local_ip.sin_port = HTTP_PORT;
	if (-1 == bind(socket_websetup, (struct sockaddr * )(&local_ip),
					sizeof(struct sockaddr)))
	{
		close(socket_websetup);
		//printf("WebSetup -> bind fail!\n");
//		TASK_END(WEBSETUP_FAIL);
		return;
	}
#ifdef DEBUG_WEB
	printf("WebSetup -> bind ok!\n");
#endif
	//Establish TCP server interception:
	if (0 != listen(socket_websetup, 2))  //MAX_CONN
	{
		close(socket_websetup);
		//printf("WebSetup -> listen fail!\n");
//		TASK_END(WEBSETUP_FAIL);
		return;
	}
#ifdef DEBUG_WEB
	printf("WebSetup -> listen ok!\n");
#endif
	//Wait until TCP client is connected with the server, then start receiving data packets when TCP
	//communication is established

	int32 client_sock;
	struct sockaddr_in remote_addr;
	int32 len = sizeof(struct sockaddr_in);

	char *recv_buf = (char *)zalloc(1024);  //was 128
	int recbytes;
	char *send_buf = (char *)zalloc(3072);
	for (;;)
	{
		printf("\nWebSetup -> waiting for a client\n");
		/*block here waiting remote connect request*/
		if ((client_sock = accept(socket_websetup, (struct sockaddr *)&remote_addr,  //accept blocks program here until connection TODO block timeout ???
				(socklen_t *)&len)) < 0)
		{
#ifdef DEBUG_WEB
			printf("\nWebSetup -> accept fail\n");
#endif
			continue;
		}
#ifdef DEBUG_WEB
		printf("\nWebSetup -> Client from %s %d\n", inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));
#endif
//		while ((recbytes = read(client_sock , recv_buf, 1024)) > 0)  //1024 was 128
//		{
//		recv_buf[recbytes] = 0;
//		printf("\n\nWebSetup -> read data success %d:\n"
//				"%s", recbytes, recv_buf);
//		}

		recbytes = read(client_sock , recv_buf, 1024);
		recv_buf[recbytes] = 0;  //put end of string
		//printf("\n\nWebSetup -> read data success %d:\n"
		//		"%s", recbytes, recv_buf);

		//Find web page path - PATH_INFO string and query string - QUERY_STRING
		QUERY_STRING = 0;
		if ((PATH_INFO = strchr(recv_buf, '/')))
		{
			if ((QUERY_STRING = strchr(PATH_INFO, ' ')))
			{
				*QUERY_STRING = 0;
				if ((QUERY_STRING = strchr(PATH_INFO, '?')))
				{
					//if "?" was found, make two string, one from "/" to "?" (PATH_INFO),
					//second from "?" to space (QUERY_STRING)
					*QUERY_STRING = 0;
					QUERY_STRING++;
				}
			}
			else
				PATH_INFO = 0;
		}
#ifdef DEBUG_WEB
		printf("\nWebSetup -> PATH_INFO = %s", PATH_INFO);
		printf("\nWebSetup -> QUERY_STRING = %s", QUERY_STRING);
#endif

		// analyze PATH_INFO
		int CGI_num = NUM_OF_CGI-1;
		if (PATH_INFO)
		{
			for (CGI_num = 0; CGI_num < NUM_OF_CGI-1; CGI_num++)
				if  (strcmp(PATH_INFO, CGI_Paths[CGI_num].path_info) == 0)
					break;
		}
		// call appropriate cgi
		if ( !(*CGI_Paths[CGI_num].cgi_bin)(send_buf) )
			not_found_cgi(send_buf);
		// send buffer
		write(client_sock, send_buf, strlen(send_buf));
		close(client_sock);
	}
		free(recv_buf);
		free(send_buf);
	return;
}

