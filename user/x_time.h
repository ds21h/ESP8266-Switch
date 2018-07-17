/*
 * tijd.h
 *
 *  Created on: 11 jun. 2017
 *      Author: Jan
 */

#ifndef USER_X_TIME_H_
#define USER_X_TIME_H_

#include <c_types.h>

uint32 xTimeNow();
void xTimeInit();
void xTimeString(uint32 pTime, char * pResult, uint8 pLength);


#endif /* USER_X_TIME_H_ */
