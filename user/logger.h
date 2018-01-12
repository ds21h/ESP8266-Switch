/*
 * logger.h
 *
 *  Created on: 9 jun. 2017
 *      Author: Jan
 */

#ifndef USER_LOGGER_H_
#define USER_LOGGER_H_

#define LOGGER_AANTAL_ENTRIES	250

#define LOGGER_AKTIE_NULL		0
#define LOGGER_AKTIE_RESET		1
#define LOGGER_AKTIE_INIT		2
#define LOGGER_WIFI_VERBONDEN	5
#define LOGGER_WIFI_VERBROKEN	6
#define LOGGER_IP_ONTVANGEN		7
#define LOGGER_GET_GROOT		10
#define LOGGER_GET_NORMAAL		11
#define LOGGER_GET_LOG			12
#define LOGGER_GET_DRUKKNOP		13
#define LOGGER_GET_FOUT			19
#define LOGGER_PUT_AAN			20
#define LOGGER_PUT_UIT			21
#define LOGGER_PUT_WISSEL		22
#define LOGGER_PUT_DRUKKNOP_AAN	25
#define LOGGER_PUT_DRUKKNOP_UIT	26
#define LOGGER_PUT_FOUT			29
#define LOGGER_VERB_FOUT		99
#define LOGGER_AKTIE_FOUT		255

void xLogInit();
uint8 xLogAantal();
uint8 xLogHuidig();
uint8 xLogAktie(uint8 pPos);
long xLogTijd(uint8 pPos);
uint32 xLogIp(uint8 pPos);

#endif /* USER_LOGGER_H_ */
