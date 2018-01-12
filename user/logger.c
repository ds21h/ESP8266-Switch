/*
 * logger.c
 *
 *  Created on: 7 jun. 2017
 *      Author: Jan
 */
#include <c_types.h>
#include <ip_addr.h>
#include <spi_flash.h>
#include "user_config.h"
#include "logger.h"
#include "tijd.h"

#define LOGGER_AANTAL_SECTOR	4

struct log_entry{
	long sTijd;
	uint8 sAktie;
	uint32 sIp;
};

struct log{
	uint8 sAantal;
	uint8 sHuidig;
	uint8 sTotaal;
	struct log_entry sEntry[LOGGER_AANTAL_ENTRIES];
};

struct log mLog;

LOCAL bool mLogAktief = false;

uint8 ICACHE_FLASH_ATTR sLeesLog(){
	SpiFlashOpResult lResult;

    lResult = spi_flash_read(LOGGER_SECTOR * (4*1024), (uint32 *)&mLog, sizeof(struct log));
    if (lResult == SPI_FLASH_RESULT_OK){
    	return RESULT_OK;
    } else {
    	mLogAktief = false;
    	mLog.sAantal = 0;
    	return RESULT_FOUT;
    }
}

uint8 ICACHE_FLASH_ATTR sSchrijfLog(){
	SpiFlashOpResult lResult;

	lResult = spi_flash_erase_sector(LOGGER_SECTOR);
    if (lResult == SPI_FLASH_RESULT_OK){
    	lResult = spi_flash_write(LOGGER_SECTOR * (4*1024), (uint32 *)&mLog, sizeof(struct log));
    }
    if (lResult == SPI_FLASH_RESULT_OK){
    	return RESULT_OK;
    } else {
    	mLogAktief = false;
    	mLog.sAantal = 0;
    	return RESULT_FOUT;
    }
}

uint8 ICACHE_FLASH_ATTR sTestLog(){
	uint8 lTeller;
	uint8 lTotaal;
	uint8 lLogGoed;

	lLogGoed = RESULT_FOUT;
	if (mLog.sAantal == LOGGER_AANTAL_ENTRIES){
		if (mLog.sHuidig < LOGGER_AANTAL_ENTRIES){
			lTotaal = 0;
			for (lTeller = 0; lTeller < LOGGER_AANTAL_ENTRIES; lTeller++){
				lTotaal += mLog.sEntry[lTeller].sAktie;
			}
			if (lTotaal == mLog.sTotaal){
				lLogGoed = RESULT_OK;
			}
		}
	}

	return lLogGoed;
}

uint8 sLogEntry(uint8 pAktie, uint32 pIp){
	uint8 lTeller;
	uint8 lTotaal;
	uint8 lResult;

	mLog.sEntry[mLog.sHuidig].sAktie = pAktie;
	mLog.sEntry[mLog.sHuidig].sTijd = xTijdNu();
	mLog.sEntry[mLog.sHuidig].sIp = pIp;
	mLog.sHuidig++;
	if (mLog.sHuidig >= mLog.sAantal){
		mLog.sHuidig = 0;
	}

	lTotaal = 0;
	for (lTeller = 0; lTeller < mLog.sAantal; lTeller++){
		lTotaal += mLog.sEntry[lTeller].sAktie;
	}
	mLog.sTotaal = lTotaal;

	lResult = sSchrijfLog();

	return lResult;
}

uint8 ICACHE_FLASH_ATTR sInitLog(){
	uint8 lTeller;
	uint8 lResult;

	mLog.sAantal = LOGGER_AANTAL_ENTRIES;
	mLog.sHuidig = 0;
	for (lTeller = 0; lTeller < LOGGER_AANTAL_ENTRIES; lTeller++){
		mLog.sEntry[lTeller].sAktie = LOGGER_AKTIE_NULL;
		mLog.sEntry[lTeller].sTijd = 0;
		mLog.sEntry[lTeller].sIp = 0;
	}

	lResult = sLogEntry(LOGGER_AKTIE_RESET, 0);

	return lResult;
}

uint8 ICACHE_FLASH_ATTR xLogAantal(){
	if (mLogAktief){
		return mLog.sAantal;
	} else {
		return 0;
	}
}

uint8 ICACHE_FLASH_ATTR xLogHuidig(){
	if (mLogAktief){
		return mLog.sHuidig;
	} else {
		return 0;
	}
}

uint8 ICACHE_FLASH_ATTR xLogAktie(uint8 pPos){
	if (mLogAktief){
		if (pPos > LOGGER_AANTAL_ENTRIES){
			return LOGGER_AKTIE_FOUT;
		} else {
			return mLog.sEntry[pPos].sAktie;
		}
	} else {
		return LOGGER_AKTIE_NULL;
	}
}

long ICACHE_FLASH_ATTR xLogTijd(uint8 pPos){
	if (mLogAktief){
		if (pPos > LOGGER_AANTAL_ENTRIES){
			return -1;
		} else {
			return mLog.sEntry[pPos].sTijd;
		}
	} else {
		return 0;
	}
}

uint32 ICACHE_FLASH_ATTR xLogIp(uint8 pPos){
	if (mLogAktief){
		if (pPos > LOGGER_AANTAL_ENTRIES){
			return 0;
		} else {
			return mLog.sEntry[pPos].sIp;
		}
	} else {
		return 0;
	}
}

void xLogEntry(uint8 pAktie, uint32 pIp){
	if (mLogAktief){
		sLogEntry(pAktie, pIp);
	}
}

void ICACHE_FLASH_ATTR xLogInit(){
	uint8 lResult;

#ifdef PLATFORM_DEBUG
	ets_uart_printf("Grootte log: %d\r\n", sizeof(struct log));
#endif

	lResult = sLeesLog();
	if (lResult == RESULT_OK){
		lResult = sTestLog();
		if (lResult == RESULT_OK){
			mLogAktief = true;
		} else {
			lResult = sInitLog();
			if (lResult == RESULT_OK){
				mLogAktief = true;
			}
		}
	}
	xLogEntry(LOGGER_AKTIE_INIT, 0);
}
