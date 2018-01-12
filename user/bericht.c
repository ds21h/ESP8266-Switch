/*
 * bericht.c
 *
 *  Created on: 9 jun. 2017
 *      Author: Jan
 */
#include <c_types.h>
#include <osapi.h>
#include <mem.h>
#include <json/json.h>
#include <json/jsonparse.h>
#include <json/jsontree.h>
#include <ip_addr.h>
#include "bericht.h"
#include "schakel.h"
#include "user_config.h"
#include "logger.h"
#include "drukknop.h"
#include "util.h"

#define URI_BASIS	"/Schakelaar"
#define URI_LOG		"Log"

#define UriBasis	0
#define UriCompleet	1
#define UriLog		2
#define UriLogPar	3
#define UriDrukknop	4
#define UriFavicon	8
#define UriFout		9

#define AktieGeen -1
#define AktieUit	0
#define AktieAan	1

LOCAL int ICACHE_FLASH_ATTR sVulBericht(struct jsontree_context *pCtx);
LOCAL int ICACHE_FLASH_ATTR sVulLog(struct jsontree_context *pCtx);
LOCAL int ICACHE_FLASH_ATTR sVulLogEntry(struct jsontree_context *pCtx);

struct jsontree_context mCtx;

LOCAL struct jsontree_callback mVulBericht_callback = JSONTREE_CALLBACK(sVulBericht, NULL);
LOCAL struct jsontree_callback mVulLog_callback = JSONTREE_CALLBACK(sVulLog, NULL);
LOCAL struct jsontree_callback mVulLogEntry_callback = JSONTREE_CALLBACK(sVulLogEntry, NULL);
LOCAL char *mBuffer = NULL;
LOCAL int mPos;
LOCAL int mGrootte;
LOCAL bool mVerwerkOK = false;
LOCAL char mMelding[64] = "";

uint8 mEntry;
uint8 mEntryAantal;

LOCAL int ICACHE_FLASH_ATTR sVulBericht(struct jsontree_context *pCtx){
    const char* lPad;

	lPad = jsontree_path_name(pCtx, pCtx->depth - 1);
    if (os_strncmp(lPad, "resultaat", 9) == 0) {
        jsontree_write_string(pCtx, mVerwerkOK ? "OK" : "NOK");
    }
    if (os_strncmp(lPad, "status", 6) == 0) {
        jsontree_write_string(pCtx, xSchakelStatus() ? "aan" : "uit");
    }
    if (os_strncmp(lPad, "drukknop", 8) == 0) {
        jsontree_write_string(pCtx, xDrukknopStatus() ? "aan" : "uit");
    }
    if (os_strncmp(lPad, "melding", 7) == 0) {
        jsontree_write_string(pCtx, mMelding);
    }
    if (os_strncmp(lPad, "naam", 4) == 0) {
        jsontree_write_string(pCtx, SCHAKEL_NAAM);
    }
    if (os_strncmp(lPad, "omschr", 6) == 0) {
        jsontree_write_string(pCtx, SCHAKEL_OMS);
    }
    if (os_strncmp(lPad, "versie", 6) == 0) {
        jsontree_write_string(pCtx, VERSIE);
    }
    if (os_strncmp(lPad, "sdk", 3) == 0) {
        jsontree_write_string(pCtx, (char*)system_get_sdk_version());
    }
    if (os_strncmp(lPad, "datum", 5) == 0) {
        jsontree_write_string(pCtx, __DATE__);
    }
    return 0;
}

LOCAL int ICACHE_FLASH_ATTR sVulLog(struct jsontree_context *pCtx){
    const char* lPad;
	uint32 lTijd;
	char lTijdStr[20];


	lPad = jsontree_path_name(pCtx, pCtx->depth - 1);
    if (os_strncmp(lPad, "resultaat", 9) == 0) {
        jsontree_write_string(pCtx, mVerwerkOK ? "OK" : "NOK");
    }
    if (os_strncmp(lPad, "aantal", 6) == 0) {
        jsontree_write_int(pCtx, xLogAantal());
    }
    if (os_strncmp(lPad, "huidig", 6) == 0) {
        jsontree_write_int(pCtx, xLogHuidig());
    }
    if (os_strncmp(lPad, "tijd", 4) == 0) {
    	lTijd = xTijdNu();
    	xTijdString(lTijd, lTijdStr, sizeof(lTijdStr));
        jsontree_write_string(pCtx, lTijdStr);
    }
    return 0;
}

LOCAL int ICACHE_FLASH_ATTR sVulLogEntry(struct jsontree_context *pCtx){
    const char* lPad;
    struct ip_addr lIp;
	char lIpStr[12];
	uint32 lTijd;
	char lTijdStr[20];

	mEntryAantal++;
	if (mEntryAantal > 4){
		mEntryAantal = 1;
		mEntry--;
		if (mEntry >= LOGGER_AANTAL_ENTRIES){
			mEntry = LOGGER_AANTAL_ENTRIES - 1;
		}
	}
	lPad = jsontree_path_name(pCtx, pCtx->depth - 1);
    if (os_strncmp(lPad, "entry", 5) == 0) {
        jsontree_write_int(pCtx, mEntry);
    }
    if (os_strncmp(lPad, "aktie", 5) == 0) {
        jsontree_write_int(pCtx, xLogAktie(mEntry));
    }
    if (os_strncmp(lPad, "tijd", 4) == 0) {
    	lTijd = xLogTijd(mEntry);
    	xTijdString(lTijd, lTijdStr, sizeof(lTijdStr));
        jsontree_write_string(pCtx, lTijdStr);
//        jsontree_write_int(pCtx, (int)xLogTijd(mEntry));
    }
    if (os_strncmp(lPad, "ip", 2) == 0) {
    	lIp.addr = xLogIp(mEntry);
    	os_sprintf(lIpStr, IPSTR, IP2STR(&lIp));
        jsontree_write_string(pCtx, lIpStr);
    }
    return 0;
}

int ICACHE_FLASH_ATTR sSchrijfChar(int pChar)
{
    if (mBuffer != NULL && mPos <= mGrootte) {
        mBuffer[mPos] = pChar;
        mPos++;
        return pChar;
    }

    return 0;
}

void ICACHE_FLASH_ATTR sGrootAntwoord(char *pBericht){
	bool lDoorgaan;
	int lResult;
	mBuffer = NULL;
	JSONTREE_OBJECT(lResult_Tree,
	                JSONTREE_PAIR("resultaat", &mVulBericht_callback),
	                JSONTREE_PAIR("versie", &mVulBericht_callback),
	                JSONTREE_PAIR("sdk-versie", &mVulBericht_callback),
	                JSONTREE_PAIR("datum", &mVulBericht_callback),
	                JSONTREE_PAIR("naam", &mVulBericht_callback),
	                JSONTREE_PAIR("omschr", &mVulBericht_callback),
					JSONTREE_PAIR("status", &mVulBericht_callback),
					JSONTREE_PAIR("drukknop", &mVulBericht_callback));

	mBuffer = (char *)os_zalloc(BERICHT_SIZE);
    mPos = 0;
    mGrootte = BERICHT_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value *)&lResult_Tree, sSchrijfChar);

	lDoorgaan = true;
	while (lDoorgaan){
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0){
			lDoorgaan = false;
		}
	}
	mBuffer[mPos] = 0;
    os_memcpy(pBericht, mBuffer, mPos + 1);
	os_free(mBuffer);
	mBuffer = NULL;
}

/**
void ICACHE_FLASH_ATTR sNormaalAntwoord(char *pBericht){
	bool lDoorgaan;
	int lResult;
	mBuffer = NULL;
	JSONTREE_OBJECT(lResult_Tree,
	                JSONTREE_PAIR("resultaat", &mVulBericht_callback),
					JSONTREE_PAIR("status", &mVulBericht_callback),
					JSONTREE_PAIR("drukknop", &mVulBericht_callback));

	mBuffer = (char *)os_zalloc(BERICHT_SIZE);
    mPos = 0;
    mGrootte = BERICHT_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value *)&lResult_Tree, sSchrijfChar);

	lDoorgaan = true;
	while (lDoorgaan){
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0){
			lDoorgaan = false;
		}
	}
	mBuffer[mPos] = 0;
    os_memcpy(pBericht, mBuffer, mPos + 1);
	os_free(mBuffer);
	mBuffer = NULL;
} */

void ICACHE_FLASH_ATTR sFoutAntwoord(char *pBericht){
	bool lDoorgaan;
	int lResult;
	mBuffer = NULL;
	JSONTREE_OBJECT(lResult_Tree,
	                JSONTREE_PAIR("resultaat", &mVulBericht_callback),
					JSONTREE_PAIR("melding", &mVulBericht_callback));

	mBuffer = (char *)os_zalloc(BERICHT_SIZE);
    mPos = 0;
    mGrootte = BERICHT_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value *)&lResult_Tree, sSchrijfChar);

	lDoorgaan = true;
	while (lDoorgaan){
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0){
			lDoorgaan = false;
		}
	}
	mBuffer[mPos] = 0;
    os_memcpy(pBericht, mBuffer, mPos + 1);
	os_free(mBuffer);
	mBuffer = NULL;
}

void ICACHE_FLASH_ATTR sLogAntwoord(int pStart, char *pBericht){
	bool lDoorgaan;
	int lResult;
	mBuffer = NULL;
	JSONTREE_OBJECT(lLogEntry_Tree,
					JSONTREE_PAIR("entry", &mVulLogEntry_callback),
					JSONTREE_PAIR("aktie", &mVulLogEntry_callback),
					JSONTREE_PAIR("tijd", &mVulLogEntry_callback),
					JSONTREE_PAIR("ip", &mVulLogEntry_callback));
	JSONTREE_ARRAY(lLog_Array, (struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree,
								(struct jsontree_value *)&lLogEntry_Tree);
	JSONTREE_OBJECT(lResult_Tree,
					JSONTREE_PAIR("resultaat", &mVulLog_callback),
	                JSONTREE_PAIR("aantal", &mVulLog_callback),
	                JSONTREE_PAIR("huidig", &mVulLog_callback),
					JSONTREE_PAIR("tijd", &mVulLog_callback),
					JSONTREE_PAIR("log", &lLog_Array));

	mBuffer = (char *)os_zalloc(BERICHT_SIZE);
	if (pStart < 0){
		mEntry = xLogHuidig() - 1;
	} else {
		mEntry = (uint8)pStart;
	}
	if (mEntry >= LOGGER_AANTAL_ENTRIES){
		mEntry = LOGGER_AANTAL_ENTRIES - 1;
	}
	mEntryAantal = 0;
    mPos = 0;
    mGrootte = BERICHT_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value *)&lResult_Tree, sSchrijfChar);

	lDoorgaan = true;
	while (lDoorgaan){
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0){
			lDoorgaan = false;
		}
	}
	mBuffer[mPos] = 0;
    os_memcpy(pBericht, mBuffer, mPos + 1);
	os_free(mBuffer);
	mBuffer = NULL;
}

int ICACHE_FLASH_ATTR sParseBericht(char * pBericht){
    struct jsonparse_state lParseState;
    int lType;
    int lType2;
    char lBuffer[64];
    int lAktie;

    lAktie = AktieGeen;
    jsonparse_setup(&lParseState, pBericht, os_strlen(pBericht));
    while ((lType = jsonparse_next(&lParseState)) != 0) {
        if (lType == JSON_TYPE_PAIR_NAME) {
            os_bzero(lBuffer, 64);
            jsonparse_copy_value(&lParseState, lBuffer, sizeof(lBuffer));
            if (xStrCmpX(lBuffer, "status") == 0) {
                lType2 = jsonparse_next(&lParseState);
                if (lType2 == JSON_TYPE_PAIR){
                    lType2 = jsonparse_next(&lParseState);
                    if (lType2 == JSON_TYPE_STRING){
                        os_bzero(lBuffer, 64);
                        jsonparse_copy_value(&lParseState, lBuffer, sizeof(lBuffer));
                        if (xStrCmpX(lBuffer, "aan") == 0) {
                        	lAktie = AktieAan;
                        } else {
                        	if (xStrCmpX(lBuffer, "uit") == 0) {
                            	lAktie = AktieUit;
                        	} else {
                        		lAktie = AktieGeen;
                        	}
                        }
                    }
                }
            }
        }
    }
    return lAktie;
}

LOCAL int ICACHE_FLASH_ATTR sTestUri(struct Bericht *pSchakel){
	const char lBasis[] = URI_BASIS;
	const char lId[] = SCHAKEL_NAAM;
	const char lLog[] = URI_LOG;
	const char lDrukknop[] = "Drukknop";
	const char lFavIcon[] = "/favicon.ico";
	char *lUriPtr;
	int lUriType;

	lUriType = UriFout;
	if (xStrnCmpX(pSchakel->sUri, lBasis, os_strlen(lBasis)) == 0){
		lUriPtr = pSchakel->sUri;
		lUriPtr += os_strlen(lBasis);
		if (*lUriPtr == '\0'){
			lUriType = UriBasis;
		} else {
			if (*lUriPtr == '/'){
				lUriPtr++;
				if (*lUriPtr == '\0'){
					lUriType = UriBasis;
				} else {
					if (xStrnCmpX(lUriPtr, lId, os_strlen(lId)) == 0){
						lUriPtr += os_strlen(lId);
						if (*lUriPtr == '\0'){
							lUriType = UriCompleet;
						} else {
							if (*lUriPtr ==  '/'){
								lUriPtr++;
								if (*lUriPtr == '\0'){
									lUriType = UriCompleet;
								} else {
									if (xStrnCmpX(lUriPtr, lLog, os_strlen(lLog)) == 0){
										lUriPtr += os_strlen(lLog);
										if (*lUriPtr == '\0'){
											lUriType = UriLog;
										} else {
											if (*lUriPtr ==  '?'){
												lUriType = UriLogPar;
											}
										}
									} else {
										if (xStrnCmpX(lUriPtr, lDrukknop, os_strlen(lDrukknop)) == 0){
											lUriPtr += os_strlen(lDrukknop);
											if (*lUriPtr == '\0'){
												lUriType = UriDrukknop;
											}
										}
									}
								}
							}
						}
					} else {
						if (xStrnCmpX(lUriPtr, lLog, os_strlen(lLog)) == 0){
							lUriPtr += os_strlen(lLog);
							if (*lUriPtr == '\0'){
								lUriType = UriLog;
							} else {
								if (*lUriPtr ==  '?'){
									lUriType = UriLogPar;
								}
							}
						} else {
							if (xStrnCmpX(lUriPtr, lDrukknop, os_strlen(lDrukknop)) == 0){
								lUriPtr += os_strlen(lDrukknop);
								if (*lUriPtr == '\0'){
									lUriType = UriDrukknop;
								}
							}
						}
					}
				}
			}
		}
	} else {
		if (xStrnCmpX(pSchakel->sUri, lFavIcon, os_strlen(lFavIcon)) == 0){
			lUriType = UriFavicon;
		}

	}
	return lUriType;
}

LOCAL int ICACHE_FLASH_ATTR sBepaalStart(struct Bericht *pSchakel){
	int lStart;
	const char lBasisLog1[] = URI_BASIS "/" SCHAKEL_NAAM "/" URI_LOG "?";
	const char lBasisLog2[] = URI_BASIS "/" URI_LOG "?";
	const char lBasisStart[] = "start=";
	char *lUriPtr;

	lStart = -1;
	lUriPtr = pSchakel->sUri;
	if (xStrnCmpX(lUriPtr, lBasisLog1, os_strlen(lBasisLog1)) == 0){
		lUriPtr += os_strlen(lBasisLog1);
		if (xStrnCmpX(lUriPtr, lBasisStart, os_strlen(lBasisStart)) == 0){
			lUriPtr += os_strlen(lBasisStart);
			lStart = atoi(lUriPtr);
		}
	} else {
		if (xStrnCmpX(lUriPtr, lBasisLog2, os_strlen(lBasisLog2)) == 0){
			lUriPtr += os_strlen(lBasisLog2);
			if (xStrnCmpX(lUriPtr, lBasisStart, os_strlen(lBasisStart)) == 0){
				lUriPtr += os_strlen(lBasisStart);
				lStart = atoi(lUriPtr);
			}
		}

	}
	return lStart;
}

void ICACHE_FLASH_ATTR xMaakAntwoord(char *pMelding, char *pBericht){
	mVerwerkOK = false;
	os_strcpy(mMelding, pMelding);

	sFoutAntwoord(pBericht);
}

void ICACHE_FLASH_ATTR xVerwerkBericht(uint32 pIp, struct Bericht *pBericht){
	int lUriType;
	int lAktie;
	int lStart;

	if (os_strcmp(pBericht->sVerb, "GET") == 0){
		lUriType = sTestUri(pBericht);
		switch (lUriType){
			case UriBasis:
				mVerwerkOK = true;
				sGrootAntwoord(pBericht->sBericht);
				xLogEntry(LOGGER_GET_GROOT, pIp);
				break;
			case UriCompleet:
				mVerwerkOK = true;
				sGrootAntwoord(pBericht->sBericht);
				xLogEntry(LOGGER_GET_NORMAAL, pIp);
				break;
			case UriDrukknop:
				mVerwerkOK = true;
				sGrootAntwoord(pBericht->sBericht);
				xLogEntry(LOGGER_GET_NORMAAL, pIp);
				break;
			case UriLog:
				mVerwerkOK = true;
				sLogAntwoord(-1, pBericht->sBericht);
//				xLogEntry(LOGGER_GET_LOG, pIp);
				break;
			case UriLogPar:
				lStart = sBepaalStart(pBericht);
				mVerwerkOK = true;
				sLogAntwoord(lStart, pBericht->sBericht);
//				xLogEntry(LOGGER_GET_LOG, pIp);
				break;
			default:
				mVerwerkOK = false;
				os_strcpy(pBericht->sBericht, "404");
				if (lUriType != UriFavicon){
					xLogEntry(LOGGER_GET_FOUT, pIp);
				}
				break;
		}
	} else {
		if (os_strcmp(pBericht->sVerb, "PUT") == 0){
			lUriType = sTestUri(pBericht);
			switch (lUriType){
				case UriBasis:
				case UriCompleet:
					lAktie = sParseBericht(pBericht->sBericht);
					if (lAktie == AktieAan){
						xSchakelZet(true);
						mVerwerkOK = true;
						sGrootAntwoord(pBericht->sBericht);
						xLogEntry(LOGGER_PUT_AAN, pIp);
					} else {
						if (lAktie == AktieUit){
							xSchakelZet(false);
							mVerwerkOK = true;
							sGrootAntwoord(pBericht->sBericht);
							xLogEntry(LOGGER_PUT_UIT, pIp);
						} else {
							mVerwerkOK = false;
							os_strcpy(mMelding, "Onjuiste opdracht");
							sFoutAntwoord(pBericht->sBericht);
							xLogEntry(LOGGER_PUT_FOUT, pIp);
						}
					}
					break;
				case UriDrukknop:
					lAktie = sParseBericht(pBericht->sBericht);
					if (lAktie == AktieAan){
						xDrukknopAktief();
						mVerwerkOK = true;
						sGrootAntwoord(pBericht->sBericht);
						xLogEntry(LOGGER_PUT_DRUKKNOP_AAN, pIp);
					} else {
						if (lAktie == AktieUit){
							xDrukknopInAktief();
							mVerwerkOK = true;
							sGrootAntwoord(pBericht->sBericht);
							xLogEntry(LOGGER_PUT_DRUKKNOP_UIT, pIp);
						} else {
							mVerwerkOK = false;
							os_strcpy(mMelding, "Onjuiste opdracht");
							sFoutAntwoord(pBericht->sBericht);
							xLogEntry(LOGGER_PUT_FOUT, pIp);
						}
					}
					break;
				default:
					mVerwerkOK = false;
					os_strcpy(pBericht->sBericht, "404");
					if (lUriType != UriFavicon){
						xLogEntry(LOGGER_GET_FOUT, pIp);
					}
					break;
			}
		} else {
			mVerwerkOK = false;
			os_strcpy(mMelding, "Verb (nog) niet ondersteund");
			sFoutAntwoord(pBericht->sBericht);
			xLogEntry(LOGGER_VERB_FOUT, pIp);
		}
	}
}


