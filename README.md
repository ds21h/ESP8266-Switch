# ESP8266-Switch

Software to control ESP8266 based switch.
The switch registers to your wifi network. It can then be adressed by a few REST-services.

If you like this/hate this/have any comments/have any questions/just want to chat about this please leave me a message at ds21h@hotmail.com

Supported services:

GET
	/Switch
		Returns SwitchData
	/Switch/Setting
		Returns SettingData
	/Switch/Log
		Returns LogData starting with the last used index
	/Switch/Log?Start=nn
		Returns LogData starting with the given index
	/Switch/Upgrade?Version=vnn.nn.nn
		Starts upgrade of the software to the given version
	/Switch/Upgrade?Version=vnn.nn.nn&Force
		Starts upgrade of the software to the given version ignoring error situations (already that version, to lower version)
	
PUT
	/Switch
		Payload SetSwitchData
		Returns SwitchData
	/Switch/Setting
		Payload SetSettingData
		Returns SettingData
	
For descriptions of the data see below the version history

Version 2.3.2 27-07-2019
- Complete auto-off function
	- Make log entry with auto-off
	- Include auto-off time in settings
	- Initial auto-off after 43200 seconds (12 hours)
	- Auto-off value of 0 disables function
	- Display on time in status

Version 2.3.1 14-07-2019:
- Added basic auto-off function that switches off after 12 hours

Version 2.3.0 08-12-2018:
- The Dutch API is removed
- Message buffers increased to 2048 bytes (log messages caused overflow)
- Button setting improved. Changing this setting only had effect after restart. Now immediately.
- URI /Switch/Button deleted. Button setting only available in /Switch/Setting
- Upgraded to SDK 3.0.0

Version 2.2.1 19-09-2018:
- xSwitchInit moved from eMainSetup to cbMainSystemReady in order to bypass the startup delay: The switch is ON on startup and xSwitchInit switches it off!

Version 2.2.0 13-08-2018 (never published):
- MAC is optional in setting. Note: Setting the MAC requires a double restart of the module before the MAC is used.
- Server IP and Port are now part of Settings.
- In Settings the version is relocated to the front. This to make upgrade possible. This version is not compatible with previous version.
- Version checks are included for upgrade. Normally only higher version will be accepted.
- Force option is included in upgrade to load non-accepted version.

Version 2.1:
  - Software is in the English language
  - Services now exist in both English and Dutch. Usage is controled by te first part ot the URI. If this reads 'Schakelaar' then the Dutch version is used. If this reads 'Switch' the English version is used. Please use the English version. In the future the Dutch version will be removed.
  - The URI is now case-independent.
  - The software is the same for every switch. All settings are seperately stored. If the settings are not yet there the ESP8266 boots as an Access Point using the name EspSw-[mac]. It can then be connected to using the password EspSwSetup in order to initialise the settings. For this a Java desktop program is available (EspSettings).
  - The software is now basically capable of updating over the air (FOTA). For that an extra URI is implemented requesting an update. For the update itself a server is required. A Javax version is available (EspServer).
  
JSON formats
SwitchData:
{
"result":"OK",							if NOK then the rest is not available
"version":"v2.2.1",						The version of the software
"sdk-version":"2.2.1(6ab97e9)",			The version of the SDK
"date":"Sep 19 2018",					Date of compilation of the software
"name":"Test",							Name of the switch 
"descr":"Switch for testing",			Description of the switch
"status":"on",							Current switch status
"loglevel":1,							Logging level. 0 = no logging, 1 = non-persistant logging, 2 = persistant logging
"button":"on"							Button enabled
}

SettingData:
{
"result":"OK",							if NOK then the rest is not available
"ssid":"YourSsid",						SSID of your network
"password":"YourPassword",				Password for your network
"mac":"00:ff:ff:ff:00:01",				MAC of this switch in your network
"name":"Test",							Name of the switch
"descr":"Switch for testing",			Description of the switch
"loglevel":1,							Logging level. 0 = no logging, 1 = non-persistant logging, 2 = persistant logging
"button":"on",							Button enabled
"serverip":"192.168.2.99",				IP of your upgrade server
"serverport":8080						Port of your upgrade server
}

LogData:
{
"result":"OK",							if NOK then the rest is not available
"number":250,							Number of log entries (cyclic log)
"current":66,							The current (= next to use) entry
"time":"2018-12-08 15:59:09",			Time of the request (always in UTC, so no time zone or DST)
"log":[									10 log entries
{
"entry":65,								entry index
"action":"GET Switch",					Logged action
"time":"2018-12-08 15:58:48",			Timestamp of action in UTC. Note: If the switch has not yet synchronised time this reads 1970-01-01 00:00:00 -> Unix time start.
"ip":"192.168.2.1"						The IP addres that requested the action
},
.....
]
}

SetSwitchData:
{
"status":"on"							To switch on (off to switch off)
}

SetSettingData:
{										Every element is optional. If not present the setting is left unchanged
"ssid":"YourSsid",						SSID of your network
"password":"YourPassword",				Password for your network
"mac":"00:ff:ff:ff:00:01",				MAC of this switch in your network
"name":"Test",							Name of the switch
"descr":"Switch for testing",			Description of the switch
"loglevel":1,							Logging level. 0 = no logging, 1 = non-persistant logging, 2 = persistant logging
"button":"on",							Button enabled
"serverip":"192.168.2.99",				IP of your upgrade server
"serverport":8080,						Port of your upgrade server
"reset":"true"							Reset the settings to zero. If present all other settings are ignored!
}
