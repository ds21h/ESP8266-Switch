/*
 * bericht.h
 *
 *  Created on: 9 jun. 2017
 *      Author: Jan
 */

#ifndef USER_BERICHT_H_
#define USER_BERICHT_H_

#define BERICHT_VERB_SIZE		7
#define BERICHT_URI_SIZE		256
#define BERICHT_SIZE			1024

struct Bericht {
	char sVerb[BERICHT_VERB_SIZE];
	char sUri[BERICHT_URI_SIZE];
	char sBericht[BERICHT_SIZE];
};

void xVerwerkBericht(uint32 pIp, struct Bericht *pBericht);


#endif /* USER_BERICHT_H_ */
