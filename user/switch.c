#include "switch.h"

#include <c_types.h>
#include <eagle_soc.h>
#include <gpio.h>
#include "user_config.h"
#include "auto_off.h"

static bool mSwitchStatus = false;
static int mSwitchPort;
static int mOn;
static int mOff;

void ICACHE_FLASH_ATTR sSwitchSet(bool pValue){
	mSwitchStatus = pValue;

	GPIO_OUTPUT_SET(mSwitchPort, mSwitchStatus ? mOn : mOff);
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
	if (xSettingSwitchModel() == 1){
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
		mSwitchPort = 2;
		mOn = 1;
		mOff = 0;
	} else {
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
		mSwitchPort = 0;
		mOn = 0;
		mOff = 1;
	}

	sSwitchSet(false);
}
