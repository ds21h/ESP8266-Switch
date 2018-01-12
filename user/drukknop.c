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
#include "schakel.h"
#include "logger.h"

#define KNOP_AAN	132

#define POLL_INTERVAL	100
#define DEBOUNCE_MAX	10
#define DEBOUNCE_MIN	5

void ICACHE_FLASH_ATTR sDrukKnopDebounceTest();
void ICACHE_FLASH_ATTR sDrukKnopPoll();

struct knopgeg{
	int sWaar;
	int sTotaal;
	int sDrukKnop;
};

struct knopgeg mGeg;

//static os_timer_t mKnopje_Timer;
static os_timer_t mKnopje_Poll;
static os_timer_t mKnopje_Debounce;
static int mDebounceTeller = DEBOUNCE_MAX;
static int mDebounceHit = 0;
static int mVorigeStatus = -1;
static bool mInit = false;
//static bool mKnopAktief;
//static bool mKnopAan;

/**
LOCAL void ICACHE_FLASH_ATTR sKnopjeAktief(void *arg){
	uint32 lGpioStatus;

	// Debounce timer voor de zekerheid uitzetten.
	os_timer_disarm(&mKnopje_Debounce);

//	if (!mKnopAktief){
		// interrupt status wissen
//		lGpioStatus = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
//		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, lGpioStatus);
//	}

	if (mGeg.sDrukKnop == mGeg.sWaar){
//		ETS_GPIO_INTR_ENABLE();
		os_timer_disarm(&mKnopje_Poll);
		os_timer_setfn(&mKnopje_Poll, (os_timer_func_t *)sDrukKnopPoll, (void *)0);
		os_timer_arm(&mKnopje_Poll, POLL_INTERVAL, 0);
		mKnopAktief = true;
	}
}
*/

void ICACHE_FLASH_ATTR sDrukKnopAan(int pPauze){
	os_timer_disarm(&mKnopje_Debounce);
	os_timer_disarm(&mKnopje_Poll);
	os_timer_setfn(&mKnopje_Poll, (os_timer_func_t *)sDrukKnopPoll, (void *)0);
	os_timer_arm(&mKnopje_Poll, pPauze, 0);
//	mKnopAktief = true;
}

void ICACHE_FLASH_ATTR sDrukKnopDebounce(){
	os_timer_disarm(&mKnopje_Poll);
	os_timer_disarm(&mKnopje_Debounce);
	os_timer_setfn(&mKnopje_Debounce, (os_timer_func_t *)sDrukKnopDebounceTest, (void *)0);
	os_timer_arm(&mKnopje_Debounce, 1, 0);
}

void ICACHE_FLASH_ATTR sDrukKnopDebounceTest(){
	int lWaarde;
	bool lKlaar;

	lKlaar = false;
	if (mDebounceTeller < DEBOUNCE_MAX){
		mDebounceTeller ++;
		lWaarde = GPIO_INPUT_GET(0);
		if (lWaarde == 0){
			mDebounceHit++;
			if (mDebounceHit > DEBOUNCE_MIN){
				xSchakelWissel();
				xLogEntry(LOGGER_PUT_WISSEL, 0);
				lKlaar = true;
			}
		} else {
			mDebounceHit = 0;
		}
	} else {
		lKlaar = true;
	}

	if (lKlaar){
		mDebounceTeller = DEBOUNCE_MAX;
		mDebounceHit = 0;
		sDrukKnopAan(POLL_INTERVAL);
	} else {
		sDrukKnopDebounce();
	}
}

/**
void sDrukKnop_cb(){
	ETS_GPIO_INTR_DISABLE();
	mKnopAktief = false;

	mDebounceTeller = 0;
	mDebounceHit = 0;
	sDrukKnopDebounce();

	sDrukKnopAan(1000);
}
*/

void ICACHE_FLASH_ATTR sDrukKnopPoll(){
	int lWaarde;

	lWaarde = GPIO_INPUT_GET(0);
	if (lWaarde == mVorigeStatus){
		sDrukKnopAan(POLL_INTERVAL);
	} else {
		mVorigeStatus = lWaarde;
		if (lWaarde == 0){
			mDebounceTeller = 0;
			mDebounceHit = 0;
			sDrukKnopDebounce();
		} else {
			sDrukKnopAan(POLL_INTERVAL);
		}
	}
}

//void ICACHE_FLASH_ATTR xDrukknopAan(){
//	if (mInit){
//		if (!mKnopAktief){
//		if (mGeg.sDrukKnop == mGeg.sWaar){
//			sDrukKnopAan(1000);
//		}
//		}
//	}
//}

//void ICACHE_FLASH_ATTR xDrukknopUit(){
//	if (mInit){
//		ETS_GPIO_INTR_DISABLE();
//		os_timer_disarm(&mKnopje_Poll);
//		os_timer_disarm(&mKnopje_Debounce);
//		mKnopAktief = false;

		// Na 1 minuut altijd weer aanzetten
//		if (mGeg.sDrukKnop == mGeg.sWaar){
//			sDrukKnopAan(60000);
//		}
//	}
//}

uint8 ICACHE_FLASH_ATTR sSchrijfGeg(){
	SpiFlashOpResult lResult;

	lResult = spi_flash_erase_sector(DRUKKNOP_SECTOR);
    if (lResult == SPI_FLASH_RESULT_OK){
    	lResult = spi_flash_write(DRUKKNOP_SECTOR * (4*1024), (uint32 *)&mGeg, sizeof(struct knopgeg));
    }
    if (lResult == SPI_FLASH_RESULT_OK){
    	return RESULT_OK;
    } else {
    	return RESULT_FOUT;
    }
}

void ICACHE_FLASH_ATTR sInitGeg(){
	mGeg.sWaar = KNOP_AAN;
	mGeg.sDrukKnop = mGeg.sWaar;
	mGeg.sTotaal = mGeg.sWaar + mGeg.sDrukKnop;
}

uint8 ICACHE_FLASH_ATTR sTestGeg(){
	uint8 lGegGoed;

	lGegGoed = RESULT_FOUT;
	if (mGeg.sWaar == KNOP_AAN){
		if (mGeg.sTotaal == mGeg.sWaar + mGeg.sDrukKnop){
			lGegGoed = RESULT_OK;
		}
	}

	return lGegGoed;
}

uint8 ICACHE_FLASH_ATTR sLeesGeg(){
		SpiFlashOpResult lResult;

	    lResult = spi_flash_read(DRUKKNOP_SECTOR * (4*1024), (uint32 *)&mGeg, sizeof(struct knopgeg));
	    if (lResult == SPI_FLASH_RESULT_OK){
	    	return RESULT_OK;
	    } else {
	    	mGeg.sWaar = KNOP_AAN;
	    	mGeg.sDrukKnop = 0;
	    	return RESULT_FOUT;
	    }
}

bool ICACHE_FLASH_ATTR xDrukknopStatus(){
	return (mGeg.sDrukKnop == mGeg.sWaar);
}

void ICACHE_FLASH_ATTR xDrukknopAktief(){
	if (mGeg.sDrukKnop != mGeg.sWaar){
		mGeg.sDrukKnop = mGeg.sWaar;
		mGeg.sTotaal = mGeg.sDrukKnop + mGeg.sWaar;
		sSchrijfGeg();
		sDrukKnopAan(1000);
	}
}

void ICACHE_FLASH_ATTR xDrukknopInAktief(){
	if (mGeg.sDrukKnop == mGeg.sWaar){
		os_timer_disarm(&mKnopje_Poll);
		os_timer_disarm(&mKnopje_Debounce);
		mGeg.sDrukKnop = 0;
		mGeg.sTotaal = mGeg.sDrukKnop + mGeg.sWaar;
		sSchrijfGeg();
//		ETS_GPIO_INTR_DISABLE();
//		mKnopAktief = false;
	}
}

void ICACHE_FLASH_ATTR xDrukknopInit(){
	int lResult;

	if (!mInit){
		lResult = sLeesGeg();
		if (lResult == RESULT_OK){
			lResult = sTestGeg();
			if (lResult != RESULT_OK){
				sInitGeg();
				sSchrijfGeg();
			}
		} else {
			sInitGeg();
			sSchrijfGeg();
		}
		// GPIO0: input voor knopje
		ETS_GPIO_INTR_DISABLE();
//		ETS_GPIO_INTR_ATTACH(sDrukKnop_cb, 0);  // Zet callback op GPIO0
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); // Zet functie
		GPIO_DIS_OUTPUT(0); // Zet op input
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
//		gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_NEGEDGE); // Zet juiste interupt
		if (mGeg.sDrukKnop == mGeg.sWaar){
			sDrukKnopAan(POLL_INTERVAL);
//			ETS_GPIO_INTR_ENABLE();
//			mKnopAktief = true;
		}
		mInit = true;
	}
}
