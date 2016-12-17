/*
 * pb_dns.c
 *
 *  Created on: 19 рту. 2016 у.
 *      Author: santi
 */

//#define DEBUG_DNS


#include "pb_dns.h"
#include "pb_coordination.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

xSemaphoreHandle xBinarySemaphoreDNSinprogress = 0;     //weather to wait for DNS callback function, it's Given back by callback function
ip_addr_t DNS_resolved_ip;     //IP address resolved by dns_gethostbyname (or callback) function

void DNS_resolve_cb(const char *hostname, ip_addr_t *ipaddr, void *arg)
{
/*
	if ((ipaddr) && (ipaddr->addr))		//both not zero

	{
		((ip_addr_t *)arg)->addr = ipaddr->addr;
		printf("DNS_resolve_cb() -> host %s ip resolved\n", hostname);
	}
	else
	{
		((ip_addr_t *)arg)->addr = 0;
		printf("DNS_resolve_cb() -> error: Host %s was not resolved! DNS_resolved_ip = 0 \n",
				hostname);
	};
*/
	((ip_addr_t *)arg)->addr = ipaddr->addr;
	xSemaphoreGive(xBinarySemaphoreDNSinprogress);
}

void DNS_resolve (char *hostname, ip_addr_t *resolved_ip)
{
resolved_ip->addr = 0;
if (!xBinarySemaphoreDNSinprogress)  //The semaphore hasn't been created yet
{
	vSemaphoreCreateBinary(xBinarySemaphoreDNSinprogress);
	xSemaphoreTake(xBinarySemaphoreDNSinprogress, portMAX_DELAY);
}     //TODO: set max delay
#ifdef DEBUG_DNS
printf("DNS_resolv() -> dns get host by name: %s\n", hostname);
#endif
switch (dns_gethostbyname(hostname, resolved_ip, DNS_resolve_cb, (void*)resolved_ip))
{
case ERR_OK:
	// numeric or cached, instantly returned in DNS_resolved_ip, no callback is needed
#ifdef DEBUG_DNS
	printf("DNS_resolv() -> dns_gethostbyname: ERR_OK\n");
#endif
	break;
case ERR_INPROGRESS:
	//Data will be returned via callback function
	printf("DNS_resolv() -> dns_gethostbyname: ERR_INPROGRESS - waiting for callback\n");
	xSemaphoreTake(xBinarySemaphoreDNSinprogress, MAX_DNS_WAIT_MS);
	break;
default:
	//Bad arguments in function call
#ifdef DEBUG_DNS
	printf("DNS_resolv() -> dns_gethostbyname: bad arguments in function call\n");
#endif
	//TODO: throw error ???
	resolved_ip->addr = 0;
}


}
