/*
 * logger.c
 *
 *  Created on: 7 jun. 2017
 *      Author: Jan
 */
#include <c_types.h>
#include <mem.h>
#include <osapi.h>
#include <spi_flash.h>
#include "user_config.h"
#include "setting.h"
#include "logger.h"
#include "x_time.h"

#define LOG_VERSION			100

struct log_entry{
	long sTime;
	uint8 sAction;
	uint32 sIp;
};

struct log{
	uint8 sHash;
	uint8 sVersion;
	uint8 sNumber;
	uint8 sCurrent;
	struct log_entry sEntry[LOG_NUMBER_ENTRIES];
};

static uint8 mLogNumber[] = {LOG_INIT, LOG_RESET, LOG_RESET_READERR, LOG_SET, LOG_GET_SWITCH, LOG_GET_SETTING, LOG_GET_ERROR, LOG_PUT_SWITCH_ON, LOG_PUT_SWITCH_OFF, LOG_PUT_SWITCH_FLIP, LOG_PUT_SWITCH_ERROR, LOG_PUT_BUTTON_ON, LOG_PUT_BUTTON_OFF, LOG_PUT_BUTTON_ERROR, LOG_PUT_SETTING, LOG_PUT_ERROR, LOG_VERB_ERROR};
static char* mLogText[] = {"Log Init", "Log Reset", "Log Read error", "Log Set", "GET Switch", "GET Setting", "GET Error", "PUT Switch On", "PUT Switch Off", "PUT Switch Flip", "PUT Switch Error", "PUT Button On", "PUT Button Off", "PUT Button Error", "PUT Setting", "PUT Error", "VERB Error"};
static char mDef[3];

static struct log *mLog = NULL;

static uint8 mLogLevel = 0;

uint8 ICACHE_FLASH_ATTR xLogNumber(){
	return mLog->sNumber;
}

uint8 ICACHE_FLASH_ATTR xLogCurrent(){
	return mLog->sCurrent;
}

uint8 ICACHE_FLASH_ATTR xLogAction(uint8 pEntry){
	return mLog->sEntry[pEntry].sAction;
}

const char * ICACHE_FLASH_ATTR xLogActionText(uint8 pEntry){
	int lCount;
	char * lResult = NULL;

	for (lCount = 0; lCount < (sizeof(mLogNumber)/sizeof(uint8)); lCount++){
		if (mLogNumber[lCount] == mLog->sEntry[pEntry].sAction){
			lResult = mLogText[lCount];
			break;
		}
	}
	if (lResult == NULL){
		os_sprintf(mDef, "%d", mLog->sEntry[pEntry].sAction);
		lResult = mDef;
	}
	return lResult;
}

uint32 ICACHE_FLASH_ATTR xLogTime(uint8 pEntry){
	return mLog->sEntry[pEntry].sTime;
}

uint32 ICACHE_FLASH_ATTR xLogIp(uint8 pEntry){
	return mLog->sEntry[pEntry].sIp;
}

static uint8 ICACHE_FLASH_ATTR sLogHash(){
	uint8 lCount;
	uint8 lHash;
	char * lPos;

	lHash = 0;
	if (mLog != NULL){
		lPos = (char *)mLog + 1;
		for (lCount = 0; lCount < sizeof(struct log) - 1; lCount++){
			lHash += *lPos;
		}
	}

	return lHash;
}

static void ICACHE_FLASH_ATTR sLogWrite(){
	SpiFlashOpResult lResult;

	mLog->sHash = sLogHash();
	lResult = spi_flash_erase_sector(LOG_SECTOR);
    if (lResult == SPI_FLASH_RESULT_OK){
    	lResult = spi_flash_write(LOG_SECTOR * (4*1024), (uint32 *)mLog, sizeof(struct log));
    }
    if (lResult != SPI_FLASH_RESULT_OK){
    	mLogLevel = 1;
    }
}

void xLogEntry(uint8 pAction, uint32 pIp){
	if (mLogLevel > 0){
		mLog->sEntry[mLog->sCurrent].sAction = pAction;
		mLog->sEntry[mLog->sCurrent].sTime = xTimeNow();
		mLog->sEntry[mLog->sCurrent].sIp = pIp;
		mLog->sCurrent++;
		if (mLog->sCurrent >= mLog->sNumber){
			mLog->sCurrent = 0;
		}
		if (mLogLevel > 1){
			sLogWrite();
		}
	}
}

static void ICACHE_FLASH_ATTR sLogInit(){
    os_bzero(mLog, sizeof(mLog));
    mLog->sVersion = LOG_VERSION;
    mLog->sNumber = LOG_NUMBER_ENTRIES;
    mLog->sCurrent = 0;
}

static void ICACHE_FLASH_ATTR sLogRead(){
	SpiFlashOpResult lResult;

    lResult = spi_flash_read(LOG_SECTOR * (4*1024), (uint32 *)mLog, sizeof(struct log));
    if (lResult == SPI_FLASH_RESULT_OK){
    	if (mLog->sVersion == LOG_VERSION && mLog->sNumber == LOG_NUMBER_ENTRIES && mLog->sHash == sLogHash()){
    		if (mLog->sCurrent >= mLog->sNumber){
    			mLog->sCurrent = 0;
    		}
    		xLogEntry(LOG_INIT, mLogLevel);
    	} else {
        	sLogInit();
    		xLogEntry(LOG_RESET, mLogLevel);
    	}
    } else {
    	mLogLevel = 1;
    	sLogInit();
		xLogEntry(LOG_RESET_READERR, mLogLevel);
    }
}

void ICACHE_FLASH_ATTR xLogSetLevel(){
	uint8 lLogLevel;

	lLogLevel = xSettingLogLevel();
	if (mLogLevel != lLogLevel){
		if (lLogLevel <= 2){
			if (mLogLevel == 0){
				mLog = (struct log *)os_zalloc(sizeof (struct log));
				if (mLog == NULL){
					mLogLevel = 0;
				} else {
					mLogLevel = lLogLevel;
					sLogInit();
				}
			} else {
				mLogLevel = lLogLevel;
				if (lLogLevel == 0){
					os_free(mLog);
					mLog = NULL;
				}
			}
		}
	}
	xLogEntry(LOG_SET, mLogLevel);
}

void ICACHE_FLASH_ATTR xLogInit(){
	uint8 lLogLevel;

	lLogLevel = xSettingLogLevel();
	if (lLogLevel > 2){
		mLogLevel = 0;
	} else {
		mLogLevel = lLogLevel;
	}

	if (mLogLevel > 0){
		mLog = (struct log *)os_zalloc(sizeof (struct log));
		if (mLog == NULL){
			mLogLevel = 0;
		} else {
			if (mLogLevel > 1){
				sLogRead();
			} else {
				sLogInit();
	    		xLogEntry(LOG_INIT, mLogLevel);
			}
		}
	}
}

