/*
 * pb_cgibin.c
 *
 *  Created on: 21 рту. 2016 у.
 *      Author: santi
 */

//#define DEBUG_CGI

#include "pb_ssprintf.h"

#include "pb_cgibin.h"
#include "config.h"
#include "pb_ntp.h"
#include "pb_wifi.h"
#include "pb_websetup.h"
#include "pb_coordination.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


#include "lwip/netdb.h"

char* html_id_err = "err";
char* html_id_ok  = "ok";
char* html_checked = "checked";
char* html_save_err = "<p ID=\"err\"> There are bad entries!</p>";
char* html_save_ok = "<p ID=\"ok\">Saved successfully!</p>";
char* html_save_empt = "<p ID=\"ok\">&nbsp</p>";
char* html_errors = "There are errors!";
char* save_status;

int web_page_header (char* buf, int	(*CGI_func)(char*) )
{
	int buf_cnt = 0;
	char* html_status;
	if (CGI_func == not_found_cgi) html_status = "404  Not Found";
	else html_status = "200 OK";

	// generate web page header

	const char* html_cgi_header =
			"HTTP/1.1 %s\r\n"
			"Content-Type: text/html; charset=UTF-8\r\n\r\n"
			"<html><head>"
			"<style>"
			"div.container {"
			"width: 100%; border: 0px solid gray; }"
			"header{ padding: 1px; padding-left: 20px; color: white;"
			"background-color: gray; clear: left; text-align: left; }"
			"footer {"
			" padding: 7px; padding-left: 5px; color: white;"
			" background-color: gray; clear: left; text-align: left; }"
			"nav {"
			"color: black; float: left; max-width: 50px; "
			"margin: 0; padding: 5px; padding-right: 10px; }"
			"nav ul {"
			"list-style-type: none; padding: 0; }"
			"nav ul a { text-decoration: none; }"
			"article {"
			"margin-left: 0px; border-left: 1px solid gray; padding: 5px; background-color: white; min-height: 400px;"
			"padding-left: 12px; overflow: hidden; color: black; }"
			"input { color: black; }"
			"#err { color: red; }"
			"</style>"
			"</head><body><div class=\"container\">"
			"<header><h2>Mail Box %s<h2></header>"
			"<nav>"
			"<ul>";
	buf_cnt = sprintf (buf+buf_cnt, html_cgi_header, html_status, FWVER);
	// generate menu items
	int ii;
	for (ii = 0; ii < NUM_OF_CGI - 1; ii++)
	{
		if (CGI_Paths[ii].menu_name != 0)
		{	// menu item not empty
			if (CGI_Paths[ii].cgi_bin == CGI_func)
				buf_cnt += sprintf(buf + buf_cnt, "<li style=\"background:%s\"><a href=%s>%s</a></li>",
						CGI_Paths[ii].color, CGI_Paths[ii].path_info, CGI_Paths[ii].menu_name);
			else
				buf_cnt += sprintf(buf + buf_cnt, "<li><a href=%s>%s</a></li>", CGI_Paths[ii].path_info,
						CGI_Paths[ii].menu_name);
		}
	}

	buf_cnt += sprintf (buf + buf_cnt, "</ul></nav><article>");

	return buf_cnt;
}

int web_page_footer (char* buf, int	(*CGI_func)(char*) )
{
	//generate footer
    int buf_cnt = 0;
	const char* html_cgi_footer =
			"</article>"
			"<footer>й I need a new job!!!</footer>"
			"</div></body></html>";
	buf_cnt += sprintf (buf + buf_cnt, html_cgi_footer);
	return buf_cnt;
}

enum CONN_status cgi_query_scan (char* cgi_query_str, CGI_QUERY* cgi_query, uint8 cgi_query_size)
{
	char* valstr;
	int ii;
	for (ii = 0; ii < cgi_query_size; ii++)
	{
		if ( !(valstr = strchr(cgi_query_str, '=')) ) {return CONN_FAIL;}
		*valstr = 0;
		if ( strcmp(cgi_query_str, cgi_query[ii].name) != 0 ) {return CONN_FAIL;}
		valstr++;
		if ( (cgi_query_str = strchr(valstr, '&')) )
		{
			if (ii!=(cgi_query_size - 1)) { *cgi_query_str = 0; cgi_query_str++;}
			else return CONN_FAIL;
		}
		cgi_query[ii].valstr = valstr;
#ifdef DEBUG_CGI
		printf ("\n %d %s = %02d\n", ii, cgi_query[ii].valstr, cgi_query[ii]);
#endif
	}

		return CONN_OK;
}

int not_found_cgi (char* buf)
{
	int buf_cnt = 0;
	buf_cnt += web_page_header (buf+buf_cnt, not_found_cgi);
	buf_cnt += sprintf (buf+buf_cnt, "<p>Page not found</p><p>OR Bad request</p>");
	buf_cnt += web_page_footer (buf+buf_cnt, not_found_cgi);
	return buf_cnt;
}

int home_cgi(char* buf)
{
	int buf_cnt = 0;
	buf_cnt += web_page_header (buf+buf_cnt, home_cgi);
	buf_cnt += sprintf (buf+buf_cnt, "<p>Requested home page</p><p>Here it is</p>");
	buf_cnt += web_page_footer (buf+buf_cnt, home_cgi);

	return buf_cnt;
}

int wifi_scan_cgi(char* buf)
{
	int buf_cnt = 0;
	ScanWifi();
	buf_cnt += wifi_cgi(buf + buf_cnt);
	return buf_cnt;
}

int wifi_cgi(char* buf)
{
#ifdef DEBUG_CGI
	printf("\nwificgi -> %s\n", PATH_INFO);
#endif
	char* radio_dhcp;
	char* radio_static;
	save_status = html_save_empt;
	ip_addr_t tmp_ip, tmp_netmask, tmp_gw, tmp_dns1, tmp_dns2;
	uint8 tmp_dhcp;
	int buf_cnt = 0;

#define WIFI_QUERY_NUM 8
CGI_QUERY wcq[WIFI_QUERY_NUM] =
	{
			{"ssid", NULL, "ok"},
			{"pswd", NULL, "ok"},
			{"dhcp", NULL, "ok"},
			{"ip",   "000.000.000.001", "ok"},
			{"mask", "000.000.000.002", "ok"},
			{"gate", "000.000.000.003", "ok"},
			{"dns1", "000.000.000.004", "ok"},
			{"dns2", "000.000.000.005", "ok"}
	};

	if (QUERY_STRING)
	{
		// break query to strings
		if (cgi_query_scan(QUERY_STRING, wcq, WIFI_QUERY_NUM) == CONN_FAIL)
			return 0;
		// convert query strings and check values
		if ( strlen(wcq[0].valstr)>32 ) {wcq[0].err=html_id_err; save_status = html_save_err;} //SSID
		if ( strlen(wcq[1].valstr)>32 ) {wcq[1].err=html_id_err; save_status = html_save_err;}//Password
		if ((strcmp(wcq[2].valstr, "0")) == 0)
		{
			radio_dhcp = "";
			radio_static = html_checked;
			tmp_dhcp = 0;
		}
		else
		{
			if ((strcmp(wcq[2].valstr, "1")) == 0)
			{
			radio_dhcp = html_checked;
			radio_static = "";
			tmp_dhcp = 1;
			}
			else
				return 0;
		}

		if ( !(inet_aton (wcq[3].valstr, &tmp_ip)) ) 		{wcq[3].err=html_id_err; save_status = html_save_err;}; //IP
		if ( !(inet_aton (wcq[4].valstr, &tmp_netmask)) ) 	{wcq[4].err=html_id_err; save_status = html_save_err;}; //Net mask
		if ( !(inet_aton (wcq[5].valstr, &tmp_gw)) )   	{wcq[5].err=html_id_err; save_status = html_save_err;}; //Gateway
		if ( !(inet_aton (wcq[6].valstr, &tmp_dns1)) ) 	{wcq[6].err=html_id_err; save_status = html_save_err;}; //DNS 1
		if ( !(inet_aton (wcq[7].valstr, &tmp_dns2)) ) 	{wcq[7].err=html_id_err; save_status = html_save_err;}; //DNS 2
		// if the query values are OK - save to flash
		if ( save_status != html_save_err )
		{
			if ( wcq[0].valstr[0] != 0 ) // if ssid is not empty
				{strcpy(sysCfg.sta_ssid, wcq[0].valstr);
				strcpy(sysCfg.sta_pswd, wcq[1].valstr);
				}
			sysCfg.sta_dhcp = tmp_dhcp;
			sysCfg.sta_ip_info.ip = tmp_ip;
			sysCfg.sta_ip_info.netmask = tmp_netmask;
			sysCfg.sta_ip_info.gw = tmp_gw;
			sysCfg.sta_dns[0] = tmp_dns1;
			sysCfg.sta_dns[1] = tmp_dns2;
			CFG_Save();
			save_status = html_save_ok;
		}

	}
	// if there was no query or the query was saved succefully
	// read values from saved config
	if (save_status != html_save_err)
	{
		int ii;
	//	for (ii = 3; ii<WIFI_QUERY_NUM-1; ii++)
	//		wcq[ii].valstr = malloc (16);

		//read from sysCfg
		wcq[0].valstr = ""; //SSID
		wcq[1].valstr = ""; //Password
		if (sysCfg.sta_dhcp == 0)
		{
			radio_dhcp = "";
			radio_static = html_checked;
		}
		else
		{
			radio_dhcp = html_checked;
			radio_static = "";
		}

		inet_ntoa_r(sysCfg.sta_ip_info.ip, wcq[3].valstr, 16);
		inet_ntoa_r(sysCfg.sta_ip_info.netmask, wcq[4].valstr, 16);
		inet_ntoa_r(sysCfg.sta_ip_info.gw, wcq[5].valstr, 16);
		inet_ntoa_r(sysCfg.sta_dns[0], wcq[6].valstr, 16);
		inet_ntoa_r(sysCfg.sta_dns[1], wcq[7].valstr, 16);
	}

#ifdef DEBUG_CGI
	printf ("FUCK!!! we are here");
#endif

	buf_cnt += web_page_header (buf+buf_cnt, wifi_cgi);

	const char* wifi_cgi_html =
			"<p><form action=\"wifi.cgi\">"
			"SSID:\"%s\"<br>"
			"<p>Change SSID<br>"
			"(type or choose from list)<br>"
			"<input list=\"ide\" name=\"ssid\" value=\"%s\"  />"
			"<datalist id=\"ide\">";
	buf_cnt += sprintf (buf+buf_cnt, wifi_cgi_html, sysCfg.sta_ssid, wcq[0].valstr);
		//scanned SSIDs list generation

		struct bss_info* bss_tmp;
		while (bss_link)
		{
#ifdef DEBUG_CGI
			printf ("\n\n ssid p = %p", (void*)bss_link );
			printf ("\n\n ssid len = %u", bss_link->ssid_len );
			printf ("\n\n ssid = %s", bss_link->ssid );


#endif
			buf_cnt += sprintf(buf + buf_cnt, "<option value=\"%s\" />", bss_link->ssid);
			bss_tmp = bss_link;
			bss_link = bss_link->next.stqe_next;
		}

	 	 	 wifi_cgi_html = "</datalist><a href=/wifi_scan.cgi>SCAN</a><br>"
			"Password:<br><input type=\"text\" name=\"pswd\" value=\"%s\" ID=\"%s\"></p>"
			"<p><input type=\"radio\" name=\"dhcp\" value=\"1\" %s> DHCP client<br>"
	 	 	"<input type=\"radio\" name=\"dhcp\" value=\"0\" %s> Static address</p>"
			"Static - IP:<br><input type=\"text\" name=\"ip\" value=\"%s\" ID=\"%s\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\"><br>"
			"Static - Netmask:<br><input type=\"text\" name=\"mask\" value=\"%s\" ID=\"%s\"><br>"
			"Static - Gate:<br><input type=\"text\" name=\"gate\" value=\"%s\" ID=\"%s\"><br>"
			"Static - DNS 1:<br><input type=\"text\" name=\"dns1\" value=\"%s\" ID=\"%s\"><br>"
			"Static - DNS 2:<br><input type=\"text\" name=\"dns2\" value=\"%s\" ID=\"%s\"><br>"
			"<br><input type=\"submit\">"
	 	 	"</form>%s"
	 		"</body></html>";

	buf_cnt += sprintf (buf+buf_cnt, wifi_cgi_html, wcq[1].valstr, wcq[1].err, radio_dhcp, radio_static,
			wcq[3].valstr, wcq[3].err, wcq[4].valstr, wcq[4].err, wcq[5].valstr, wcq[5].err,
			wcq[6].valstr, wcq[6].err, wcq[7].valstr, wcq[7].err, save_status );
/*
	char* valstrs[15];
	valstrs[0] = wcq[1].valstr; valstrs[1] = wcq[1].err, valstrs[2] = radio_dhcp; valstrs[3] =  radio_static;
	valstrs[4] = wcq[3].valstr; valstrs[5] = wcq[3].err; valstrs[6] =  wcq[4].valstr; valstrs[7] =  wcq[4].err; valstrs[8] =  wcq[5].valstr; valstrs[9] =  wcq[5].err;
	valstrs[10] = wcq[6].valstr; valstrs[11] =  wcq[6].err; valstrs[12] =  wcq[7].valstr; valstrs[13] =  wcq[7].err; valstrs[14] =  save_status;

	buf_cnt += sssprintf (buf+buf_cnt, wifi_cgi_html, valstrs);

*/
	/*

 	 wifi_cgi_html = "</datalist><a href=/wifi_scan.cgi>SCAN</a><br>"
			"Password:<br><input type=\"text\" name=\"pswd\" value=\"%s\" ID=\"%s\"><br><br>"
			"<input type=\"radio\" name=\"dhcp\" value=\"1\" %s> DHCP client<br>"
			"<input type=\"radio\" name=\"dhcp\" value=\"0\" %s> Static address<br>";
	buf_cnt += sprintf(buf + buf_cnt, wifi_cgi_html, wcq[1].valstr, wcq[1].err, radio_dhcp, radio_static);
	wifi_cgi_html =
			"Static - IP:<br><input type=\"text\" name=\"ip\" value=\"%s\" ID=\"%s\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\"><br>"
					"Static - Netmask:<br><input type=\"text\" name=\"mask\" value=\"%s\" ID=\"%s\"><br>"
					"Static - Gate:<br><input type=\"text\" name=\"gate\" value=\"%s\" ID=\"%s\"><br>";
	buf_cnt += sprintf(buf + buf_cnt, wifi_cgi_html, wcq[3].valstr, wcq[3].err, wcq[4].valstr, wcq[4].err,
			wcq[5].valstr, wcq[5].err);
	wifi_cgi_html = "Static - DNS 1:<br><input type=\"text\" name=\"dns1\" value=\"%s\" ID=\"%s\"><br>"
			"Static - DNS 2:<br><input type=\"text\" name=\"dns2\" value=\"%s\" ID=\"%s\"><br>"
			"<br><input type=\"submit\">"
			"</form>%s"
			"</body></html>";

	buf_cnt += sprintf(buf + buf_cnt, wifi_cgi_html,wcq[6].valstr, wcq[6].err, wcq[7].valstr, wcq[7].err, save_status );

*/

	buf_cnt += web_page_footer (buf+buf_cnt, wifi_cgi);
	return buf_cnt;
}

int time_cgi(char* buf)
{
#define TIME_QUERY_NUM 8
static CGI_QUERY tcq[TIME_QUERY_NUM] =
	{
			{"hrs_start",	"00", },
			{"min_start",	"01", },
			{"hrs_end",		"02", },
			{"min_end",		"03", },
			{"hrs_tzone",	"04", },
			{"min_tzone",	"05", },
			{"min_period",	"06", },
			{"sec_period",	"07", }
	};

int tcq_val [TIME_QUERY_NUM];
int buf_cnt = 0;
int ii;
save_status = html_save_empt;

if (QUERY_STRING)
{
	if (cgi_query_scan (QUERY_STRING, tcq, TIME_QUERY_NUM) == CONN_FAIL )
		return 0;
	for (ii=0; ii<TIME_QUERY_NUM; ii++)
	{
		tcq_val[ii] = strtol(tcq[ii].valstr, NULL , 10);
	}

	if ( 	   tcq_val[0]>=0 && tcq_val[0]<24
			&& tcq_val[1]>=0 && tcq_val[1]<60
			&& tcq_val[2]>=0 && tcq_val[2]<24
			&& tcq_val[3]>=0 && tcq_val[3]<60
			&& tcq_val[4]>-10 && tcq_val[3]<13
			&& (tcq_val[5]==0 || tcq_val[5]==15 || tcq_val[5]==30 || tcq_val[5]==45)
			&& tcq_val[6]>=0 && tcq_val[6]<60
			&& tcq_val[7]>=0 && tcq_val[7]<60   )
	{
		sysCfg.active_start = tcq_val[0] * SECONDS_IN_HOUR + tcq_val[1] * 60;
		sysCfg.active_end = tcq_val[2] * SECONDS_IN_HOUR + tcq_val[3] * 60;
		sysCfg.timezone = tcq_val[4] * SECONDS_IN_HOUR;
		if (tcq_val[4] <0 ) tcq_val[5] =  (-1) * tcq_val[5];
		sysCfg.timezone += (tcq_val[5] * 60);
		sysCfg.period_checking = tcq_val[6] * 60 + tcq_val[7];
		CFG_Save();
		save_status = html_save_ok;
	}
	else return 0;
}
	tcq_val[0] = sysCfg.active_start / 60;			//make minutes
	tcq_val[1] = tcq_val[0] % 60;					//get minutes without round hours
	tcq_val[0] = ( tcq_val[0] - tcq_val[1] ) / 60;	//get (round) hours

	tcq_val[2] = sysCfg.active_end / 60;
	tcq_val[3] = tcq_val[2] % 60;
	tcq_val[2] = ( tcq_val[2] - tcq_val[3] ) / 60;

	tcq_val[4] = sysCfg.timezone / 60;		//make minutes
	tcq_val[5] = tcq_val[4] % 60;			//get minutes without round hours
	tcq_val[4] = ( tcq_val[4] - tcq_val[5] ) / 60;	//get (round) hours
	tcq_val[5] = abs ( tcq_val[5] );				//make minutes positive


	tcq_val[7] =   sysCfg.period_checking % 60;
	tcq_val[6] = ( sysCfg.period_checking - tcq_val[7] ) / 60;

	for (ii=0; ii<TIME_QUERY_NUM; ii++)
	{
//		tcq[ii].valstr = malloc (3);
		sprintf (tcq[ii].valstr, "%02d", tcq_val[ii]);
	}
	// Web page HEADER
	buf_cnt += web_page_header (buf+buf_cnt, time_cgi);

	const char* time_cgi_html =
			"<form action=\"time.cgi\">"
			"<p>Mail check period:<br>"
			"Start time (HH:MM)<br>"
			"<input type=\"number\" name=\"hrs_start\" min=\"00\" max=\"23\" value=\"%s\" maxlength=\"2\" size=\"2\">&nbsp&nbsp:&nbsp&nbsp"
			"<input type=\"number\" name=\"min_start\" min=\"00\" max=\"59\" value=\"%s\" maxlength=\"2\" size=\"2\"><br>"
			"End time (HH:MM)<br>"
			"<input type=\"number\" name=\"hrs_end\" min=\"00\" max=\"23\" value=\"%s\" maxlength=\"2\" size=\"2\">&nbsp&nbsp:&nbsp&nbsp"
			"<input type=\"number\" name=\"min_end\" min=\"00\" max=\"59\" value=\"%s\" maxlength=\"2\" size=\"2\"></p>"
			"<p>Time zone (HH:MM)<br>"
			"<input type=\"number\" name=\"hrs_tzone\" min=\"-9\" max=\"12\" value=\"%s\" maxlength=\"2\" size=\"2\">&nbsp&nbsp:&nbsp&nbsp"
			"<input type=\"number\" name=\"min_tzone\" min=\"00\" max=\"45\" value=\"%s\" step=\"15\" maxlength=\"2\" size=\"2\"></p>"
			"<p>Check every (MM:SS)<br>"
			"<input type=\"number\" name=\"min_period\" min=\"00\" max=\"59\" value=\"%s\" maxlength=\"2\" size=\"2\">&nbsp&nbsp:&nbsp&nbsp"
			"<input type=\"number\" name=\"sec_period\" min=\"00\" max=\"59\" value=\"%s\" maxlength=\"2\" size=\"2\"></p>"
			"<input type=\"submit\">"
			"</form>%s"
			"</body></html>";

	buf_cnt += sprintf (buf+buf_cnt, time_cgi_html, tcq[0].valstr, tcq[1].valstr, tcq[2].valstr, tcq[3].valstr, tcq[4].valstr, tcq[5].valstr,
			tcq[6].valstr, tcq[7].valstr, save_status);
	//Web page FOOTER
	buf_cnt += web_page_footer (buf+buf_cnt, time_cgi);
	return buf_cnt;
}

int ifttt_cgi(char* buf)
{


	save_status = html_save_empt;
	int buf_cnt = 0;

#define IFTTT_QUERY_NUM 3
CGI_QUERY icq[IFTTT_QUERY_NUM] =
	{
			{"event", NULL, "ok"},
			{"key", NULL, "ok"},
			{"ifttt_port", "00000", "ok"}
	};


if (QUERY_STRING)
{
	// SCAN QUERY_STRING
	if (cgi_query_scan (QUERY_STRING, icq, IFTTT_QUERY_NUM) == CONN_FAIL )
		return 0;
	// Check query entries
	if ( strlen(icq[0].valstr) > 64 ) {icq[0].err=html_id_err; save_status = html_save_err;};
	if ( strlen(icq[1].valstr) > 64 ) {icq[1].err=html_id_err; save_status = html_save_err;};
	int32 ifttt_port = strtol(icq[2].valstr, NULL , 10);
	if ( (ifttt_port<1 && ifttt_port>65535) || ifttt_port == ntohs(sysCfg.ntp_local_port)  || ifttt_port == ntohs(HTTP_PORT))
	{icq[2].err=html_id_err; save_status = html_save_err;};
	// if the query values are OK - save to flash
	if ( save_status != html_save_err )
	{
		strcpy (sysCfg.ifttt_event, icq[0].valstr);
		strcpy (sysCfg.ifttt_key, icq[1].valstr);
		sysCfg.ifttt_local_port = htons(ifttt_port);
		CFG_Save();
		save_status = html_save_ok;
	}

}
// if there was no query or the query was saved succefully
// read values from saved config
if (save_status != html_save_err)
{
icq[0].valstr = sysCfg.ifttt_event;
icq[1].valstr = sysCfg.ifttt_key;
sprintf (icq[2].valstr, "%u", ntohs( sysCfg.ifttt_local_port) );
}
// Web page HEADER
buf_cnt += web_page_header (buf+buf_cnt, ifttt_cgi);

const char* ifttt_cgi_html =
		"<form action=\"ifttt.cgi\">"
		"<p>IFTTT Event Name:<br>"
		"<input type=\"text\" name=\"event\" value=\"%s\"></p>"
		"<p>IFTTT key:<br>"
		"<input type=\"text\" name=\"key\" value=\"%s\"></p>"
		"<p>IFTTT local port:<br>"
		"<input type=\"number\" name=\"ifttt_port\" min=\"1\" max=\"65535\" value=\"%s\" maxlength=\"5\" size=\"5\"></p>"
		"<input type=\"submit\">"
		"</form>%s<br>"
		"</body></html>";
buf_cnt += sprintf (buf+buf_cnt, ifttt_cgi_html, icq[0].valstr, icq[1].valstr, icq[2].valstr, save_status);
//Web page FOOTER
buf_cnt += web_page_footer (buf+buf_cnt, ntp_cgi);

return buf_cnt;
}


// NTP cgi is less needed
// commented out because of IRAM problem
/*
int ntp_cgi(char* buf)
{
	save_status = html_save_empt;
	int buf_cnt = 0;

#define NTP_QUERY_NUM 5
CGI_QUERY ncq[NTP_QUERY_NUM] =
	{
			{"ntp1", NULL, "ok"},
			{"ntp2", NULL, "ok"},
			{"ntp3", NULL, "ok"},
			{"ntp4", NULL, "ok"},
			{"ntp_port", "00000", "ok"}
	};


if (QUERY_STRING)
{
	// SCAN QUERY_STRING
	if (cgi_query_scan (QUERY_STRING, ncq, NTP_QUERY_NUM) == CONN_FAIL )
		return 0;
	// Check query entries
	if ( strlen(ncq[0].valstr) > 64 ) {ncq[0].err=html_id_err; save_status = html_save_err;};
	if ( strlen(ncq[1].valstr) > 64 ) {ncq[1].err=html_id_err; save_status = html_save_err;};
	if ( strlen(ncq[2].valstr) > 64 ) {ncq[2].err=html_id_err; save_status = html_save_err;};
	if ( strlen(ncq[3].valstr) > 64 ) {ncq[3].err=html_id_err; save_status = html_save_err;};
	int32 ntp_port = strtol(ncq[4].valstr, NULL , 10);
	if (( ntp_port<1 && ntp_port>65535)|| ntp_port == ntohs(sysCfg.ifttt_local_port)  || ntp_port == ntohs(HTTP_PORT) )
	{ncq[4].err=html_id_err; save_status = html_save_err;};
	// if the query values are OK - save to flash
	if ( save_status != html_save_err )
	{
		strcpy (sysCfg.ntp_server[0], ncq[0].valstr);
		strcpy (sysCfg.ntp_server[1], ncq[1].valstr);
		strcpy (sysCfg.ntp_server[2], ncq[2].valstr);w
		strcpy (sysCfg.ntp_server[3], ncq[3].valstr);
		sysCfg.ntp_local_port = ntp_port;
		CFG_Save();
		save_status = html_save_ok;
	}

}
// if there was no query or the query was saved succefully
// read values from saved config
if (save_status != html_save_err)
{
ncq[0].valstr = sysCfg.ntp_server[0];
ncq[1].valstr = sysCfg.ntp_server[1];
ncq[2].valstr = sysCfg.ntp_server[2];
ncq[3].valstr = sysCfg.ntp_server[3];
sprintf (ncq[4].valstr, "%u", ntohs( sysCfg.ntp_local_port) );
}
// Web page HEADER
buf_cnt += web_page_header (buf+buf_cnt, ntp_cgi);

const char* ntp_cgi_html =
		"<form action=\"ntp.cgi\">"
		"<p>NTP server 1:<br>"
		"<input type=\"text\" name=\"ntp1\" value=\"%s\"><\p>"
		"<p>NTP server 2:<br>"
		"<input type=\"text\" name=\"ntp2\" value=\"%s\"></p>"
		"<p>NTP server 3:<br>"
		"<input type=\"text\" name=\"ntp3\" value=\"%s\"></p>"
		"<p>NTP server 4:<br>"
		"<input type=\"text\" name=\"ntp4\" value=\"%s\"></p>"
		"<p>NTP local port:<br>"
		"<input type=\"number\" name=\"npt_port\" min=\"1\" max=\"65535\" value=\"%s\" maxlength=\"5\" size=\"5\"></p>"
		"<input type=\"submit\">"
		"</form>%s<br>"
		"</body></html>";
buf_cnt += sprintf (buf+buf_cnt, ntp_cgi_html, ncq[0].valstr, ncq[1].valstr, ncq[2].valstr, ncq[3].valstr, ncq[4].valstr, save_status);
//Web page FOOTER
buf_cnt += web_page_footer (buf+buf_cnt, ntp_cgi);

return buf_cnt;
}

*/



