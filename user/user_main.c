/*
 * user_main.c
 *
 *  Created on: 18 feb. 2017
 *      Author: Jan
 */
#include <ets_sys.h>
#include <osapi.h>
#include "user_config.h"
#include <user_interface.h>
#include <os_type.h>
#include <gpio.h>
#include "driver/uart.h"
#include <espconn.h>
#include "http_verbinding.h"
#include "drukknop.h"
#include "logger.h"
#include "tijd.h"


void ICACHE_FLASH_ATTR setup_wifi(void);
void ICACHE_FLASH_ATTR setup_wifi_st_mode(void);

static bool mStart = true;
static bool mVerbonden;
#ifdef PLATFORM_DEBUG
	static os_timer_t mTeller_Timer;
	static bool mAanloop;
	static int mVolgnr;
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
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
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

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

#ifdef PLATFORM_DEBUG
LOCAL void ICACHE_FLASH_ATTR aftellen(void *arg){
	LOCAL int lTest;
	uint32_t lFlashChipId;
	uint8_t lFabr;
	uint8_t lType;
	uint8_t lFormaat;

	  mVolgnr++;
	if (mAanloop){
		ets_uart_printf("Wakker worden....%d\r\n", mVolgnr);
	} else {
		if (mVerbonden){
			lTest = (mVolgnr/10)*10;
			if (mVolgnr == lTest){
				ets_uart_printf("Doortellen....%d\r\n", mVolgnr);
			}
		} else {
			ets_uart_printf("Doortellen....%d\r\n", mVolgnr);
		}

	}

	if (mVolgnr == 10){
		mAanloop = false;
		ets_uart_printf("SDK versie:%s\r\n", system_get_sdk_version());
		lFlashChipId = spi_flash_get_id();
		lFabr = lFlashChipId & 0xff;
		lType = (lFlashChipId >> 8) & 0xff;
		lFormaat = (lFlashChipId >> 16) & 0xff;
		ets_uart_printf("Flash chip id: %x, fabr: %x\r\n", lFlashChipId, lFabr);
		xLogInit();
		ets_uart_printf("Start configuratie\r\n");
		setup_wifi();
	}
}
#endif

static void ICACHE_FLASH_ATTR systeem_klaar(void){
	#ifdef PLATFORM_DEBUG
	os_timer_disarm(&mTeller_Timer);
	os_timer_setfn(&mTeller_Timer, (os_timer_func_t *)aftellen, (void *)0);
	os_timer_arm(&mTeller_Timer, 1000, 1);
	#else
	setup_wifi();
	#endif
}

void ICACHE_FLASH_ATTR setup_wifi(void)
{
	setup_wifi_st_mode();
	wifi_station_connect();
	wifi_station_dhcpc_start();
	if(wifi_station_get_auto_connect() == 1)
		wifi_station_set_auto_connect(0);
}

void ICACHE_FLASH_ATTR setup_wifi_st_mode(void)
{
	struct station_config stconfig;

	wifi_set_opmode(STATION_MODE);
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
	{
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		os_sprintf(stconfig.ssid, "%s", WIFI_SSID);
		os_sprintf(stconfig.password, "%s", WIFI_PASSWORD);
		stconfig.bssid_set = 0;
		if(!wifi_station_set_config(&stconfig))
		{
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Configuratie mislukt!\r\n");
			#endif
		}
	}
	if(wifi_get_phy_mode() != PHY_MODE_11N)
		wifi_set_phy_mode(PHY_MODE_11N);
	#ifdef PLATFORM_DEBUG
	ets_uart_printf("Configuratie gelukt!\r\n");
	#endif
}

void eventHandler(System_Event_t *event) {
	static struct ip_info ipconfig;
	static struct ip_addr ladres;

	switch(event->event) {
		case EVENT_STAMODE_CONNECTED:
			xLogEntry(LOGGER_WIFI_VERBONDEN, 0);
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Wifi verbonden\r\n");
			#endif
			break;
		case EVENT_STAMODE_DISCONNECTED:
			xLogEntry(LOGGER_WIFI_VERBROKEN, 0);
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Wifi verbinding verbroken\r\n");
			#endif
			mVerbonden = false;
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: Station mode - Authenticatie veranderd\r\n");
			#endif
			break;
		case EVENT_STAMODE_GOT_IP:
			wifi_get_ip_info(STATION_IF, &ipconfig);
			xLogEntry(LOGGER_IP_ONTVANGEN, ipconfig.ip.addr);
			#ifdef PLATFORM_DEBUG
			ladres.addr = ipconfig.ip.addr;
			ets_uart_printf("IP adres gekregen: " IPSTR "\r\n", IP2STR(&ladres));
			#endif
			mVerbonden = true;
			if (mStart){
				mStart = false;
				xTijdInit();
			}
			http_init();
			break;
		case EVENT_STAMODE_DHCP_TIMEOUT:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_DHCP_TIMEOUT\r\n");
			#endif
			break;
		case EVENT_SOFTAPMODE_STACONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_SOFTAPMODE_STACONNECTED\r\n");
			#endif
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_SOFTAPMODE_STADISCONNECTED\r\n");
			#endif
			break;
		case EVENT_SOFTAPMODE_PROBEREQRECVED:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: EVENT_SOFTAPMODE_PROBEREQRECVED\r\n");
			#endif
			break;
		case EVENT_OPMODE_CHANGED:  //Eclipse vindt dit fout. Compileert en werkt wel correct.
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Event: Operation mode veranderd\r\n");
			#endif
			break;
		default:
			#ifdef PLATFORM_DEBUG
			ets_uart_printf("Onverwacht event: %d\n", event->event);
			#endif
			break;
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{
#ifdef METMAC
	char lMac[6] = {MAC_ADRES};
	wifi_set_macaddr(STATION_IF, lMac);
#endif
	gpio_init();
#ifdef PLATFORM_DEBUG
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
#endif
	xSchakelInit();
	xDrukknopInit();
#ifndef PLATFORM_DEBUG
	xLogInit();
#endif
	mVerbonden = false;
	#ifdef PLATFORM_DEBUG
		mVolgnr = 0;
		mAanloop = true;
	#endif
	wifi_set_event_handler_cb(eventHandler);
	system_init_done_cb(&systeem_klaar);
}
