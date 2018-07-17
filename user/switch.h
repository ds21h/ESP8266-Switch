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
void xSwitchFlip();
bool xSwitchStatus();
void xSwitchSet(bool pValue);

#endif /* USER_SWITCH_H_ */
