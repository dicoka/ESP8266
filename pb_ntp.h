/*
 * ntp.h
 *
 *  Created on: 29 мая 2016 г.
 *      Author: santi
 */

#ifndef USER_PB_NTP_H_
#define USER_PB_NTP_H_

#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600

#include "pb_coordination.h"

enum CONN_status NTPGetTime(void);

#endif /* USER_PB_NTP_H_ */
