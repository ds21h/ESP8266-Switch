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
static uint32 mTimeOnReal = 0;

static void ICACHE_FLASH_ATTR tcbCheckOff(void *arg){
	if (xSwitchStatus()){
		mTimeOn++;
		if (mTimeOn >= xSettingAutoOff() && xSettingAutoOff() > 0){
			ets_uart_printf("Expired, switch off\r\n");
			xSwitchSet(false);
			xLogEntry(LOG_SWITCH_AUTO_OFF, 0);
		}
	} else {
		mTimeOn = 0;
		os_timer_disarm(&tmCheckOff);
	}
}

void ICACHE_FLASH_ATTR xInitOff(){
	mTimeOn = 0;
	mTimeOnReal = xTimeNow();
	os_timer_disarm(&tmCheckOff);
	os_timer_setfn(&tmCheckOff, (os_timer_func_t *)tcbCheckOff, (void *)0);
	os_timer_arm(&tmCheckOff, 1000, 1);
}

uint32 ICACHE_FLASH_ATTR xTimeOn(){
	return mTimeOn;
}

uint32 ICACHE_FLASH_ATTR xTimeOnReal(){
	uint32 lTime;

	if (xSwitchStatus()){
		if (mTimeOnReal == 0){
			return 0;
		} else {
			lTime = xTimeNow();
			if (lTime == 0){
				return 0;
			} else {
				return lTime - mTimeOnReal;
			}
		}
	} else {
		return 0;
	}
}
