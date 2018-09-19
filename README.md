# ESP8266-Switch

Software to control ESP8266 based switch.
The switch registers to your wifi network. It can then be adressed by a few REST-services.

If you like this/hate this/have any comments/have any questions/just want to chat about this please leave me a message at ds21h@hotmail.com

Version 2.2.1 19-09-2018:
- xSwitchInit moved from eMainSetup to cbMainSystemReady in order to bypass the startup delay: The switch is ON on start and xSwitchInit switches it off!

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
  
