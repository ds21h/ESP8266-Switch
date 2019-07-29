#include "switch.h"

#include <c_types.h>
#include <eagle_soc.h>
#include <gpio.h>
#include "user_config.h"
#include "auto_off.h"

#define GPIO2	2
#define High	1
#define Low		0

static bool mSwitchStatus = false;

void ICACHE_FLASH_ATTR sSwitchSet(bool pValue){
	mSwitchStatus = pValue;

	GPIO_OUTPUT_SET(GPIO2, mSwitchStatus ? High : Low);
}

void ICACHE_FLASH_ATTR xSwitchOn(long pAutoOff){
	sSwitchSet(true);
	xInitOff(pAutoOff);
}

void ICACHE_FLASH_ATTR xSwitchOff(){
	sSwitchSet(false);
}

void ICACHE_FLASH_ATTR xSwitchFlip(){
	sSwitchSet(!mSwitchStatus);
	if (mSwitchStatus){
		xInitOff(-1);
	}
}

bool ICACHE_FLASH_ATTR xSwitchStatus(){
	return mSwitchStatus;
}

void ICACHE_FLASH_ATTR xSwitchInit(){
	// GPIO2: output for switch
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	sSwitchSet(false);
}
