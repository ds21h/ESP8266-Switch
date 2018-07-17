/*
 * util.c
 *
 *  Created on: 1 okt. 2017
 *      Author: Jan
 */
#include <c_types.h>
#include <osapi.h>
#include "user_config.h"
#include "util.h"

int ICACHE_FLASH_ATTR xStrnCmpX(const char *pStr1, const char *pStr2, int pLength){
	char lChar1;
	char lChar2;
	int lCount;
	int lDifference;

	for (lCount = 0; lCount < pLength; lCount++){
		lChar1 = xLCase(pStr1[lCount]);
		lChar2 = xLCase(pStr2[lCount]);
		if (lChar1 == 0){
			if (lChar2 == 0){
				lDifference = 0;
			} else {
				lDifference = -1;
			}
			break;
		} else {
			if (lChar2 == 0){
				lDifference = 1;
				break;
			} else {
				lDifference = lChar1 - lChar2;
				if (lDifference != 0){
					break;
				}
			}
		}
	}
	return lDifference;
}

int ICACHE_FLASH_ATTR xStrCmpX(const char *pStr1, const char *pStr2){
	int lLen1;
	int lLen2;
	int lMax;
	int lDifference;

	lLen1 = os_strlen(pStr1);
	lLen2 = os_strlen(pStr2);
	if (lLen1 < lLen2){
		lMax = lLen2;
	} else {
		lMax = lLen1;
	}
	lDifference = xStrnCmpX(pStr1, pStr2, lMax);
	return lDifference;
}

char ICACHE_FLASH_ATTR xLCase(char pIn){
	char lOut;

	if (pIn >= 65 && pIn <= 90){
		lOut = pIn + 32;
	} else {
		lOut = pIn;
	}
	return lOut;
}
