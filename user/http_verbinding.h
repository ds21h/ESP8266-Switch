/*
 * httpd.h
 *
 *  Created on: Nov 19, 2014
 *      Author: frans-willem
 */

#ifndef CONFIG_HTTP_H_
#define CONFIG_HTTP_H_
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>

#define HTTP_MAX_CONN			4
#define HTTP_BUFFER_SIZE		1024
#define HTTP_URI_SIZE			256
#define HTTP_VERB_SIZE			7

#define HTTP_STATE_INIT			0
#define HTTP_STATE_PARSEVERB	1
#define HTTP_STATE_PARSEHDRS	2
#define HTTP_STATE_VERWDATA		3
#define HTTP_STATE_VERWERKT		4
#define HTTP_STATE_ANTWOORD		5

void http_init();

#endif /* CONFIG_HTTP_H_ */
