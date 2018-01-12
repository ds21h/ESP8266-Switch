/*
 * tijd.c
 *
 *  Created on: 11 jun. 2017
 *      Author: Jan
 */
#include <osapi.h>
#include <c_types.h>
#include "u_time.h"
//#include <time.h>
#include <sntp.h>
#include "tijd.h"

bool lTijdAktief = false;

//static os_timer_t mRefresh_Timer;

void ICACHE_FLASH_ATTR xTijdString(uint32 pTijd, char * pResult, uint8 pLengte){
	struct tmElements lTijd;
	time_t lTijdIn;

	if (pLengte < 20){
		pResult = "";
	} else {
		lTijdIn = pTijd;
		timet_to_tm(pTijd, &lTijd);
		os_sprintf(pResult, "%04d-%02d-%02d %02d:%02d:%02d",lTijd.Year + 1970, lTijd.Month, lTijd.Day, lTijd.Hour, lTijd.Minute, lTijd.Second);
	}
}

uint32 ICACHE_FLASH_ATTR xTijdNu(){
	uint32 lTijd;

	if (lTijdAktief){
		lTijd = sntp_get_current_timestamp();
	} else {
		lTijd = 0;
	}

	return lTijd;
}

//LOCAL void ICACHE_FLASH_ATTR sRefreshTijd(void *arg){
//	sntp_stop();
//	sntp_init();
//}

void ICACHE_FLASH_ATTR xTijdInit(){
	if (lTijdAktief){
		sntp_stop();
	}
	sntp_setservername(0, "0.nl.pool.ntp.org");
	sntp_setservername(1, "1.nl.pool.ntp.org");
	sntp_setservername(2, "2.nl.pool.ntp.org");
	sntp_set_timezone(0);
	sntp_init();
	lTijdAktief = true;
//	os_timer_disarm(&mRefresh_Timer);
//	os_timer_setfn(&mRefresh_Timer, (os_timer_func_t *)sRefreshTijd, (void *)0);
//	os_timer_arm(&mRefresh_Timer, 3600000, 1);
}


