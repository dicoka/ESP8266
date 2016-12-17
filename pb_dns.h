/*
 * pb_dns.h
 *
 *  Created on: 19 рту. 2016 у.
 *      Author: santi
 */

#ifndef USER_PB_DNS_H_
#define USER_PB_DNS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "pb_coordination.h"

void DNS_resolve (char *hostname, ip_addr_t *resolved_ip);

#endif /* USER_PB_DNS_H_ */
