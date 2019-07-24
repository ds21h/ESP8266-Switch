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

static void ICACHE_FLASH_ATTR tcbCheckOff(void *arg){
	if (xSwitchStatus()){
		if (mTimeOn < xSettingAutoOff()){
			mTimeOn++;
			os_timer_disarm(&tmCheckOff);
			os_timer_setfn(&tmCheckOff, (os_timer_func_t *)tcbCheckOff, (void *)0);
			os_timer_arm(&tmCheckOff, 1000, 0);
		} else {
			ets_uart_printf("Expired, switch off\r\n");
			xSwitchSet(false);
			xLogEntry(LOG_SWITCH_AUTO_OFF, 0);
		}
	}
}

void ICACHE_FLASH_ATTR xInitOff(){
	mTimeOn = 0;
	os_timer_disarm(&tmCheckOff);
	os_timer_setfn(&tmCheckOff, (os_timer_func_t *)tcbCheckOff, (void *)0);
	os_timer_arm(&tmCheckOff, 1000, 0);
}

