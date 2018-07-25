/*
 * drukknop.c
 *
 *  Created on: 28 apr. 2017
 *      Author: Jan
 */
#include "user_config.h"
#include <ets_sys.h>
#include <os_type.h>
#include <gpio.h>
#include <osapi.h>
#include <spi_flash.h>
#include "switch.h"
#include "logger.h"

#define POLL_INTERVAL	100
#define DEBOUNCE_MAX	10
#define DEBOUNCE_MIN	5

static void ICACHE_FLASH_ATTR tcbButtonDebounceTest();
static void ICACHE_FLASH_ATTR tcbButtonPoll();

static os_timer_t tmButtonPoll;
static os_timer_t tmButtonDebounce;
static int mDebounceCount = DEBOUNCE_MAX;
static int mDebounceHit = 0;
static int mLastStatus = -1;

static void ICACHE_FLASH_ATTR sButtonOn(int pDelay){
	os_timer_disarm(&tmButtonDebounce);
	os_timer_disarm(&tmButtonPoll);
	os_timer_setfn(&tmButtonPoll, (os_timer_func_t *)tcbButtonPoll, (void *)0);
	os_timer_arm(&tmButtonPoll, pDelay, 0);
}

static void ICACHE_FLASH_ATTR sButtonDebounce(){
	os_timer_disarm(&tmButtonPoll);
	os_timer_disarm(&tmButtonDebounce);
	os_timer_setfn(&tmButtonDebounce, (os_timer_func_t *)tcbButtonDebounceTest, (void *)0);
	os_timer_arm(&tmButtonDebounce, 1, 0);
}

static void ICACHE_FLASH_ATTR tcbButtonDebounceTest(){
	int lStatus;
	bool lReady;

	lReady = false;
	if (mDebounceCount < DEBOUNCE_MAX){
		mDebounceCount ++;
		lStatus = GPIO_INPUT_GET(0);
		if (lStatus == 0){
			mDebounceHit++;
			if (mDebounceHit > DEBOUNCE_MIN){
				xSwitchFlip();
				xLogEntry(LOG_PUT_SWITCH_FLIP, 0);
				lReady = true;
			}
		} else {
			mDebounceHit = 0;
		}
	} else {
		lReady = true;
	}

	if (lReady){
		mDebounceCount = DEBOUNCE_MAX;
		mDebounceHit = 0;
		sButtonOn(POLL_INTERVAL);
	} else {
		sButtonDebounce();
	}
}

static void ICACHE_FLASH_ATTR tcbButtonPoll(){
	int lStatus;

	lStatus = GPIO_INPUT_GET(0);
	if (lStatus == mLastStatus){
		sButtonOn(POLL_INTERVAL);
	} else {
		mLastStatus = lStatus;
		if (lStatus == 0){
			mDebounceCount = 0;
			mDebounceHit = 0;
			sButtonDebounce();
		} else {
			sButtonOn(POLL_INTERVAL);
		}
	}
}

void ICACHE_FLASH_ATTR xButtonSet(){
	os_timer_disarm(&tmButtonPoll);
	os_timer_disarm(&tmButtonDebounce);
	if (xSettingButton()){
		sButtonOn(POLL_INTERVAL);
	}
}

void ICACHE_FLASH_ATTR xButtonInit(){
	ETS_GPIO_INTR_DISABLE();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_DIS_OUTPUT(0);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	if (xSettingButton()){
		sButtonOn(POLL_INTERVAL);
	}
}
