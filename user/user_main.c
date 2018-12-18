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

static bool mConnected;
static os_timer_t tmStartCounter;
static bool mDeferredStart;
static int mStartCounter;

#define SPI_FLASH_SIZE_MAP 2

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#else
#error "The flash map is not supported"
#endif

#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM                SYSTEM_PARTITION_CUSTOMER_BEGIN

uint32 priv_param_start_sec;

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
    { SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM,             SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR,          0x1000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

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

static void ICACHE_FLASH_ATTR tcbMainDeferredStart(void *arg){
	static int lTest;

	mStartCounter++;
	if (mDeferredStart){
		ets_uart_printf("Counting....%d\r\n", mStartCounter);
	} else {
		if (mConnected){
			lTest = (mStartCounter/10)*10;
			if (mStartCounter == lTest){
				ets_uart_printf("Counting....%d\r\n", mStartCounter);
			}
		} else {
			ets_uart_printf("Counting....%d\r\n", mStartCounter);
		}

	}

	if (mStartCounter == STARTPAUSE){
		mDeferredStart = false;
		ets_uart_printf("SDK version:%s\r\n", system_get_sdk_version());
		ets_uart_printf("Flash chip id: %x\r\n", spi_flash_get_id());
		switch (system_upgrade_userbin_check()){
		case UPGRADE_FW_BIN1:
			ets_uart_printf("Image %d\r\n", 1);
			break;
		case UPGRADE_FW_BIN2:
			ets_uart_printf("Image %d\r\n", 2);
			break;
		default:
			ets_uart_printf("Unknown image\r\n");
			break;
		}
		ets_uart_printf("Start configuration\r\n");
		system_os_post(0, EventStartSetup, 0);
	}
}

static void ICACHE_FLASH_ATTR tcbIpTimeOut(void *arg){
	int lCount;

	wifi_station_set_reconnect_policy(false);
	lCount = xSettingConnectFail();
	ets_uart_printf("Time-out connect Station: %d!\r\n", lCount);
}

void cbMainWifiEvent(System_Event_t *pEvent) {
	static struct ip_info lIpConfig;

	switch(pEvent->event) {
		case EVENT_STAMODE_CONNECTED:
			ets_uart_printf("Wifi connected\r\n");
			break;
		case EVENT_STAMODE_DISCONNECTED:
			ets_uart_printf("Wifi disconnected\r\n");
			mConnected = false;
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
//			ets_uart_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE\r\n");
			break;
		case EVENT_STAMODE_GOT_IP:
			wifi_get_ip_info(STATION_IF, &lIpConfig);
			os_timer_disarm(&tmIpTimeOut);
			ets_uart_printf("IP address acquired: " IPSTR "\r\n", IP2STR(&lIpConfig.ip));
			mConnected = true;
			xTimeInit();
			xSettingConnectOk();
			system_os_post(0, EventStartHttp, 0);
			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			ets_uart_printf("station: " MACSTR "join, AID = %d\r\n",
					MAC2STR(pEvent->event_info.sta_connected.mac),
					pEvent->event_info.sta_connected.aid);
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			ets_uart_printf("station: " MACSTR "leave, AID = %d\r\n",
			MAC2STR(pEvent->event_info.sta_disconnected.mac),
			pEvent->event_info.sta_disconnected.aid);
			break;
		case EVENT_SOFTAPMODE_PROBEREQRECVED:
//			ets_uart_printf("Event: EVENT_SOFTAPMODE_PROBEREQRECVED, MAC: " MACSTR "\r\n", MAC2STR(pEvent->event_info.ap_probereqrecved.mac));
			break;
		case EVENT_OPMODE_CHANGED:
//			ets_uart_printf("Event: EVENT_OPMODE_CHANGED\r\n");
			break;
		default:
//			ets_uart_printf("Unexpected event: %d\n", pEvent->event);
			break;
	}
}

void ICACHE_FLASH_ATTR cbMainSystemReady(){
	xSwitchInit();
	os_timer_disarm(&tmStartCounter);
	os_timer_setfn(&tmStartCounter, (os_timer_func_t *)tcbMainDeferredStart, (void *)0);
	os_timer_arm(&tmStartCounter, 1000, 1);
}

static void ICACHE_FLASH_ATTR sMainSetupWifiStMode(){
	struct station_config lStConfig;
	uint8 lMac[6];

	ets_uart_printf("Start setup station\r\n");
	if (xSettingMacAddrPres() == true){
		wifi_set_macaddr(STATION_IF, (char *)xSettingMacAddr());
	}

	wifi_set_opmode(STATION_MODE);
	wifi_station_set_reconnect_policy(true);
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&lStConfig))
	{
		os_memset(lStConfig.ssid, 0, sizeof(lStConfig.ssid));
		os_memset(lStConfig.password, 0, sizeof(lStConfig.password));
		os_strcpy(lStConfig.ssid, xSettingSsId());
		os_strcpy(lStConfig.password, xSettingPassword());
		lStConfig.bssid_set = 0;
		if(wifi_station_set_config(&lStConfig))
		{
			ets_uart_printf("Configuration finished: Station!\r\n");
			wifi_station_dhcpc_start();
			wifi_station_connect();
			os_timer_disarm(&tmIpTimeOut);
			os_timer_setfn(&tmIpTimeOut, (os_timer_func_t *)tcbIpTimeOut, (void *)0);
			os_timer_arm(&tmIpTimeOut, 30000, 0);
		} else {
			ets_uart_printf("Configuration failed: Station config set!\r\n");
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
			ets_uart_printf("Configuration finished: SoftAP!\r\n");
			mConnected = true;
			system_os_post(0, EventStartHttp, 0);
		} else {
			ets_uart_printf("Configuration failed: Softap config set!\r\n");
		}
	} else {
		ets_uart_printf("Configuration failed: Softap config get!\r\n");
	}
}

void ICACHE_FLASH_ATTR eMainSetup()
{
	xSettingInit();
	xLogInit();
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
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	mConnected = false;
	mStartCounter = 0;
	mDeferredStart = true;
	xEventInit();
	system_init_done_cb(&cbMainSystemReady);
}
