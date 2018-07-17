/*
 * setting.c
 *
 *  Created on: 1 jan. 2018
 *      Author: Jan
 */
#include <c_types.h>
#include <osapi.h>
#include "setting.h"
#include "user_interface.h"
#include "user_config.h"

#define SETTING_VERSION		100

struct setting_block{
	int sHash;
	int sFail;
	struct setting sSetting;
};

static struct setting_block mSetting;

static int ICACHE_FLASH_ATTR sSettingHash(){
	int lHash;
	uint8 *lSettingPos;
	int lCount;

	lSettingPos = (uint8*)&mSetting.sSetting;
	lHash = 0;
	for (lCount = 0; lCount < sizeof(struct setting); lCount++){
		lHash += *lSettingPos;
		lSettingPos++;
	}
	return lHash;
}

static void ICACHE_FLASH_ATTR sSettingReset(){
	os_memset(&mSetting, '\0', sizeof(struct setting_block));
	mSetting.sFail = 0;
	mSetting.sSetting.sVersion = SETTING_VERSION;
	mSetting.sSetting.sLogLevel = 1;
	mSetting.sSetting.sButton = true;
}

static void ICACHE_FLASH_ATTR sSettingWrite(){
	SpiFlashOpResult lResult;

	mSetting.sHash = sSettingHash();
	lResult = spi_flash_erase_sector(SETTING_SECTOR);
    if (lResult == SPI_FLASH_RESULT_OK){
    	lResult = spi_flash_write(SETTING_SECTOR * (4*1024), (uint32 *)&mSetting, sizeof(struct setting_block));
    }
    if (lResult != SPI_FLASH_RESULT_OK){
    	sSettingReset();
    }
}

static void ICACHE_FLASH_ATTR sSettingRead(){
	SpiFlashOpResult lResult;
	int lHash;

    lResult = spi_flash_read(SETTING_SECTOR * (4*1024), (uint32 *)&mSetting, sizeof(struct setting_block));
    if (lResult != SPI_FLASH_RESULT_OK){
		#ifdef PLATFORM_DEBUG
    	ets_uart_printf("Read setting failed!\r\n");
		#endif
    	sSettingReset();
    } else {
        lHash = sSettingHash();
        if (lHash == mSetting.sHash){
        	if (mSetting.sSetting.sVersion != SETTING_VERSION){
        		sSettingReset();
        	}
        } else {
        	sSettingReset();
        }
    }
}

void ICACHE_FLASH_ATTR xSettingConnectOk(){
	if (mSetting.sFail != 0){
		mSetting.sFail = 0;
		sSettingWrite();
	}
}

int ICACHE_FLASH_ATTR xSettingConnectFail(){
	int lCount;

	mSetting.sFail++;
	if (mSetting.sFail > 4){
		sSettingReset();
		lCount = 9999;
	} else {
		lCount = mSetting.sFail;
	}
	sSettingWrite();
	return lCount;
}

const char* ICACHE_FLASH_ATTR xSettingSsId(){
	return mSetting.sSetting.sSsId;
}

const char* ICACHE_FLASH_ATTR xSettingPassword(){
	return mSetting.sSetting.sPassword;
}

const char* ICACHE_FLASH_ATTR xSettingName(){
	return mSetting.sSetting.sName;
}

const char* ICACHE_FLASH_ATTR xSettingDescription(){
	return mSetting.sSetting.sDescription;
}

const char* ICACHE_FLASH_ATTR xSettingMacAddr(){
	return mSetting.sSetting.sMac;
}

void ICACHE_FLASH_ATTR xSettingMac(char *pMac){
	os_sprintf(pMac, MACSTR, MAC2STR(mSetting.sSetting.sMac));
}

uint8 ICACHE_FLASH_ATTR xSettingLogLevel(){
	return mSetting.sSetting.sLogLevel;
}

bool ICACHE_FLASH_ATTR xSettingButton(){
	return mSetting.sSetting.sButton;
}

void ICACHE_FLASH_ATTR xSettingButtonSet(bool pButton){
	mSetting.sSetting.sButton = pButton;
	sSettingWrite();
}

void ICACHE_FLASH_ATTR xSettingCopy(struct setting *pSetting){
	os_memcpy(pSetting, &mSetting.sSetting, sizeof(struct setting));
}

void ICACHE_FLASH_ATTR xSettingSave(struct setting *pSetting){
	os_memcpy(&mSetting.sSetting, pSetting, sizeof(struct setting));
	sSettingWrite();
}

void ICACHE_FLASH_ATTR xSettingReset(){
	sSettingReset();
	sSettingWrite();
}

void ICACHE_FLASH_ATTR xSettingInit(){
	sSettingRead();
}


