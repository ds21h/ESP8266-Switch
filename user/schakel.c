#include <c_types.h>
#include <eagle_soc.h>
#include <gpio.h>
#include "schakel.h"
#include "user_config.h"

#define GPIO2	2
#define High	1
#define Low		0

static bool mSchakelStatus = false;

void ICACHE_FLASH_ATTR xSchakelZet(bool pWaarde){
	mSchakelStatus = pWaarde;
	if (mSchakelStatus){
		GPIO_OUTPUT_SET(GPIO2, High);
	} else {
		GPIO_OUTPUT_SET(GPIO2, Low);
	}
}

void ICACHE_FLASH_ATTR xSchakelWissel(){
	xSchakelZet(!mSchakelStatus);
}

bool ICACHE_FLASH_ATTR xSchakelStatus(){
	return mSchakelStatus;
}

void ICACHE_FLASH_ATTR xSchakelInit(){
	// GPIO2: output voor schakelaar
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	xSchakelZet(false);
}
