/*
 * setting.h
 *
 *  Created on: 1 jan. 2018
 *      Author: Jan
 */

#ifndef USER_SETTING_H_
#define USER_SETTING_H_

#include "user_config.h"

struct setting{
	char sSsId[LEN_SSID];
	char sPassword[LEN_PASSWORD];
	uint8 sMac[6];
	char sName[64];
	char sDescription[128];
	uint8 sLogLevel;
	bool sButton;
	uint32 sAutoOff;
	uint8 sServerIP[4];
	int sServerPort;
};

void xSettingInit();

const char* xSettingSsId();
const char* xSettingPassword();
const char* xSettingName();
const char* xSettingDescription();
bool xSettingMacAddrPres();
const char* xSettingMacAddr();
void xSettingMac(char *pMac);
uint8 xSettingLogLevel();
bool xSettingButton();
void xSettingButtonSet(bool pButton);
uint32 xSettingAutoOff();

void xSettingCopy(struct setting *pSetting);
void xSettingSave(struct setting *pSetting);
void xSettingReset();
void xSettingConnectOk();
int xSettingConnectFail();

#endif /* USER_SETTING_H_ */
