# ESP8266-Switch

Software to control ESP8266 based switch.
The switch registers to your wifi network. It can then be adressed by a few REST-services.  
If SSID in setting is not set (value "") the switch starts as AccessPoint with name EspSw_nnnnnnnnnnnn (nnnnnnnnnnnn is the MAC). After connecting using password "EspSwSetup" it listens to IP adress 192.168.4.1 (port 80). You can use the Java desktop application 'EspSettings' to give the settings their proper value.  
If SSID in setting is set the switch tries to connect to the specified network. If that isn't successfull within 40 seconds the connection attempt stops. The switch needs to be restarted. After 5 consecutive failures all the settings are reset so it wil start the next time as an Access Point. 

If you like this/hate this/have any comments/have any questions/just want to chat about this please leave me a message at ds21h@hotmail.com

For descriptions of the supported services and messages see file MessageFormat.txt

Version 2.4 01-12-2019
- Support for different Switch boards added
	- Setting SwitchModel added (see MessageFormat.txt for supported models)
	- switch.c and button.c altered to handle this
- URI /Switch/Restart added to force a restart (necessary after changing some of the settings)

Version 2.3.2 27-07-2019
- Complete auto-off function  
	- Make log entry with auto-off  
	- Include auto-off time in settings  
	- Initial auto-off after 43200 seconds (12 hours)  
	- Auto-off value of 0 disables function  
	- Display on-time in status  

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
