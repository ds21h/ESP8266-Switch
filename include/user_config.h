#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

//***
//*** Versie 1.0 11-07-2017
//***			Eerste stabiele versie op PCB versie 1.0
//*** Versie 1.1 5-9-2017
//***			- UART initialisatie uitsluitend in debug mode
//***			- xTijdInit aanroep eenmaal ipv bij iedere keer dat IP adres verkregen wordt
//***			- Drukknop uitzetten voor sent (en weer aanzetten erna) verwijderd
//***
//***			PCB aanvulling:
//***			- 68 ohm weerstand toevoegen tussen + en - van voeding
//***			- 0.1uF condensator over vcc en gnd van ESP01 (direct op het ESP01 board)
//***Versie 1.1.test 01-10-2017
//***			Refresh functie voor SNTP uitgeschakeld om te testen hoe de 6288 dan reageert.
//***Versie 1.2.test 03-10-2017
//***			URI's case insensitive gemaakt dmv eigen String functies (in util.c)
//***			Schakelaar-id in URI is optioneel. Moet uiteindelijk totaal vervallen!
//***Versie 1.2 08-10-2017
//***			Gelijk aan versie 1.2.test. De test heeft uitgewezen dat de 6288 op zich goed omgaat met SNTP. Refresh niet nodig.
//***			Gecompileerd met SDK 2.1.0
//***versie 1.2.1 08-10-2017
//***			Gebruikte SDK toegevoegd aan antwoord
//***version 2.0.dev 12-01-2018
//***			- Translate to English
//***			- Move Network and Switch data to dynamic memory
//***			- Start as an Access Point when Network data is not up-to-date
//***			- Create API for maintenance of Network and Switch data
//***version 2.0 17-07-2018
//***			- Changed to event driven application
//***			- API both in English and in Dutch. If URI starts with 'Switch' the English API is used. If it starts with 'Schakelaar' the Dutch one is used.
//***************************************************************
#define VERSION			"2.0"
//***************************************************************
//***	Memory
//***
//***	0x00000: eagle.flash.bin (max 64 KB)
//***				contains boot sector and the user program that is loaded in memeory on startup (including variables)
//***	0x10000: eagle.irom0text.bin
//***				contains the read-only part of the user program
//***	in between: free for use (attention, Flash memory, to be used in segments of 4 KB!)
//***	0xFC000: system params (ao RF calibration Sector - output of user_rf_cal_sector_set)
//***
//***************************************************************
//***	Used sectors in flash-memory
#define LEN_SSID		32
#define LEN_PASSWORD	64

#define LOG_SECTOR		0x7C
#define SETTING_SECTOR	0x7D

//***	If define in debug mode
#define PLATFORM_DEBUG	1

#endif
