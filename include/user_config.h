#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

//***************************************************************
//***
//***  Let op: Zet juiste context hieronder!
//***
//***************************************************************
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
//***
//***************************************************************
#define VERSIE			"1.2.1"
//***************************************************************
//***	Memory
//***
//***	0x00000: eagle.flash.bin (max 64 KB)
//***				bevat boot sector en het user programma dat in het geheugen geladen wordt (dus ook de variabelen)
//***	0x10000: eagle.irom0text.bin
//***				bevat het read-only deel van het user programma
//***	tussenliggend: vrij voor programmagebruik (let op Flash memory, te gebruiken in segmenten van 4 KB!)
//***	0xFC000: system params (oa RF calibration Sector - output van user_rf_cal_sector_set)
//***
//***************************************************************
//***	Te gebuiken sectoren in flash-memory
#define LOGGER_SECTOR	0x7C
#define DRUKKNOP_SECTOR	0x7D

//***	Algemeen te gebruiken resultaat-codes
#define RESULT_OK				0
#define RESULT_FOUT				9

//***	Als gedefinieerd in debug mode
//#define PLATFORM_DEBUG	1

#define METMAC
#define SCHAKELAAR05

//***  Netwerk definitie
#define WIFI_SSID		"Your SSID"
#define WIFI_PASSWORD	"Your password"
#define TCPIP_PORT 		80

//***  Definitie schakelaars
#ifdef SCHAKELAAR01
#define SCHAKEL_NAAM	"Lamp"
#define SCHAKEL_OMS		"Staande schemerlamp"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x01
#endif
#ifdef SCHAKELAAR02
#define SCHAKEL_NAAM	"Cocktails"
#define SCHAKEL_OMS		"Neon Cocktails"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x02
#endif
#ifdef SCHAKELAAR03
#define SCHAKEL_NAAM	"RockOla"
#define SCHAKEL_OMS		"RockOla 1422"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x03
#endif
#ifdef SCHAKELAAR04
#define SCHAKEL_NAAM	"Pennen"
#define SCHAKEL_OMS		"Pennenkast"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x04
#endif
#ifdef SCHAKELAAR05
#define SCHAKEL_NAAM	"Ami"
#define SCHAKEL_OMS		"AMI H"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x05
#endif
#ifdef SCHAKELAAR06
#define SCHAKEL_NAAM	"CrissCross"
#define SCHAKEL_OMS		"Gokkast CrissCross"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x06
#endif
#ifdef SCHAKELAAR07
#define SCHAKEL_NAAM	"GrijpKast"
#define SCHAKEL_OMS		"Grijpkast Fortune Crane"
#define MAC_ADRES		0x00, 0xff, 0xff, 0xff, 0x00, 0x07
#endif

#endif
