/*
 * http-connection.c
 *
 */
#include "http_connection.h"

#include "user_config.h"
#include <math.h>
#include <osapi.h>
#include <mem.h>
#include <ip_addr.h>
#include "events.h"
#include "util.h"

#define HTTP_PORT	80

bool gHttpActive;

static struct HttpConnectionSlot mHttp_Conns[HTTP_MAX_CONN];


static struct HttpConnectionSlot ICACHE_FLASH_ATTR *sHttpFindSlot(struct espconn *pConn) {
	int lCount;
	int lRemotePort;
	struct ip_addr lIp;

	IP4_ADDR(&lIp, pConn->proto.tcp->remote_ip[0], pConn->proto.tcp->remote_ip[1], pConn->proto.tcp->remote_ip[2], pConn->proto.tcp->remote_ip[3]);

	for (lCount=0; lCount<HTTP_MAX_CONN; lCount++) {
		if (!mHttp_Conns[lCount].sFree){
			if (mHttp_Conns[lCount].sRemoteIp.addr == lIp.addr) {
				if (mHttp_Conns[lCount].sRemotePort == pConn->proto.tcp->remote_port) {
					mHttp_Conns[lCount].sConn = pConn;
					return &mHttp_Conns[lCount];
				}
			}
		}
	}
	return NULL;
}

static struct HttpConnectionSlot ICACHE_FLASH_ATTR *sHttpFindFreeSlot() {
	int lCount;

	for (lCount=0; lCount<HTTP_MAX_CONN; lCount++){
		if (mHttp_Conns[lCount].sFree){
			return &mHttp_Conns[lCount];
		}
	}
	return NULL;
}

static void ICACHE_FLASH_ATTR sHttpAddBuffer(struct HttpConnectionSlot *pSlot, uint8_t *pData, uint16_t pLen) {
	uint16_t lCopyLen;

	lCopyLen = HTTP_BUFFER_SIZE - pSlot->sBufferLen;
	if (lCopyLen > pLen){
		lCopyLen = pLen;
	}
	os_memcpy(&pSlot->sBuffer[pSlot->sBufferLen], pData, lCopyLen);
	pSlot->sBufferLen+=lCopyLen;
}

static void ICACHE_FLASH_ATTR sHttpAddBufferString(struct HttpConnectionSlot *pSlot, char *pStr) {
	sHttpAddBuffer(pSlot, pStr, os_strlen(pStr));
}

static void ICACHE_FLASH_ATTR sHttpReply(struct HttpConnectionSlot *pSlot) {
	char lTemp[32];
	uint16_t lMessageLen;

	lMessageLen=os_strlen(pSlot->sAnswer);
	pSlot->sBufferLen = 0;
	sHttpAddBufferString(pSlot, "HTTP/1.1 ");
	os_sprintf(lTemp,"%d ",pSlot->sProcessResult);
	sHttpAddBufferString(pSlot, lTemp);
	switch (pSlot->sProcessResult){
	case HTTP_OK:
		os_strcpy(lTemp, "OK");
		break;
	case HTTP_BADREQUEST:
		os_strcpy(lTemp, "Bad Request");
		break;
	case HTTP_NOTFOUND:
		os_strcpy(lTemp, "Not Found");
		break;
	case HTTP_NOTALLOWED:
		os_strcpy(lTemp, "Method Not Allowed");
		break;
	case HTTP_URITOOLONG:
		os_strcpy(lTemp, "URI Too Long");
		break;
	default:
		os_strcpy(lTemp, "Unknown");
		break;
	}
	sHttpAddBufferString(pSlot, lTemp);
	sHttpAddBufferString(pSlot, "\r\nContent-Length: ");
	os_sprintf(lTemp,"%d ", lMessageLen);
	sHttpAddBufferString(pSlot, lTemp);
	sHttpAddBufferString(pSlot, "\r\nContent-Type: text/plain\r\n\r\n");
	sHttpAddBufferString(pSlot, pSlot->sAnswer);

	if (pSlot->sBufferLen < HTTP_BUFFER_SIZE){
		pSlot->sBuffer[pSlot->sBufferLen] = '\0';
	} else {
		pSlot->sBuffer[HTTP_BUFFER_SIZE - 1] = '\0';
	}

	pSlot->sState = HTTP_STATE_REPLY;
	espconn_sent(pSlot->sConn, pSlot->sBuffer, pSlot->sBufferLen);
	ets_uart_printf("Reply sent\r\n%s", pSlot->sBuffer);
}

static void ICACHE_FLASH_ATTR sHttpErrorReply(struct HttpConnectionSlot *pSlot, char *pMessage){
	xMessageMakeErrorReply(pMessage, pSlot->sAnswer);

	sHttpReply(pSlot);
}

static void ICACHE_FLASH_ATTR sHttpProcessVerb(struct HttpConnectionSlot *pSlot, uint8_t *pLine, uint16_t pLen) {
	uint16_t lPosBlank[4];
	uint16_t lNumberBlank;
	uint16_t lCount;
	uint16_t lUriLen;

	lNumberBlank = 0;
	for (lCount = 0; lCount < pLen; lCount++){
		if (pLine[lCount] == ' '){
			if (lNumberBlank < 3){
				lPosBlank[lNumberBlank] = lCount;
				lNumberBlank++;
			}
		}
	}
	lPosBlank[lNumberBlank] = pLen;
	if (lNumberBlank < 2) {
		pSlot->sProcessResult = HTTP_BADREQUEST;
		sHttpErrorReply(pSlot, "VERB line faulty");
		return;
	}
	//Parse verb
	if (lPosBlank[0] > MESSAGE_VERB_SIZE){
		os_strcpy(pSlot->sVerb, "");
		pSlot->sProcessResult = HTTP_BADREQUEST;
		sHttpErrorReply(pSlot, "VERB too long");
		return;
	} else {
		memset(pSlot->sVerb, '\0', sizeof(pSlot->sVerb));
		os_strncpy(pSlot->sVerb, pLine, lPosBlank[0]);
	}

	//Parse URL
	lUriLen = lPosBlank[1] - (lPosBlank[0] + 1);
	if (lUriLen + 1 > MESSAGE_URI_SIZE) {
		pSlot->sProcessResult = HTTP_URITOOLONG;
		sHttpErrorReply(pSlot, "URI too long");
		return;
	}
	os_memcpy(pSlot->sUri, &pLine[lPosBlank[0] + 1], lUriLen);
	pSlot->sUri[lUriLen]='\0';
	//Continue with headers
	pSlot->sState = HTTP_STATE_PARSEHDRS;
}

static void ICACHE_FLASH_ATTR sHttpProcessHeader(struct HttpConnectionSlot *pSlot, uint8_t *pLine, uint16_t pLen) {
	int lSplit;
	int lCount;
	int lStartValue;

	// Null-line means end of headers
	if (pLen == 0) {
		if (pSlot->sContentLen > HTTP_BUFFER_SIZE){
			pSlot->sState = HTTP_STATE_REPLY;
			pSlot->sProcessResult = HTTP_BADREQUEST;
			sHttpErrorReply(pSlot, "Too much data");
		} else {
			if (pSlot->sContentLen > 0){
				pSlot->sState = HTTP_STATE_PROCESSDATA;
			} else {
				pSlot->sState = HTTP_STATE_PROCESSED;
			}
		}
		return;
	}

	lSplit = -1;
	for (lCount = 0; lCount < pLen; lCount++){
		if (pLine[lCount] == ':'){
			lSplit = lCount;
			break;
		}
	}
	if (lSplit < 0){
		return;
	}

	pLine[lSplit] = '\0';
	lStartValue = pLen - 1;
	for (lCount = lSplit + 1; lCount < pLen; lCount++){
		if (pLine[lCount] != ' ' && pLine[lCount] != '\t'){
			lStartValue = lCount;
			break;
		}
	}

	if (xStrCmpX(pLine,"Content-length") == 0) {
		pSlot->sContentLen=atoi(&pLine[lStartValue]);
	}
}

static uint16_t ICACHE_FLASH_ATTR sHttpProcessBuffer(struct HttpConnectionSlot *pSlot) {
	uint16_t lProcessed;
	uint16_t lPrevProcessed;
	uint16_t lLineStart;
	uint16_t lLineEnd;
	uint16_t lLeft;
	int lCount;
	int lFound;

	lProcessed = 0;
	do {
		lPrevProcessed = lProcessed;
		switch (pSlot->sState) {
		case HTTP_STATE_REPLY:
			return 0;
		case HTTP_STATE_PARSEVERB:
		case HTTP_STATE_PARSEHDRS:
			lFound = 0;
			for (lCount = lProcessed; lCount < pSlot->sBufferLen; lCount++){
				if (pSlot->sBuffer[lCount] == '\n'){
					lFound = 1;
					lLineEnd = lCount;
					break;
				}
			}
			if (lFound){
				lLineStart = lProcessed;
				lProcessed = lLineEnd + 1;

				if (lLineEnd > lLineStart){
					if (pSlot->sBuffer[lLineEnd - 1] == '\r'){
						lLineEnd--;
					}
				}
				pSlot->sBuffer[lLineEnd] = '\0';

				if (pSlot->sState == HTTP_STATE_PARSEVERB){
					sHttpProcessVerb(pSlot, &pSlot->sBuffer[lLineStart], lLineEnd - lLineStart);
				} else {
					sHttpProcessHeader(pSlot, &pSlot->sBuffer[lLineStart], lLineEnd - lLineStart);
				}
			}
			break;
		case HTTP_STATE_PROCESSDATA:
			lLeft = pSlot->sBufferLen - lProcessed;
			if (lLeft >= pSlot->sContentLen) {
				pSlot->sState = HTTP_STATE_PROCESSED;
			}
			break;
		}
	} while (lProcessed != lPrevProcessed);
	//Remove processed data
	if (lProcessed > 0) {
		pSlot->sBufferLen -= lProcessed;
		os_memcpy(pSlot->sBuffer, &pSlot->sBuffer[lProcessed], pSlot->sBufferLen);
	}
	return lProcessed;
}


void ICACHE_FLASH_ATTR cbHttpRecv(void *pArg, char *pData, unsigned short pLen) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;
	uint16_t lCopyLen;

	ets_uart_printf("received with length %d: %s\r\n", pLen, pData);

	lConn = (struct espconn *)pArg;
	lSlot = sHttpFindSlot(lConn);
	if (lSlot == NULL) {
		system_os_post(0, EventDisconnect, pArg);
		return;
	}
	if (lSlot->sState == HTTP_STATE_INIT){
		lSlot->sState = HTTP_STATE_PARSEVERB;
	}
	while (pLen > 0 && lSlot->sState != HTTP_STATE_PROCESSED && lSlot->sState != HTTP_STATE_REPLY) {
		lCopyLen = HTTP_BUFFER_SIZE - lSlot->sBufferLen;
		if (lCopyLen == 0) {
			lSlot->sProcessResult = HTTP_BADREQUEST;
			sHttpErrorReply(lSlot, "Buffer overflow");
			break;
		}
		if (lCopyLen > pLen){
			lCopyLen = pLen;
		}
		os_memcpy(&lSlot->sBuffer[lSlot->sBufferLen], pData, lCopyLen);
		lSlot->sBufferLen+=lCopyLen;
		pData+=lCopyLen;
		pLen-=lCopyLen;
		sHttpProcessBuffer(lSlot);
	}

	if (lSlot->sState == HTTP_STATE_PROCESSED) {
		if (lSlot->sBufferLen >= MESSAGE_SIZE) {
			lSlot->sBuffer[MESSAGE_SIZE - 1] = '\0';
		} else {
			lSlot->sBuffer[lSlot->sBufferLen] = '\0';
		}
		system_os_post(0, EventProcessMessage, lSlot);
	}
}

void ICACHE_FLASH_ATTR eHttpMessageProcessed(struct HttpConnectionSlot *pSlot) {
	sHttpReply(pSlot);
}

void ICACHE_FLASH_ATTR cbHttpSent(void *pArg) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;

	ets_uart_printf("Reply sent\r\n");

	lConn = (struct espconn *)pArg;
	lSlot = sHttpFindSlot(lConn);
	system_os_post(0, EventDisconnect, pArg);
	if (lSlot != NULL) {
		lSlot->sFree = 1;
		lSlot->sConn = NULL;
	}
}

void ICACHE_FLASH_ATTR cbHttpReconnect(void *arg) {
	//arg cannot be use. Connection no longer exists!
	int lCount;

	ets_uart_printf("Lost connection, clean up\r\n");

	for (lCount = 0; lCount < HTTP_MAX_CONN; lCount++) {
		if (!mHttp_Conns[lCount].sFree){
			if (mHttp_Conns[lCount].sConn->state == ESPCONN_NONE || mHttp_Conns[lCount].sConn->state == ESPCONN_CLOSE){
				ets_uart_printf("Connection %d freed\r\n", lCount);
				mHttp_Conns[lCount].sFree = true;
				mHttp_Conns[lCount].sConn = NULL;
			}
		}
	}
}

void ICACHE_FLASH_ATTR cbHttpDisconnect(void *pArg) {
	//arg cannot be used. Connection no longer exists!
	int lCount;

	ets_uart_printf("Lost connection, clean up\r\n");

	for (lCount = 0; lCount < HTTP_MAX_CONN; lCount++) {
		if (!mHttp_Conns[lCount].sFree){
			if (mHttp_Conns[lCount].sConn->state == ESPCONN_NONE || mHttp_Conns[lCount].sConn->state == ESPCONN_CLOSE){
				ets_uart_printf("Connection %d freed\r\n", lCount);
				mHttp_Conns[lCount].sFree = true;
				mHttp_Conns[lCount].sConn = NULL;
			}
		}
	}
}

void ICACHE_FLASH_ATTR cbHttpConnect(void *pArg) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;
	struct ip_addr lIp;

	ets_uart_printf("TCP/IP connection made\r\n");

	if (!gHttpActive){	// When systemupgrade is scheduled no new connections are accepted
		system_os_post(0, EventDisconnect, pArg);
		return;
	}
	lConn=(struct espconn *)pArg;
	lSlot = sHttpFindFreeSlot();
	if (lSlot == NULL) {
		system_os_post(0, EventDisconnect, pArg);
		return;
	}
	lSlot->sFree = 0;
	IP4_ADDR(&lIp, lConn->proto.tcp->remote_ip[0], lConn->proto.tcp->remote_ip[1], lConn->proto.tcp->remote_ip[2], lConn->proto.tcp->remote_ip[3]);
	lSlot->sRemoteIp.addr = lIp.addr;
	lSlot->sRemotePort = lConn->proto.tcp->remote_port;
	lSlot->sConn = lConn;
	lSlot->sBufferLen = 0;
	lSlot->sState = HTTP_STATE_INIT;
	lSlot->sVerb[0] = '\0';
	lSlot->sUri[0] = '\0';
	lSlot->sAnswer[0] = '\0';
	lSlot->sContentLen = 0;

	espconn_regist_recvcb(lConn, cbHttpRecv);
	espconn_regist_sentcb(lConn, cbHttpSent);
//	espconn_regist_reconcb(lConn, cbHttpReconnect);
	espconn_regist_disconcb(lConn, cbHttpDisconnect);
}

void ICACHE_FLASH_ATTR eHttpDisconnect(struct espconn *pArg) {
	espconn_disconnect(pArg);
}

void ICACHE_FLASH_ATTR eHttpInit() {
	static struct espconn lConn;
	static esp_tcp lTcp;
	unsigned int lCount;

	gHttpActive = true;
	for (lCount=0; lCount < HTTP_MAX_CONN; lCount++) {
		mHttp_Conns[lCount].sFree = true;
		mHttp_Conns[lCount].sConn = NULL;
	}

	memset(&lConn,0,sizeof(struct espconn));
	memset(&lTcp,0,sizeof(esp_tcp));
	lConn.type=ESPCONN_TCP;
	lConn.state=ESPCONN_NONE;
	lTcp.local_port=HTTP_PORT;
	lConn.proto.tcp=&lTcp;

	espconn_regist_connectcb(&lConn, cbHttpConnect);
	espconn_accept(&lConn);

	ets_uart_printf("TCP/IP initialized\r\n");
}
