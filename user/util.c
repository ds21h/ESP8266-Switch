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

int ICACHE_FLASH_ATTR xStrnCmpX(const char *pStr1, const char *pStr2, int pLengte){
	char lChar1;
	char lChar2;
	int lTeller;
	int lVerschil;

	for (lTeller = 0; lTeller < pLengte; lTeller++){
		lChar1 = xLCase(pStr1[lTeller]);
		lChar2 = xLCase(pStr2[lTeller]);
		if (lChar1 == 0){
			if (lChar2 == 0){
				lVerschil = 0;
			} else {
				lVerschil = -1;
			}
			break;
		} else {
			if (lChar2 == 0){
				lVerschil = 1;
				break;
			} else {
				lVerschil = lChar1 - lChar2;
				if (lVerschil != 0){
					break;
				}
			}
		}
	}
	return lVerschil;
}

int ICACHE_FLASH_ATTR xStrCmpX(const char *pStr1, const char *pStr2){
	int lLen1;
	int lLen2;
	int lMax;
	int lVerschil;

	lLen1 = os_strlen(pStr1);
	lLen2 = os_strlen(pStr2);
	if (lLen1 < lLen2){
		lMax = lLen2;
	} else {
		lMax = lLen1;
	}
	lVerschil = xStrnCmpX(pStr1, pStr2, lMax);
	return lVerschil;
}

char ICACHE_FLASH_ATTR xLCase(char pIn){
	char lUit;

	if (pIn >= 65 && pIn <= 90){
		lUit = pIn + 32;
	} else {
		lUit = pIn;
	}
	return lUit;
}
