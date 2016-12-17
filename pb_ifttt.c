/*
 * pb_ifttt.c
 *
 *  Created on: 4 июля 2016 г.
 *      Author: santi
 */

//#define DEBUG_IFFFT

#include "esp_common.h"
#include "pb_wifi.h"
#include "config.h"
#include "user_config.h"
#include "pb_coordination.h"
#include "pb_dns.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"


enum CONN_status IFTTTPost(void)
{
#ifdef DEBUG_IFTTT
	printf("\nIFTTTPost() -> task started!\n\n");
#endif
	//	Create socket:
	int socket_ifttt = lwip_socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == socket_ifttt)
	{
		close(socket_ifttt);
#ifdef DEBUG_IFTTT
		printf("IFTTTPost() -> socket fail!\n");
#endif
		return(CONN_FAIL);
	}
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> socket ok!\n");
#endif
	//	Set socket receive timeout
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 000000;
	if (setsockopt(socket_ifttt, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
#ifdef DEBUG_IFTTT
		printf("IFTTTPost() -> socket setsockopt timeout SO_RCVTIME fail!\n");
#endif
		return(CONN_FAIL);
	}

	//Bind socket to local IP/Port:
	struct sockaddr_in local_ip;
	bzero(&local_ip, sizeof(struct sockaddr_in));
	local_ip.sin_family = AF_INET;
	local_ip.sin_addr.s_addr = sta_ip_info.ip.addr;
	local_ip.sin_port = sysCfg.ifttt_local_port;
	if (-1 == bind(socket_ifttt, (struct sockaddr * )(&local_ip),
					sizeof(struct sockaddr)))
	{
		close(socket_ifttt);
#ifdef DEBUG_IFTTT
		printf("IFTTTPost() -> bind fail!\n");
#endif
		return(CONN_FAIL);
	}
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> bind ok!\n");
	printf("IFTTTPost() -> dns get host by name: %s\n", SERVER_IFTTT);
#endif

	// DNS Resolve IFTTT server IP
	ip_addr_t IFTTT_DNS_resolved_ip;
	DNS_resolve (SERVER_IFTTT, &IFTTT_DNS_resolved_ip);


//	xSemaphoreTake(xBinarySemaphoreDNSinprogress, MAX_DNS_WAIT_MS);

	if (!IFTTT_DNS_resolved_ip.addr)     //if 0 - error, address resolve failed
	{
#ifdef DEBUG_IFTTT
		printf("IFTTTPost() -> resolving fail");
#endif
		return(CONN_FAIL);
	}
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> resolved ip address: %s \n", inet_ntoa(IFTTT_DNS_resolved_ip.addr));
#endif
	//Create TCP connection:
	struct sockaddr_in remote_ip;
	bzero(&remote_ip, sizeof(struct sockaddr_in));
	remote_ip.sin_family = AF_INET;
	remote_ip.sin_addr.s_addr = IFTTT_DNS_resolved_ip.addr;
	remote_ip.sin_port = SERVER_PORT_IFTTT;

	//Try 2 times to connect
	if (0 != connect(socket_ifttt, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr)))
	{
		if (0 != connect(socket_ifttt, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr)))
			{
			close(socket_ifttt);
#ifdef DEBUG_IFTTT
			printf("IFTTTPost() -> connect fail!\n");
#endif
			return(CONN_FAIL);
			}
	}
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> connect ok!\n");
#endif

	//TCP communication, sending data packets:
	const char* IFTTT_post = "POST /trigger/%s/with/key/%s HTTP/1.1\r\nHost: maker.ifttt.com\r\nContent-Type: application/json\r\n\r\n";
	char* send_buf_ifttt = malloc (256);
	sprintf (send_buf_ifttt, IFTTT_post, sysCfg.ifttt_event, sysCfg.ifttt_key );

    //Try 2 times to send
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> sending: %s\n", send_buf_ifttt);
#endif
	if (write(socket_ifttt, send_buf_ifttt, strlen(send_buf_ifttt) + 1) < 0)
	{
		if (write(socket_ifttt, send_buf_ifttt, strlen(send_buf_ifttt) + 1) < 0)
			{
				close(socket_ifttt);
#ifdef DEBUG_IFTTT
				printf("IFTTTPost() -> send fail\n");
#endif
				return(CONN_FAIL);
			}
	}
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> send success\n");
#endif
	//free(send_buf_ifttt);

	//TCP communication, receiving packets:

	int recbytes;
	vTaskDelay(100 / portTICK_RATE_MS);
#ifdef DEBUG_IFTTT
	printf("IFTTTPost() -> receiving data:\n");
#endif
	char *recv_buf = (char *)zalloc(30);
	if ((recbytes = read(socket_ifttt, recv_buf, 30)) < 0)
	{     //second try
		vTaskDelay(100 / portTICK_RATE_MS);
		if ((recbytes = read(socket_ifttt, recv_buf, 30)) < 0)
		{
			close(socket_ifttt);
#ifdef DEBUG_IFTTT
			printf("\nIFTTTPost() -> NO data received...!\n");
#endif
			return (CONN_FAIL);
		}
	}
	close(socket_ifttt);
	recv_buf[recbytes] = 0;
	printf("%s", recv_buf);

	int i;
	for (i=0; i<(recbytes-6); i++)
	{
		if (recv_buf[i] == ' ') break;
	}
	recv_buf[i+7] = 0;
	if ( strcmp(&recv_buf[i+1],"200 OK") )
	{
#ifdef DEBUG_IFTTT
		printf("\nIFTTTPost() -> server answered '%s' = send fail :(\n"
				"\nIFTTTPost() -> NO data received...!\n", &recv_buf[i+1]);
#endif
		return(CONN_FAIL);

	}

	free(recv_buf);
	return(CONN_OK);;
}
