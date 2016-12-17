/*
 * pb_cgibin.h
 *
 *  Created on: 21 рту. 2016 у.
 *      Author: santi
 */

#ifndef USER_PB_CGIBIN_H_
#define USER_PB_CGIBIN_H_

#include <c_types.h>
#include "pb_coordination.h"

char* PATH_INFO;
char* QUERY_STRING;

typedef struct //
{
	const char* menu_name;
	const char* path_info;
	int			(*cgi_bin)(char*);
	const char* color;
} PATH;

typedef struct
{
	char* name;
	char* valstr;
	char* err;
} CGI_QUERY;

int not_found_cgi(char*);
int home_cgi(char*);
int wifi_cgi(char*);
int time_cgi(char*);
int ntp_cgi(char*);
int ifttt_cgi(char*);
int wifi_scan_cgi(char*);

#define NUM_OF_CGI  6
extern const PATH CGI_Paths[NUM_OF_CGI];

#endif /* USER_PB_CGIBIN_H_ */
