/*
 * pb_ssprintf.c
 *
 *  Created on: 1 сент. 2016 г.
 *      Author: santi
 *
 *      Experiment on saving IRAM:
 *      creating simpler sprintf
 *      didn't help...
 */

#include "stdarg.h"	////////////////////////
#include "string.h"
#include <c_types.h>

int ssprintf (char* buf, const char* str, int num, ...)
{
	int buf_cnt = 0;
	va_list valist;
	va_start(valist, num);
	int ii = 0;
	while (str[ii])
	{
		if (str[ii] == '%')
		{
			char * s = va_arg(valist, char*);
			strcpy(buf, s);
			uint32 slen = strlen (s);
			buf += slen;
			buf_cnt +=slen;
			ii +=2;
			continue;
		}
		*buf = str[ii];
		buf++;
		ii++;
		buf_cnt++;
	}
	*buf = 0;  //end of string
	return buf_cnt;
}

int sssprintf (char* buf, char* str, char* *svars)
{
	int buf_cnt = 0;
	int ii = 0;
	int iis= 0;
	while (str[ii])
	{
		if (str[ii] == '%')
		{

			//strcpy(buf, svars[iis]);  //change this
			uint32 slen;// = strlen (svars[iis]);
			buf += slen;
			buf_cnt +=slen;
			ii +=2;
			iis++;
			continue;
		}
		*buf = str[ii];
		buf++;
		ii++;
		buf_cnt++;
	}
	*buf = 0;  //end of string
	return buf_cnt;
}
