/*
 * auto_off.c
 *
 *  Created on: 14 jul. 2019
 *      Author: Jan
 */
#include <osapi.h>
#include <os_type.h>
#include "auto_off.h"
#include "switch.h"
#include "logger.h"
#include "setting.h"

static os_timer_t tmCheckOff;

static uint32 mTimeOn = 0;
static long mAutoOff = -1;

static void ICACHE_FLASH_ATTR tcbCheckOff(void *arg){
	uint32 lAutoOff;

	if (xSwitchStatus()){
		mTimeOn++;
		if (mAutoOff < 0){
			lAutoOff = xSettingAutoOff();
		} else {
			lAutoOff = mAutoOff;
		}
		if (mTimeOn >= lAutoOff && lAutoOff > 0){
			ets_uart_printf("Expired, switch off\r\n");
			xSwitchOff();
			xLogEntry(LOG_SWITCH_AUTO_OFF, 0);
		}
	} else {
		mTimeOn = 0;
		os_timer_disarm(&tmCheckOff);
	}
}

void ICACHE_FLASH_ATTR xInitOff(long pAutoOff){
	mAutoOff = pAutoOff;
	mTimeOn = 0;
	os_timer_disarm(&tmCheckOff);
	os_timer_setfn(&tmCheckOff, (os_timer_func_t *)tcbCheckOff, (void *)0);
	os_timer_arm(&tmCheckOff, 1000, 1);
}

uint32 ICACHE_FLASH_ATTR xTimeOn(){
	return mTimeOn;
}
