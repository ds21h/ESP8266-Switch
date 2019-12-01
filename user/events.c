/*
 * events.c
 *
 *  Created on: 16 feb. 2018
 *      Author: Jan
 */
#include <osapi.h>
#include "events.h"
#include "user_config.h"

#define LEN_EVENTQUEUE0	2
os_event_t gEventQueue0[LEN_EVENTQUEUE0];


static void ICACHE_FLASH_ATTR cbEventQueue0(os_event_t *pEvent) {
    enum eventtask lEvent;

    lEvent = (enum eventtask)pEvent->sig;

    switch(lEvent) {
    case EventStartSetup:
    	eMainSetup();
    	break;
    case EventStartHttp:
		eHttpInit();
    	break;
    case EventDisconnect:
		eHttpDisconnect(pEvent->par);
    	break;
    case EventProcessMessage:
		eMessageProcess(pEvent->par);
    	break;
    case EventMessageProcessed:
		eHttpMessageProcessed(pEvent->par);
    	break;
    case EventStartUpgrade:
		eOtaUpgrade(pEvent->par);
    	break;
    case EventRestart:
		eRestart();
    	break;
    }
}

void ICACHE_FLASH_ATTR xEventInit() {
	system_os_task(cbEventQueue0, 0, gEventQueue0, LEN_EVENTQUEUE0);
}
