/*
 * schakel.h
 *
 *  Created on: 26 feb. 2017
 *      Author: Jan
 */

#ifndef USER_SCHAKEL_H_
#define USER_SCHAKEL_H_

#include <c_types.h>
/**
#define SCHAKEL_VERB_SIZE		7
#define SCHAKEL_URI_SIZE		256
#define SCHAKEL_BERICHT_SIZE	512

struct SchakelCommand {
	char sVerb[SCHAKEL_VERB_SIZE];
	char sUri[SCHAKEL_URI_SIZE];
	char sBericht[SCHAKEL_BERICHT_SIZE];
};

struct AntwoordContext {
	bool sVerwerkOK;
	bool sGrootAntwoord;
	char sMelding[64];
};
*/
void xSchakelInit();
void xSchakelWissel();
bool xSchakelStatus();
void xSchakelZet(bool pWaarde);
/**
void schakel_verwerk(struct SchakelCommand *pSchakel);
void xmaak_json_antwoord(struct AntwoordContext *pAntwoord, char *pBericht);
*/
#endif /* USER_SCHAKEL_H_ */
