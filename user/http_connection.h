/*
 * http_connection.h
 *
 */

#ifndef CONFIG_HTTP_H_
#define CONFIG_HTTP_H_
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>

#define HTTP_MAX_CONN			4
#define HTTP_BUFFER_SIZE		1024
#define MESSAGE_VERB_SIZE		7
#define MESSAGE_URI_SIZE		256
#define MESSAGE_SIZE			1024

#define HTTP_STATE_INIT			0
#define HTTP_STATE_PARSEVERB	1
#define HTTP_STATE_PARSEHDRS	2
#define HTTP_STATE_PROCESSDATA	3
#define HTTP_STATE_PROCESSED	4
#define HTTP_STATE_REPLY		5

#define HTTP_OK					200
#define HTTP_BADREQUEST			400
#define HTTP_NOTFOUND			404
#define HTTP_NOTALLOWED			405
#define HTTP_URITOOLONG			414

void xHttpInit();

struct HttpConnectionSlot {
	bool sFree;
	struct ip_addr sRemoteIp;
	int sRemotePort;
	struct espconn *sConn;
	uint8_t sState;
	uint8_t sBuffer[HTTP_BUFFER_SIZE];
	uint16_t sBufferLen;
	uint16_t sContentLen;
	char sVerb[MESSAGE_VERB_SIZE];
	char sUri[MESSAGE_URI_SIZE];
	char sAnswer[MESSAGE_SIZE];
	uint16 sProcessResult;
};

#endif /* CONFIG_HTTP_H_ */
