/*
 * switch.h
 *
 *  Created on: 26 feb. 2017
 *      Author: Jan
 */

#ifndef USER_SWITCH_H_
#define USER_SWITCH_H_

#include <c_types.h>

void xSwitchInit();
void xSwitchOn(long pAutoOff);
void xSwitchOff();
void xSwitchFlip();
bool xSwitchStatus();

#endif /* USER_SWITCH_H_ */
