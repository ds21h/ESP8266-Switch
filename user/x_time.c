/*
 * tijd.c
 *
 *  Created on: 11 jun. 2017
 *      Author: Jan
 */
#include "x_time.h"

#include <osapi.h>
#include <c_types.h>
#include "u_time.h"
#include <sntp.h>

void ICACHE_FLASH_ATTR xTimeString(uint32 pTime, char * pResult, uint8 pLength){
	struct tmElements lTime;
	time_t lTimeIn;

	if (pLength < 20){
		pResult = "";
	} else {
		lTimeIn = pTime;
		timet_to_tm(pTime, &lTime);
		os_sprintf(pResult, "%04d-%02d-%02d %02d:%02d:%02d",lTime.Year + 1970, lTime.Month, lTime.Day, lTime.Hour, lTime.Minute, lTime.Second);
	}
}

uint32 ICACHE_FLASH_ATTR xTimeNow(){
	uint32 lTijd;

	lTijd = sntp_get_current_timestamp();

	return lTijd;
}

void ICACHE_FLASH_ATTR xTimeInit(){
	sntp_stop();

	sntp_setservername(0, "0.nl.pool.ntp.org");
	sntp_setservername(1, "1.nl.pool.ntp.org");
	sntp_setservername(2, "2.nl.pool.ntp.org");
	sntp_set_timezone(0);
	sntp_init();
}


