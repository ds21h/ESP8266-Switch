/*
 * http-verbinding.c
 *
 */
#include "http_verbinding.h"
#include "user_config.h"
#include "schakel.h"
#include "drukknop.h"
#include <math.h>
#include <osapi.h>
#include <mem.h>
#include <bericht.h>
#include <ip_addr.h>
#include "util.h"

#define HTTP_PORT	80

struct HttpConnectionSlot {
	int sVrij;
	uint8 sRemoteIp[4];
	int sRemotePort;
	struct espconn *sConn;
	uint8_t sState;
	uint8_t sBuffer[HTTP_BUFFER_SIZE];
	uint16_t sBufferLen;
	char sVerb[HTTP_VERB_SIZE];
	char sUri[HTTP_URI_SIZE];
	uint16_t sContentLen;
};

static struct HttpConnectionSlot mHttp_Conns[HTTP_MAX_CONN];


static struct HttpConnectionSlot ICACHE_FLASH_ATTR *http_zoek_slot(struct espconn *pConn) {
	int lTel;
	uint8 lRemoteIp[4];
	int lRemotePort;

	lRemoteIp[0] = pConn->proto.tcp->remote_ip[0];
	lRemoteIp[1] = pConn->proto.tcp->remote_ip[1];
	lRemoteIp[2] = pConn->proto.tcp->remote_ip[2];
	lRemoteIp[3] = pConn->proto.tcp->remote_ip[3];
	lRemotePort = pConn->proto.tcp->remote_port;

	for (lTel=0; lTel<HTTP_MAX_CONN; lTel++) {
		if (!mHttp_Conns[lTel].sVrij){
			if (mHttp_Conns[lTel].sRemoteIp[0] == lRemoteIp[0]) {
				if (mHttp_Conns[lTel].sRemoteIp[1] == lRemoteIp[1]) {
					if (mHttp_Conns[lTel].sRemoteIp[2] == lRemoteIp[2]) {
						if (mHttp_Conns[lTel].sRemoteIp[3] == lRemoteIp[3]) {
							if (mHttp_Conns[lTel].sRemotePort == lRemotePort) {
								mHttp_Conns[lTel].sConn = pConn;
								return &mHttp_Conns[lTel];
							}
						}
					}
				}
			}
		}
	}
	return NULL;
}

static struct HttpConnectionSlot ICACHE_FLASH_ATTR *http_zoek_vrij_slot() {
	int lTel;

	for (lTel=0; lTel<HTTP_MAX_CONN; lTel++){
		if (mHttp_Conns[lTel].sVrij){
			return &mHttp_Conns[lTel];
		}
	}
	return NULL;
}

static void ICACHE_FLASH_ATTR http_add_buffer(struct HttpConnectionSlot *pSlot, uint8_t *pData, uint16_t pLen) {
	uint16_t lCopyLen;

	lCopyLen = HTTP_BUFFER_SIZE - pSlot->sBufferLen;
	if (lCopyLen > pLen){
		lCopyLen = pLen;
	}
	memcpy(&pSlot->sBuffer[pSlot->sBufferLen], pData, lCopyLen);
	pSlot->sBufferLen+=lCopyLen;
}


static void ICACHE_FLASH_ATTR http_add_buffer_string(struct HttpConnectionSlot *pSlot, char *pStr) {
	http_add_buffer(pSlot, pStr, os_strlen(pStr));
}

static void ICACHE_FLASH_ATTR http_antwoord(struct HttpConnectionSlot *pSlot, uint16_t pCode, char *pCodeBericht, char *pBericht) {
	char lTemp[32];
	uint16_t lBerichtLen;

	lBerichtLen=os_strlen(pBericht);
	pSlot->sBufferLen = 0;
	http_add_buffer_string(pSlot, "HTTP/1.1 ");
	os_sprintf(lTemp,"%d ",pCode);
	http_add_buffer_string(pSlot, lTemp);
	http_add_buffer_string(pSlot, pCodeBericht);
	http_add_buffer_string(pSlot, "\r\nContent-Length: ");
	os_sprintf(lTemp,"%d ", lBerichtLen);
	http_add_buffer_string(pSlot, lTemp);
	http_add_buffer_string(pSlot, "\r\nContent-Type: text/plain\r\n\r\n");
	http_add_buffer_string(pSlot, pBericht);

	if (pSlot->sBufferLen < HTTP_BUFFER_SIZE){
		pSlot->sBuffer[pSlot->sBufferLen] = '\0';
	} else {
		pSlot->sBuffer[HTTP_BUFFER_SIZE - 1] = '\0';
	}

	pSlot->sState = HTTP_STATE_ANTWOORD;
	espconn_sent(pSlot->sConn, pSlot->sBuffer, pSlot->sBufferLen);
	#ifdef PLATFORM_DEBUG
	ets_uart_printf("Antwoord verzonden\r\n%s", pSlot->sBuffer);
	#endif
}

void ICACHE_FLASH_ATTR fout_antwoord(struct HttpConnectionSlot *pSlot, uint16_t pCode, char *pCodeBericht, char *pMelding){
	char *lJsonAntwoord;

	lJsonAntwoord = (char *)os_zalloc(BERICHT_SIZE);

	xMaakAntwoord(pMelding, lJsonAntwoord);

	http_antwoord(pSlot, pCode, pCodeBericht, lJsonAntwoord);

	os_free(lJsonAntwoord);
}

static void ICACHE_FLASH_ATTR http_verwerk_verb(struct HttpConnectionSlot *pSlot, uint8_t *pRegel, uint16_t pLen) {
	uint16_t lSpatiePos[4];
	uint16_t lAantSpaties;
	uint16_t lTeller;
	uint16_t lUriLen;

	lAantSpaties = 0;
	for (lTeller = 0; lTeller < pLen; lTeller++){
		if (pRegel[lTeller] == ' '){
			if (lAantSpaties < 3){
				lSpatiePos[lAantSpaties] = lTeller;
				lAantSpaties++;
			}
		}
	}
	lSpatiePos[lAantSpaties] = pLen;
	if (lAantSpaties < 2) {
		fout_antwoord(pSlot, 400, "Bad Request", "VERB regel onjuist");
		return;
	}
	//Parse verb
	if (lSpatiePos[0] > HTTP_VERB_SIZE){
		os_strcpy(pSlot->sVerb, "");
		fout_antwoord(pSlot, 400, "Bad Request", "VERB te lang");
		return;
	} else {
		os_strncpy(pSlot->sVerb, pRegel, lSpatiePos[0]);
	}

	//Parse URL
	lUriLen = lSpatiePos[1] - (lSpatiePos[0] + 1);
	if (lUriLen + 1 > HTTP_URI_SIZE) {
		fout_antwoord(pSlot, 414, "Request-URI Too Long", "URI te lang");
		return;
	}
	memcpy(pSlot->sUri, &pRegel[lSpatiePos[0] + 1], lUriLen);
	pSlot->sUri[lUriLen]='\0';
	//Verder met de headers
	pSlot->sState = HTTP_STATE_PARSEHDRS;
}

static void ICACHE_FLASH_ATTR http_verwerk_header(struct HttpConnectionSlot *pSlot, uint8_t *pRegel, uint16_t pLen) {
	int lSplit;
	int lTel;
	int lStartWaarde;

	// Een nul-regel betekent Einde headers.
	if (pLen == 0) {
		if (pSlot->sContentLen > HTTP_BUFFER_SIZE){
			pSlot->sState = HTTP_STATE_ANTWOORD;
			fout_antwoord(pSlot, 400, "Bad Request", "Te veel data");
		} else {
			if (pSlot->sContentLen > 0){
				pSlot->sState = HTTP_STATE_VERWDATA;
			} else {
				pSlot->sState = HTTP_STATE_VERWERKT;
			}
		}
		return;
	}

	lSplit = -1;
	for (lTel = 0; lTel < pLen; lTel++){
		if (pRegel[lTel] == ':'){
			lSplit = lTel;
			break;
		}
	}
	if (lSplit < 0){
		return;
	}

	pRegel[lSplit] = '\0';
	lStartWaarde = pLen - 1;
	for (lTel = lSplit + 1; lTel < pLen; lTel++){
		if (pRegel[lTel] != ' ' && pRegel[lTel] != '\t'){
			lStartWaarde = lTel;
			break;
		}
	}

	if (xStrCmpX(pRegel,"Content-length") == 0) {
		pSlot->sContentLen=atoi(&pRegel[lStartWaarde]);
	}
}

static uint16_t ICACHE_FLASH_ATTR http_verwerk_buffer(struct HttpConnectionSlot *pSlot) {
	uint16_t lVerwerkt;
	uint16_t lVorigVerwerkt;
	uint16_t lBeginRegel;
	uint16_t lEindeRegel;
	uint16_t lOver;
	int lTeller;
	int lGevonden;

	lVerwerkt = 0;
	do {
		lVorigVerwerkt = lVerwerkt;
		switch (pSlot->sState) {
		case HTTP_STATE_ANTWOORD:
			return 0;
		case HTTP_STATE_PARSEVERB:
		case HTTP_STATE_PARSEHDRS:
			lGevonden = 0;
			for (lTeller = lVerwerkt; lTeller < pSlot->sBufferLen; lTeller++){
				if (pSlot->sBuffer[lTeller] == '\n'){
					lGevonden = 1;
					lEindeRegel = lTeller;
					break;
				}
			}
			if (lGevonden){
				lBeginRegel = lVerwerkt;
				lVerwerkt = lEindeRegel + 1;

				if (lEindeRegel > lBeginRegel){
					if (pSlot->sBuffer[lEindeRegel - 1] == '\r'){
						lEindeRegel--;
					}
				}
				pSlot->sBuffer[lEindeRegel] = '\0';

				if (pSlot->sState == HTTP_STATE_PARSEVERB){
					http_verwerk_verb(pSlot, &pSlot->sBuffer[lBeginRegel], lEindeRegel - lBeginRegel);
				} else {
					http_verwerk_header(pSlot, &pSlot->sBuffer[lBeginRegel], lEindeRegel - lBeginRegel);
				}
			}
			break;
		case HTTP_STATE_VERWDATA:
			//Is alles binnen?
			lOver = pSlot->sBufferLen - lVerwerkt;
			if (lOver >= pSlot->sContentLen) {
				pSlot->sState = HTTP_STATE_VERWERKT;
			}
			break;
		}
	} while (lVerwerkt != lVorigVerwerkt);
	//Verwerkte gegevens verwijderen
	if (lVerwerkt > 0) {
		pSlot->sBufferLen -= lVerwerkt;
		memcpy(pSlot->sBuffer, &pSlot->sBuffer[lVerwerkt], pSlot->sBufferLen);
	}
	return lVerwerkt;
}


void http_recv_callback(void *pArg, char *pData, unsigned short pLen) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;
	uint16_t lCopyLen;
	struct Bericht lBericht;
	struct ip_addr lIp;

	#ifdef PLATFORM_DEBUG
	ets_uart_printf("ontvangen met lengte %d: %s\r\n", pLen, pData);
	#endif

//	xDrukknopUit();

	lConn = (struct espconn *)pArg;
	lSlot = http_zoek_slot(lConn);
	if (lSlot == NULL) {
		espconn_disconnect((struct espconn *)pArg);
		return;
	}
	if (lSlot->sState == HTTP_STATE_INIT){
		lSlot->sState = HTTP_STATE_PARSEVERB;
	}
	while (pLen > 0 && lSlot->sState != HTTP_STATE_VERWERKT && lSlot->sState != HTTP_STATE_ANTWOORD) {
		lCopyLen = HTTP_BUFFER_SIZE - lSlot->sBufferLen;
		if (lCopyLen == 0) {
			fout_antwoord(lSlot, 400, "Bad Request", "Buffer overflow");
			break;
		}
		if (lCopyLen > pLen){
			lCopyLen = pLen;
		}
		memcpy(&lSlot->sBuffer[lSlot->sBufferLen], pData, lCopyLen);
		lSlot->sBufferLen+=lCopyLen;
		pData+=lCopyLen;
		pLen-=lCopyLen;
		http_verwerk_buffer(lSlot);
	}
	if (lSlot->sState == HTTP_STATE_VERWERKT) {
		//Klaar, nu verwerken
		os_strcpy(lBericht.sVerb, lSlot->sVerb);
		os_strcpy(lBericht.sUri, lSlot->sUri);
		if (lSlot->sBufferLen > BERICHT_SIZE - 1) {
			lSlot->sBuffer[BERICHT_SIZE - 1] = '\0';
		} else {
			lSlot->sBuffer[lSlot->sBufferLen] = '\0';
		}
		os_strcpy(lBericht.sBericht, lSlot->sBuffer);
		IP4_ADDR(&lIp, lSlot->sRemoteIp[0], lSlot->sRemoteIp[1], lSlot->sRemoteIp[2], lSlot->sRemoteIp[3]);
		xVerwerkBericht(lIp.addr , &lBericht);
		if (os_strncmp(lBericht.sBericht, "404", 3) == 0){
			fout_antwoord(lSlot, 404, "Not Found", "Resource not found");
		} else {
			http_antwoord(lSlot, 200, "OK", lBericht.sBericht);
		}
	}
}

void http_sent_callback(void *pArg) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;

//	xDrukknopAan();

	#ifdef PLATFORM_DEBUG
	ets_uart_printf("Antwoord echt verzonden\r\n");
	#endif

	lConn = (struct espconn *)pArg;
	lSlot = http_zoek_slot(lConn);
	espconn_disconnect(lConn);
	if (lSlot != NULL) {
		lSlot->sVrij = 1;
		lSlot->sConn = NULL;
	}
}

void httpd_disconnect_callback(void *arg) {
	//arg klopt niet, er is geen verbinding meer!
	int lTel;

	#ifdef PLATFORM_DEBUG
	ets_uart_printf("Verbinding verbroken, opruimen\r\n");
	#endif

	for (lTel = 0; lTel < HTTP_MAX_CONN; lTel++) {
		if (!mHttp_Conns[lTel].sVrij){
			if (mHttp_Conns[lTel].sConn->state == ESPCONN_NONE || mHttp_Conns[lTel].sConn->state == ESPCONN_CLOSE){
				#ifdef PLATFORM_DEBUG
				ets_uart_printf("Connectie %d vrijgeven\r\n", lTel);
				#endif
				mHttp_Conns[lTel].sVrij = true;
				mHttp_Conns[lTel].sConn = NULL;
			}
		}
	}
}

void http_connect_callback(void *pArg) {
	struct espconn *lConn;
	struct HttpConnectionSlot *lSlot;

	#ifdef PLATFORM_DEBUG
	ets_uart_printf("TCP/IP connectie gemaakt\r\n");
	#endif

	lConn=(struct espconn *)pArg;
	lSlot = http_zoek_vrij_slot();
	if (lSlot == NULL) {
		espconn_disconnect(lConn);
		return;
	}
	lSlot->sVrij = 0;
	lSlot->sRemoteIp[0] = lConn->proto.tcp->remote_ip[0];
	lSlot->sRemoteIp[1] = lConn->proto.tcp->remote_ip[1];
	lSlot->sRemoteIp[2] = lConn->proto.tcp->remote_ip[2];
	lSlot->sRemoteIp[3] = lConn->proto.tcp->remote_ip[3];
	lSlot->sRemotePort = lConn->proto.tcp->remote_port;
	lSlot->sConn = lConn;
	lSlot->sBufferLen = 0;
	lSlot->sState = HTTP_STATE_INIT;
	os_strcpy(lSlot->sVerb, "");
	lSlot->sUri[0] = '\0';
	lSlot->sContentLen = 0;

	espconn_regist_recvcb(lConn, http_recv_callback);
	espconn_regist_sentcb(lConn, http_sent_callback);
	espconn_regist_disconcb(lConn, httpd_disconnect_callback);
}

void http_init() {
	static struct espconn lConn;
	static esp_tcp lTcp;
	unsigned int lTel;

	for (lTel=0; lTel<HTTP_MAX_CONN; lTel++) {
		mHttp_Conns[lTel].sVrij = 1;
		mHttp_Conns[lTel].sConn = NULL;
	}

	memset(&lConn,0,sizeof(struct espconn));
	memset(&lTcp,0,sizeof(esp_tcp));
	lConn.type=ESPCONN_TCP;
	lConn.state=ESPCONN_NONE;
	lTcp.local_port=HTTP_PORT;
	lConn.proto.tcp=&lTcp;

	espconn_regist_connectcb(&lConn, http_connect_callback);
	espconn_accept(&lConn);

	#ifdef PLATFORM_DEBUG
	ets_uart_printf("TCP/IP geinitialiseerd\r\n");
	#endif
}
