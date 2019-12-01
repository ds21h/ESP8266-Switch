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
//***version 2.1.0 24-07-2018
//***			- Changed to FOTA
//***			- Added API to request upgrade
//***			- Added upgrade function (ota_upgrade.c)
//***version 2.2.0 13-08-2018
//***			- MAC is optional in setting. Note: Setting the MAC requires a double restart of the module before the MAC is used.
//***			- Server IP and Port are now part of Settings.
//***			- In Settings the version is relocated to the front. This to make upgrade possible. This version is not compatible with previous version.
//***			- Version checks are included for upgrade. Normally only higher version will be accepted.
//***			- Force option is included in upgrade to load non-accepted version.
//***version 2.2.1 19-09-2018
//***			- xSwitchInit moved from eMainSetup to cbMainSystemReady in order to bypass the startup delay: The switch is ON on start and xSwitchInit switches it off!
//***Version 2.3.0 08-12-2018:
//***			- The Dutch API is removed
//***			- Message buffers increased to 2048 bytes (log messages caused overflow)
//***			- Button setting improved. Changing this setting only had effect after restart. Now immediately.
//***			- URI /Switch/Button deleted. Button setting only available in /Switch/Setting
//***			- Upgraded to SDK 3.0.0
//***Version 2.3.1 14-07-2019
//***			- Added basic auto-off function that switches off after 12 hours
//***Version 2.3.2 27-07-2019
//***			- Complete auto-off function
//***				- Make log entry with auto-off
//***				- Include auto-off time in settings
//***				- Initial auto-off after 43200 seconds (12 hours)
//***				- Auto-off value of 0 disables function
//***				- Display on-time in status
//***Version 2.4 01-12-2019
//***			- Support for different Switch boards added
//***				- Setting SwitchModel added (see MessageFormat.txt for supported models)
//***				- switch.c and button.c altered to handle this
//***			- URI /Switch/Restart added to force a restart (necessary after changing some of the settings)
//***************************************************************
#define VERSION			"v2.4.0"
#define AUTO_OFF		43200
#define SWITCH_MODEL	1
//***************************************************************
//***	Memory non-Fota
//***
//***	0x00000: eagle.flash.bin (max 64 KB)
//***				contains boot sector and the user program that is loaded in memory on startup (including variables)
//***	0x10000: eagle.irom0text.bin
//***				contains the read-only part of the user program
//***	in between: free for use (attention, Flash memory, to be used in segments of 4 KB!)
//***	0xFB000: system params (ao RF calibration Sector - output of user_rf_cal_sector_set)
//***
//***************************************************************
//***	Memory Fota
//***
//***	0x00000: Boot (4K)
//***				contains bootloader and logic to determine which image to use
//***	0x01000: Bin1 (max 488K, together with User data 1)
//***				contains Image 1 of the program
//***	connecting: User data 1 (max 488K, together with Bin1)
//***				Free to use data
//***	0x7B000: User data (max 24K)
//***				Free to use
//***	0x81000: Bin2 (max 488K, together with User data 2)
//***				contains Image 2 of the program
//***	connecting: User data 2 (max 488K, together with Bin2)
//***				Free to use data
//***	0xFB000: RF Cal sector (4K)
//***	0xFC000: RF Param sector (4K)
//***	0xFD000: System parameters (12K)
//***
//***************************************************************
//***	Used sectors in flash-memory
#define LOG_SECTOR		0x7C
#define SETTING_SECTOR	0x7D

#define LEN_SSID		32
#define LEN_PASSWORD	64

#define STARTPAUSE	10
#endif
