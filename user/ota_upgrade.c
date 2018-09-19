/*
 * ota_upgrade.c
 *
 *  Created on: 18 jul. 2018
 *      Author: Jan
 */
#include <osapi.h>
#include <user_interface.h>
#include <upgrade.h>
#include <mem.h>
#include "user_config.h"

bool gHttpActive;

char *mVersion;
struct upgrade_server_info *mUpgServer = NULL;
static os_timer_t tmStartUpgrade;
struct upgrade_server_info *upServer = NULL;

LOCAL void ICACHE_FLASH_ATTR cbUpgrade(void *arg){
	struct upgrade_server_info *lServer;

	lServer = (struct upgrade_server_info *)arg;
	if(lServer->upgrade_flag == true){
		system_upgrade_reboot();
	}

	os_free(mUpgServer->url);
	os_free(mUpgServer);
}

static void ICACHE_FLASH_ATTR tcbUpgradeStart(void *arg){
	int lImageBin = 0;

	os_timer_disarm(&tmStartUpgrade);

	mUpgServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));

	os_sprintf(mUpgServer->pre_version, VERSION);
	os_strcpy(mUpgServer->upgrade_version, mVersion);

	os_memcpy(mUpgServer->ip, (char *)xSettingServerIp(), 4);
	mUpgServer->port = xSettingServerPort();

	mUpgServer->check_cb = cbUpgrade;
	mUpgServer->check_times = 60000; // TimeOut for callback

	if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1){
		lImageBin = 2;
	} else if(system_upgrade_userbin_check() == UPGRADE_FW_BIN2){
		lImageBin = 1;
	}

	mUpgServer->url = (uint8 *) os_zalloc(512);
	os_sprintf(mUpgServer->url,
"GET /EspServer/Fota/Image/%d/%s HTTP/1.1\r\n"
"Host: " IPSTR ":%d\r\n"
"Connection: close\r\n"
"Accept: */*\r\n"
"Cache-Control: no-cache\r\n"
"\r\n", lImageBin, mVersion, IP2STR(mUpgServer->ip), mUpgServer->port);
//	"Accept-Encoding: gzip,deflate,sdch\r\n"

	if(system_upgrade_start(mUpgServer) == false){
		ets_uart_printf("Upgrade did not start!\r\n");
	} else {
		ets_uart_printf("Upgrade started!\r\n");
	}

}

void ICACHE_FLASH_ATTR eOtaUpgrade(char *pVersion){
	mVersion = pVersion;
	gHttpActive = false;

	ets_uart_printf("Start upgrade to version %s!\r\n", pVersion);

	os_timer_disarm(&tmStartUpgrade);
	os_timer_setfn(&tmStartUpgrade, (os_timer_func_t *)tcbUpgradeStart, (void *)0);
	os_timer_arm(&tmStartUpgrade, 5000, 1);
}

