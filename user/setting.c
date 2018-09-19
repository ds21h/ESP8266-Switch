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
	int sVersion;
	int sHash;
	int sFail;
	struct setting sSetting;
};

static struct setting_block mSetting;

static int ICACHE_FLASH_ATTR sSettingHash(struct setting *pSetting, int pLength){
	int lHash;
	uint8 *lSettingPos;
	int lCount;

	lSettingPos = (uint8*)pSetting;
	lHash = 0;
	for (lCount = 0; lCount < pLength; lCount++){
		lHash += *lSettingPos;
		lSettingPos++;
	}
	return lHash;
}

static void ICACHE_FLASH_ATTR sSettingReset(){
	os_memset(&mSetting, '\0', sizeof(struct setting_block));
	mSetting.sFail = 0;
	mSetting.sVersion = SETTING_VERSION;
	mSetting.sSetting.sLogLevel = 1;
	mSetting.sSetting.sButton = true;
}

static void ICACHE_FLASH_ATTR sSettingWrite(){
	SpiFlashOpResult lResult;

    mSetting.sHash = sSettingHash(&mSetting.sSetting, sizeof(struct setting));
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
    	ets_uart_printf("Read setting failed!\r\n");
    	sSettingReset();
    } else {
    	if (mSetting.sVersion == SETTING_VERSION){
            lHash = sSettingHash(&mSetting.sSetting, sizeof(struct setting));
            if (lHash != mSetting.sHash){
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

bool ICACHE_FLASH_ATTR xSettingMacAddrPres(){
	int lTeller;

	for (lTeller = 0; lTeller < 6; lTeller++){
		if (mSetting.sSetting.sMac[lTeller] != 0){
			return true;
		}
	}
	return false;
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

void ICACHE_FLASH_ATTR xSettingServerIpDisp(char *pIp){
	os_sprintf(pIp, IPSTR, IP2STR(&mSetting.sSetting.sServerIP));
}

const char* ICACHE_FLASH_ATTR xSettingServerIp(){
	return mSetting.sSetting.sServerIP;
}

int ICACHE_FLASH_ATTR xSettingServerPort(){
	return mSetting.sSetting.sServerPort;
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


