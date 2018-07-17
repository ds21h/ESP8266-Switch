/*
 * user_main.c
 *
 *  Created on: 18 feb. 2017
 *      Author: Jan
 */
#include <ets_sys.h>
#include <osapi.h>
#include "user_config.h"
#include "user_interface.h"
#include <os_type.h>
#include <gpio.h>
#include "driver/uart.h"
#include <espconn.h>
#include "events.h"
#include "http_connection.h"
#include "switch.h"
#include "setting.h"
#include "logger.h"

static void ICACHE_FLASH_ATTR sMainSetupWifiApMode();

static os_timer_t tmIpTimeOut;

#ifdef PLATFORM_DEBUG
static bool mConnected;
static os_timer_t tmStartCounter;
static bool mDeferredStart;
static int mStartCounter;
#endif

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABBBCDDD
 *                A : rf cal
 *                B : at parameters
 *                C : rf init data
 *                D : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set()
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init()
{
}

#ifdef PLATFORM_DEBUG
static void ICACHE_FLASH_ATTR tcbMainDeferredStart(void *arg){
	static int lTest;

	  mStartCounter++;
	if (mDeferredStart){
		ets_uart_printf("Counting....%d\r\n", mStartCounter);
	} else {
		if (mConnected){
			lTest = (mStartCounter/10)*10;
			if (mStartCounter == lTest){
				ets_uart_printf("Still counting....%d\r\n", mStartCounter);
			}
		} else {
			ets_uart_printf("Still counting....%d\r\n", mStartCounter);
		}

	}

	if (mStartCounter == 10){
		mDeferredStart = false;
		ets_uart_printf("SDK version:%s\r\n", system_get_sdk_version());
		ets_uart_printf("Flash chip id: %x\r\n", spi_flash_get_id());
		ets_uart_printf("Start configuration\r\n");
		system_os_post(0, EventStartSetup, 0);
	}
}
#endif

static void ICACHE_FLASH_ATTR tcbIpTimeOut(void *arg){
	int lCount;

	wifi_station_set_reconnect_policy(false);
	lCount = xSettingConnectFail();
	#ifdef PLATFORM_DEBUG
	ets_uart_printf("Time-out connect Station: %d!\r\n", lCount);
	#endif
}

void cbMainWifiEvent(System_Event_t *pEvent) {
	static struct ip_info lIpConfig;
//	static struct ip_addr lAddress;

	switch(pEvent->event) {
		case EVENT_STAMODE_CONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Wifi connected\r\n");
			#endif
			break;
		case EVENT_STAMODE_DISCONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Wifi disconnected\r\n");
			mConnected = false;
			#endif
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE\r\n");
			#endif
			break;
		case EVENT_STAMODE_GOT_IP:
			wifi_get_ip_info(STATION_IF, &lIpConfig);
			os_timer_disarm(&tmIpTimeOut);
			#ifdef PLATFORM_DEBUG
//			lAddress.addr = lIpConfig.ip.addr;
			ets_uart_printf("IP address acquired: " IPSTR "\r\n", IP2STR(&lIpConfig.ip));
			mConnected = true;
			#endif
			xTimeInit();
			xSettingConnectOk();
			system_os_post(0, EventStartHttp, 0);
			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("station: " MACSTR "join, AID = %d\r\n",
					MAC2STR(pEvent->event_info.sta_connected.mac),
					pEvent->event_info.sta_connected.aid);
			#endif
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("station: " MACSTR "leave, AID = %d\r\n",
			MAC2STR(pEvent->event_info.sta_disconnected.mac),
			pEvent->event_info.sta_disconnected.aid);
			#endif
			break;
		case EVENT_SOFTAPMODE_PROBEREQRECVED:
//			#ifdef PLATFORM_DEBUG
//			ets_uart_printf("Event: EVENT_SOFTAPMODE_PROBEREQRECVED, MAC: " MACSTR "\r\n", MAC2STR(pEvent->event_info.ap_probereqrecved.mac));
//			#endif
			break;
		case EVENT_OPMODE_CHANGED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_OPMODE_CHANGED\r\n");
			#endif
			break;
		default:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Unexpected event: %d\n", pEvent->event);
			#endif
			break;
	}
}

void ICACHE_FLASH_ATTR cbMainSystemReady(){
	#ifdef PLATFORM_DEBUG
	os_timer_disarm(&tmStartCounter);
	os_timer_setfn(&tmStartCounter, (os_timer_func_t *)tcbMainDeferredStart, (void *)0);
	os_timer_arm(&tmStartCounter, 1000, 1);
	#else
	system_os_post(0, EventStartSetup, 0);
	#endif
}

static void ICACHE_FLASH_ATTR sMainSetupWifiStMode(){
	struct station_config stconfig;

	wifi_set_macaddr(STATION_IF, (char *)xSettingMacAddr());
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_reconnect_policy(true);
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
	{
		os_memcpy(stconfig.ssid, xSettingSsId(), sizeof(stconfig.ssid));
		os_memcpy(stconfig.password, xSettingPassword(), sizeof(stconfig.password));
		stconfig.bssid_set = 0;
		if(wifi_station_set_config(&stconfig))
		{
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Configuration finished: Station!\r\n");
			#endif
			wifi_station_dhcpc_start();
			wifi_station_connect();
			os_timer_disarm(&tmIpTimeOut);
			os_timer_setfn(&tmIpTimeOut, (os_timer_func_t *)tcbIpTimeOut, (void *)0);
			os_timer_arm(&tmIpTimeOut, 30000, 0);
		} else {
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Configuration failed: Station config set!\r\n");
			#endif
		}
	}
}

static void ICACHE_FLASH_ATTR sMainSetupWifiApMode()
{
	struct softap_config lApConfig;
	char lMac[6];

	wifi_set_opmode(SOFTAP_MODE);
	if(wifi_softap_get_config(&lApConfig)){
		os_memset(lApConfig.ssid, 0, sizeof(lApConfig.ssid));
		os_memset(lApConfig.password, 0, sizeof(lApConfig.password));
		wifi_get_macaddr(SOFTAP_IF, lMac);
		os_sprintf(lApConfig.ssid, "EspSw_%02x%02x%02x%02x%02x%02x", MAC2STR(lMac));
		os_sprintf(lApConfig.password, "%s", "EspSwSetup");
		lApConfig.ssid_len = 0;
		lApConfig.authmode = AUTH_WPA_WPA2_PSK;
		if(wifi_softap_set_config(&lApConfig)){
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Configuration finished: SoftAP!\r\n");
			mConnected = true;
			#endif
			system_os_post(0, EventStartHttp, 0);
		} else {
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Configuration failed: Softap config set!\r\n");
			#endif
		}
	} else {
		#ifdef PLATFORM_DEBUG
		ets_uart_printf("Configuration failed: Softap config get!\r\n");
		#endif
	}
}

void ICACHE_FLASH_ATTR eMainSetup()
{
	xSettingInit();
	xLogInit();
	xSwitchInit();
	xButtonInit();
	if(wifi_station_get_auto_connect() == 1){
		wifi_station_set_auto_connect(0);
	}
	if(wifi_get_phy_mode() != PHY_MODE_11N){
		wifi_set_phy_mode(PHY_MODE_11N);
	}
	wifi_set_event_handler_cb(cbMainWifiEvent);
	if (os_strlen(xSettingSsId()) > 0) {
		sMainSetupWifiStMode();
	} else {
		sMainSetupWifiApMode();
	}
}

void ICACHE_FLASH_ATTR user_init()
{
#ifdef PLATFORM_DEBUG
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	mConnected = false;
	mStartCounter = 0;
	mDeferredStart = true;
#endif
	xEventInit();
	system_init_done_cb(&cbMainSystemReady);
}
