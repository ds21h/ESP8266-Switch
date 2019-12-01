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
#include "user_config.h"
#include "setting.h"
#include "switch.h"
#include "logger.h"
#include "http_connection.h"
#include "events.h"
#include "auto_off.h"

struct version {
	uint16 sMajor;
	uint16 sMinor;
	uint16 sRevision;
};

enum upgradetestresult {
	UpgradeTestOK,
	UpgradeTestVersionLow,
	UpgradeTestVersionEqual,
	UpgradeTestVersionOldError,
	UpgradeTestVersionNewError,
	UpgradeTestError
};

enum segment{
	SegmentSwitch,
	SegmentLog,
	SegmentSetting,
	SegmentUpgrade,
	SegmentRestart,
	SegmentEnd,
	SegmentData,
	SegmentError,
	SegmentFavicon
};

enum uri {
	UriBasis, UriLog, UriLogPar, UriSetting, UriUpgrade, UriRestart, UriFavicon, UriError
};

enum action {
	ActionNone, ActionOff, ActionOn
};

static int mStart;
static char mVersion[12];
static bool mForce;

static long mAutoOff = -1;

static int ICACHE_FLASH_ATTR cbMessageFillReply(struct jsontree_context *pCtx);
LOCAL int ICACHE_FLASH_ATTR cbMessageFillLogEntry(struct jsontree_context *pCtx);

struct jsontree_context mCtx;

static struct jsontree_callback mMessageFillReply_callback =
		JSONTREE_CALLBACK(cbMessageFillReply, NULL);
LOCAL struct jsontree_callback mMessageFillLogEntry_callback =
		JSONTREE_CALLBACK(cbMessageFillLogEntry, NULL);
static char *mBuffer = NULL;
static int mPos;
static int mSize;
static bool mProcessOk = false;
static char mText[64] = "";

static uint8 mEntry;
static uint8 mEntryNumber;

static int ICACHE_FLASH_ATTR cbMessageFillReply(struct jsontree_context *pCtx) {
	static char *lTagName[] = { "result", "version", "sdk-version", "date",
			"name", "descr", "model", "status", "time-on", "loglevel", "button", "text", "ssid", "password", "mac", "auto-off", "serverip", "serverport", "number", "current", "time"};
	const char *lPad;
	char lWorkStr[17];
	uint32 lTime;
	char lTimeStr[20];
	int lTag;
	int lNumber;

	lPad = jsontree_path_name(pCtx, pCtx->depth - 1);

	lNumber = sizeof(lTagName) / sizeof(lTagName[0]);
	for (lTag = 0; lTag < lNumber; lTag++) {
		if (xStrnCmpX(lPad, lTagName[lTag], os_strlen(lTagName[lTag])) == 0) {
			break;
		}
	}
	switch (lTag) {
	case 0:
		jsontree_write_string(pCtx, mProcessOk ? "OK" : "NOK");
		break;
	case 1:
		jsontree_write_string(pCtx, VERSION);
		break;
	case 2:
		jsontree_write_string(pCtx, (char*) system_get_sdk_version());
		break;
	case 3:
		jsontree_write_string(pCtx, __DATE__);
		break;
	case 4:
		jsontree_write_string(pCtx, xSettingName());
		break;
	case 5:
		jsontree_write_string(pCtx, xSettingDescription());
		break;
	case 6:
		jsontree_write_int(pCtx, xSettingSwitchModel());
		break;
	case 7:
		jsontree_write_string(pCtx, xSwitchStatus() ? "on" : "off");
		break;
	case 8:
		jsontree_write_int(pCtx, xTimeOn());
		break;
	case 9:
		jsontree_write_int(pCtx, xSettingLogLevel());
		break;
	case 10:
		jsontree_write_string(pCtx, xSettingButton() ? "on" : "off");
		break;
	case 11:
		jsontree_write_string(pCtx, mText);
		break;
	case 12:
		jsontree_write_string(pCtx, xSettingSsId());
		break;
	case 13:
		jsontree_write_string(pCtx, xSettingPassword());
		break;
	case 14:
		xSettingMac(lWorkStr);
		jsontree_write_string(pCtx, lWorkStr);
		break;
	case 15:
		jsontree_write_int(pCtx, xSettingAutoOff());
		break;
	case 16:
		xSettingServerIpDisp(lWorkStr);
		jsontree_write_string(pCtx, lWorkStr);
		break;
	case 17:
		jsontree_write_int(pCtx, xSettingServerPort());
		break;
	case 18:
		jsontree_write_int(pCtx, xLogNumber());
		break;
	case 19:
		jsontree_write_int(pCtx, xLogCurrent());
		break;
	case 20:
		lTime = xTimeNow();
		xTimeString(lTime, lTimeStr, sizeof(lTimeStr));
		jsontree_write_string(pCtx, lTimeStr);
		break;
	default:
		break;
	}
	return 0;
}

LOCAL int ICACHE_FLASH_ATTR cbMessageFillLogEntry(struct jsontree_context *pCtx) {
	static char *lTagName[] = { "entry", "action", "time", "ip" };
	const char *lPad;
	uint32 lTime;
	char lTimeStr[20];
	uint32 lIp;
	char lIpStr[12];
	int lTag;
	int lNumber;

	mEntryNumber++;
	if (mEntryNumber > 4) {
		mEntryNumber = 1;
		mEntry--;
		if (mEntry >= LOG_NUMBER_ENTRIES) {
			mEntry = LOG_NUMBER_ENTRIES - 1;
		}
	}

	lPad = jsontree_path_name(pCtx, pCtx->depth - 1);

	lNumber = sizeof(lTagName) / sizeof(lTagName[0]);
	for (lTag = 0; lTag < lNumber; lTag++) {
		if (xStrnCmpX(lPad, lTagName[lTag], os_strlen(lTagName[lTag])) == 0) {
			break;
		}
	}
	switch (lTag) {
	case 0:
		jsontree_write_int(pCtx, mEntry);
		break;
	case 1:
		jsontree_write_string(pCtx, (char*) xLogActionText(mEntry));
		break;
	case 2:
		lTime = xLogTime(mEntry);
		xTimeString(lTime, lTimeStr, sizeof(lTimeStr));
		jsontree_write_string(pCtx, lTimeStr);
		break;
	case 3:
		lIp = xLogIp(mEntry);
		os_sprintf(lIpStr, IPSTR, IP2STR(&lIp));
		jsontree_write_string(pCtx, lIpStr);
		break;
	default:
		break;
	}
	return 0;
}

static int ICACHE_FLASH_ATTR sMessageWriteChar(int pChar) {
	if (mBuffer != NULL && mPos <= mSize) {
		mBuffer[mPos] = pChar;
		mPos++;
		return pChar;
	}

	return 0;
}

void ICACHE_FLASH_ATTR sMessageSettingReply(char *pMessage) {
	bool lContinue;
	int lResult;
	JSONTREE_OBJECT(lResult_Tree,
			JSONTREE_PAIR("result", &mMessageFillReply_callback),
			JSONTREE_PAIR("ssid", &mMessageFillReply_callback),
			JSONTREE_PAIR("password", &mMessageFillReply_callback),
			JSONTREE_PAIR("mac", &mMessageFillReply_callback),
			JSONTREE_PAIR("name", &mMessageFillReply_callback),
			JSONTREE_PAIR("descr", &mMessageFillReply_callback),
			JSONTREE_PAIR("model", &mMessageFillReply_callback),
			JSONTREE_PAIR("loglevel", &mMessageFillReply_callback),
			JSONTREE_PAIR("button", &mMessageFillReply_callback),
			JSONTREE_PAIR("auto-off", &mMessageFillReply_callback),
			JSONTREE_PAIR("serverip", &mMessageFillReply_callback),
			JSONTREE_PAIR("serverport", &mMessageFillReply_callback));

	mBuffer = pMessage;
	mPos = 0;
	mSize = MESSAGE_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value*) &lResult_Tree,
			sMessageWriteChar);

	lContinue = true;
	while (lContinue) {
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0) {
			lContinue = false;
		}
	}
	mBuffer[mPos] = 0;
}

void ICACHE_FLASH_ATTR sMessageReply(char *pMessage) {
	bool lContinue;
	int lResult;
	JSONTREE_OBJECT(lResult_Tree,
			JSONTREE_PAIR("result", &mMessageFillReply_callback),
			JSONTREE_PAIR("version", &mMessageFillReply_callback),
			JSONTREE_PAIR("sdk-version", &mMessageFillReply_callback),
			JSONTREE_PAIR("date", &mMessageFillReply_callback),
			JSONTREE_PAIR("name", &mMessageFillReply_callback),
			JSONTREE_PAIR("descr", &mMessageFillReply_callback),
			JSONTREE_PAIR("model", &mMessageFillReply_callback),
			JSONTREE_PAIR("status", &mMessageFillReply_callback),
			JSONTREE_PAIR("time-on", &mMessageFillReply_callback),
			JSONTREE_PAIR("loglevel", &mMessageFillReply_callback),
			JSONTREE_PAIR("button", &mMessageFillReply_callback));

	mBuffer = pMessage;
	mPos = 0;
	mSize = MESSAGE_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value*) &lResult_Tree,
			sMessageWriteChar);

	lContinue = true;
	while (lContinue) {
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0) {
			lContinue = false;
		}
	}
	mBuffer[mPos] = 0;
}

static void ICACHE_FLASH_ATTR sMessageErrorReply(char *pMessage) {
	bool lContinue;
	int lResult;
	JSONTREE_OBJECT(lResult_Tree,
			JSONTREE_PAIR("result", &mMessageFillReply_callback),
			JSONTREE_PAIR("text", &mMessageFillReply_callback));

	mBuffer = pMessage;
	mPos = 0;
	mSize = MESSAGE_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value*) &lResult_Tree,
			sMessageWriteChar);

	lContinue = true;
	while (lContinue) {
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0) {
			lContinue = false;
		}
	}
	mBuffer[mPos] = 0;
}

void ICACHE_FLASH_ATTR sMessageLogReply(int pStart, char *pMessage) {
	bool lContinue;
	int lResult;
	JSONTREE_OBJECT(lLogEntry_Tree,
			JSONTREE_PAIR("entry", &mMessageFillLogEntry_callback),
			JSONTREE_PAIR("action", &mMessageFillLogEntry_callback),
			JSONTREE_PAIR("time", &mMessageFillLogEntry_callback),
			JSONTREE_PAIR("ip", &mMessageFillLogEntry_callback));
	JSONTREE_ARRAY(lLog_Array, (struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree,
			(struct jsontree_value* )&lLogEntry_Tree);
	JSONTREE_OBJECT(lResult_Tree,
			JSONTREE_PAIR("result", &mMessageFillReply_callback),
			JSONTREE_PAIR("number", &mMessageFillReply_callback),
			JSONTREE_PAIR("current", &mMessageFillReply_callback),
			JSONTREE_PAIR("time", &mMessageFillReply_callback),
			JSONTREE_PAIR("log", &lLog_Array));

	mBuffer = pMessage;
	if (pStart < 0) {
		mEntry = xLogCurrent() - 1;
	} else {
		mEntry = (uint8) pStart;
	}
	if (mEntry >= LOG_NUMBER_ENTRIES) {
		mEntry = LOG_NUMBER_ENTRIES - 1;
	}
	mEntryNumber = 0;
	mPos = 0;
	mSize = MESSAGE_SIZE;
	jsontree_setup(&mCtx, (struct jsontree_value*) &lResult_Tree,
			sMessageWriteChar);

	lContinue = true;
	while (lContinue) {
		lResult = jsontree_print_next(&mCtx);
		if (lResult == 0) {
			lContinue = false;
		}
	}
	mBuffer[mPos] = 0;
}

int ICACHE_FLASH_ATTR sMessageActionParse(char *pMessage) {
	struct jsonparse_state lParseState;
	int lType;
	int lType2;
	char lBuffer[64];
	enum action lAction;

	lAction = ActionNone;
	mAutoOff = -1;
	jsonparse_setup(&lParseState, pMessage, os_strlen(pMessage));
	while ((lType = jsonparse_next(&lParseState)) != 0) {
		if (lType == JSON_TYPE_PAIR_NAME) {
			os_bzero(lBuffer, 64);
			jsonparse_copy_value(&lParseState, lBuffer, sizeof(lBuffer));
			if (xStrCmpX(lBuffer, "status") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lBuffer, 64);
						jsonparse_copy_value(&lParseState, lBuffer,
								sizeof(lBuffer));
						if (xStrCmpX(lBuffer, "on") == 0) {
							lAction = ActionOn;
						} else {
							if (xStrCmpX(lBuffer, "off") == 0) {
								lAction = ActionOff;
							} else {
								lAction = ActionNone;
							}
						}
					}
				}
			}
			if (xStrCmpX(lBuffer, "auto-off") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_NUMBER) {
						mAutoOff = jsonparse_get_value_as_int(&lParseState);
					}
				}
			}
		}
	}
	return lAction;
}

static uint8 ICACHE_FLASH_ATTR sHexToInt(char pHex) {
	uint8 lInt;

	if (pHex >= '0' && pHex <= '9') {
		lInt = pHex - '0';
	} else {
		if (pHex >= 'a' && pHex <= 'f') {
			lInt = pHex - 'a' + 10;
		} else {
			if (pHex >= 'A' && pHex <= 'F') {
				lInt = pHex - 'A' + 10;
			} else {
				lInt = 16;
			}
		}
	}
	return lInt;
}

static ICACHE_FLASH_ATTR sParseIp(const char *pIpString, uint8 pIp[4]) {
	const char *lPos;
	int lVolgNr;
	bool lInit;

	lPos = pIpString;
	lVolgNr = 0;
	lInit = true;
	while (*lPos != '\0') {
		if (lInit == true) {
			pIp[lVolgNr] = 0;
			lInit = false;
		}
		if (isdigit(*lPos)) {
			pIp[lVolgNr] *= 10;
			pIp[lVolgNr] += *lPos - '0';
		} else {
			if (*lPos == '.') {
				lVolgNr++;
				if (lVolgNr < 4) {
					lInit = true;
				} else {
					lVolgNr = 3;
				}
			}
		}
		lPos++;
	}
}

static void ICACHE_FLASH_ATTR sMessageSetSetting(char *pMessage) {
	struct setting *lSetting;
	char *lBuffer;
	char *lMacBuffer;
	struct jsonparse_state lParseState;
	int lType;
	int lType2;
	int lCount;
	uint8 lInt;
	uint8 lByte;
	uint8 lMac[6];
	uint8 lIp[4];
	bool lMacOk;
	bool lSettingReset;

	lSetting = (struct setting*) os_zalloc(sizeof(struct setting));
	lBuffer = (char*) os_zalloc(64);
	xSettingCopy(lSetting);
	lSettingReset = false;

	jsonparse_setup(&lParseState, pMessage, os_strlen(pMessage));
	while ((lType = jsonparse_next(&lParseState)) != 0) {
		if (lType == JSON_TYPE_PAIR_NAME) {
			os_bzero(lBuffer, 64);
			jsonparse_copy_value(&lParseState, lBuffer, 64);
			if (xStrCmpX(lBuffer, "ssid") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lSetting->sSsId, sizeof(lSetting->sSsId));
						jsonparse_copy_value(&lParseState, lSetting->sSsId,
								sizeof(lSetting->sSsId));
					}
				}
			}
			if (xStrCmpX(lBuffer, "password") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lSetting->sPassword,
								sizeof(lSetting->sPassword));
						jsonparse_copy_value(&lParseState, lSetting->sPassword,
								sizeof(lSetting->sPassword));
					}
				}
			}
			if (xStrCmpX(lBuffer, "mac") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lBuffer, 64);
						jsonparse_copy_value(&lParseState, lBuffer, 64);
						if (lBuffer[17] == '\0') {
							if (lBuffer[2] == ':' && lBuffer[5] == ':'
									&& lBuffer[8] == ':' && lBuffer[11] == ':'
									&& lBuffer[14] == ':') {
								os_bzero(lMac, 6);
								lMacOk = true;
								lMacBuffer = lBuffer;
								for (lCount = 0; lCount < 6; lCount++) {
									lInt = sHexToInt(lMacBuffer[0]);
									if (lInt > 15) {
										lMacOk = false;
										break;
									}
									lByte = lInt << 4;

									lInt = sHexToInt(lMacBuffer[1]);
									if (lInt > 15) {
										lMacOk = false;
										break;
									}
									lByte |= lInt;
									lMac[lCount] = lByte;

									lMacBuffer += 3;
								}
								if (lMacOk) {
									os_memcpy(lSetting->sMac, lMac, 6);
								}
							}
						}
					}
				}
			}
			if (xStrCmpX(lBuffer, "name") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lSetting->sName, sizeof(lSetting->sName));
						jsonparse_copy_value(&lParseState, lSetting->sName,
								sizeof(lSetting->sName));
					}
				}
			}
			if (xStrCmpX(lBuffer, "descr") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lSetting->sDescription,
								sizeof(lSetting->sDescription));
						jsonparse_copy_value(&lParseState,
								lSetting->sDescription,
								sizeof(lSetting->sDescription));
					}
				}
			}
			if (xStrCmpX(lBuffer, "model") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_NUMBER) {
						lSetting->sSwitchModel = (uint8)jsonparse_get_value_as_int(
								&lParseState);
					}
				}
			}
			if (xStrCmpX(lBuffer, "loglevel") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_NUMBER) {
						lSetting->sLogLevel = jsonparse_get_value_as_int(
								&lParseState);
					}
				}
			}
			if (xStrCmpX(lBuffer, "button") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lBuffer, 64);
						jsonparse_copy_value(&lParseState, lBuffer, 64);
						if (xStrCmpX(lBuffer, "on") == 0) {
							lSetting->sButton = true;
						} else {
							if (xStrCmpX(lBuffer, "off") == 0) {
								lSetting->sButton = false;
							}
						}
					}
				}
			}
			if (xStrCmpX(lBuffer, "auto-off") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_NUMBER) {
						lSetting->sAutoOff = jsonparse_get_value_as_int(
								&lParseState);
					}
				}
			}
			if (xStrCmpX(lBuffer, "serverip") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lBuffer, 64);
						jsonparse_copy_value(&lParseState, lBuffer, 64);
						sParseIp(lBuffer, lIp);
						os_memcpy(lSetting->sServerIP, lIp, 4);
					}
				}
			}
			if (xStrCmpX(lBuffer, "serverport") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_NUMBER) {
						lSetting->sServerPort = jsonparse_get_value_as_int(
								&lParseState);
					}
				}
			}

			if (xStrCmpX(lBuffer, "reset") == 0) {
				lType2 = jsonparse_next(&lParseState);
				if (lType2 == JSON_TYPE_PAIR) {
					lType2 = jsonparse_next(&lParseState);
					if (lType2 == JSON_TYPE_STRING) {
						os_bzero(lBuffer, 64);
						jsonparse_copy_value(&lParseState, lBuffer, 64);
						if (xStrCmpX(lBuffer, "true") == 0) {
							lSettingReset = true;
						}
					}
				}
			}
		}
	}

	if (lSettingReset) {
		xSettingReset();
	} else {
		xSettingSave(lSetting);
		xButtonSet();
	}

	os_free(lSetting);
	os_free(lBuffer);
}

static bool ICACHE_FLASH_ATTR sMessageFindVersion(char **pUriPtr) {
	char *lUriPtr;
	char *lStart;
	bool lEnd;
	int lLength;
	bool lOK;

	lUriPtr = *pUriPtr;
	lUriPtr++;
	if (xStrnCmpX(lUriPtr, "version=", 8) == 0) {
		lUriPtr += 8;
		lStart = lUriPtr;
		lEnd = false;
		lLength = 0;
		do {
			if (*lUriPtr == '\0' || *lUriPtr == '&') {
				lEnd = true;
			} else {
				lLength++;
				lUriPtr++;
			}
		} while (!lEnd);
		if (lLength < 6 || lLength > 12) {
			lOK = false;
		} else {
			os_memcpy(mVersion, lStart, lLength);
			mVersion[lLength] = '\0';
			if (*lUriPtr == '&') {
				lUriPtr++;
				if (xStrnCmpX(lUriPtr, "force", 5) == 0
						&& *(lUriPtr + 5) == '\0') {
					mForce = true;
					lOK = true;
				} else {
					lOK = false;
				}
			} else {
				mForce = false;
				lOK = true;
			}
		}
	} else {
		lOK = false;
	}
	return lOK;
}

static int ICACHE_FLASH_ATTR sMessageFindStart(char **pUriPtr) {
	char *lUriPtr;
	int lStart;

	lUriPtr = *pUriPtr;
	lStart = -1;
	lUriPtr++;
	if (xStrnCmpX(lUriPtr, "start=", 6) == 0) {
		lUriPtr += 6;
		lStart = atoi(lUriPtr);
	}
	return lStart;
}

static enum segment ICACHE_FLASH_ATTR sMessageUriSegment(char **pUriPtr) {
	char *lUriPtr;
	char lPos;
	char *lStart;
	enum segment lResult;
	int lLength;
	bool lEnd;

	lUriPtr = *pUriPtr;
	lPos = *lUriPtr;
	switch (lPos) {
	case '/':
		lUriPtr++;
		lStart = lUriPtr;
		lEnd = false;
		lLength = 0;
		do {
			if (*lUriPtr == '/' || *lUriPtr == '?' || *lUriPtr == '\0') {
				lEnd = true;
			} else {
				lLength++;
				lUriPtr++;
			}
		} while (!lEnd);
		*pUriPtr = lUriPtr;
		lResult = SegmentError;
		if (lLength == 3) {
			if (xStrnCmpX(lStart, "log", lLength) == 0) {
				lResult = SegmentLog;
			}
		}
		if (lLength == 7) {
			if (xStrnCmpX(lStart, "setting", lLength) == 0) {
				lResult = SegmentSetting;
			}
		}
		if (lLength == 6) {
			if (xStrnCmpX(lStart, "switch", lLength) == 0) {
				lResult = SegmentSwitch;
			}
		}
		if (lLength == 7) {
			if (xStrnCmpX(lStart, "upgrade", lLength) == 0) {
				lResult = SegmentUpgrade;
			}
		}
		if (lLength == 7) {
			if (xStrnCmpX(lStart, "restart", lLength) == 0) {
				lResult = SegmentRestart;
			}
		}
		if (lLength >= 7) {
			if (xStrnCmpX(lStart, "favicon", 7) == 0) {
				lResult = SegmentFavicon;
			}
		}
		break;
	case '\0':
		lResult = SegmentEnd;
		break;
	case '?':
		lResult = SegmentData;
		break;
	default:
		lResult = SegmentError;
		break;
	}
	return lResult;
}

static enum uri ICACHE_FLASH_ATTR sMessageTestUriSwitch(char **pUriPtr) {
	enum segment lSegmentType;
	enum uri lUriType;

	lSegmentType = sMessageUriSegment(pUriPtr);
	switch (lSegmentType) {
	case SegmentEnd:
		lUriType = UriBasis;
		break;
	case SegmentLog:
		lSegmentType = sMessageUriSegment(pUriPtr);
		if (lSegmentType == SegmentEnd) {
			lUriType = UriLog;
		} else {
			if (lSegmentType == SegmentData) {
				mStart = sMessageFindStart(pUriPtr);
				if (mStart < 0) {
					lUriType = UriError;
				} else {
					lUriType = UriLogPar;
				}
			} else {
				lUriType = UriError;
			}
		}
		break;
	case SegmentSetting:
		lSegmentType = sMessageUriSegment(pUriPtr);
		if (lSegmentType == SegmentEnd) {
			lUriType = UriSetting;
		} else {
			lUriType = UriError;
		}
		break;
	case SegmentUpgrade:
		lSegmentType = sMessageUriSegment(pUriPtr);
		if (lSegmentType == SegmentEnd) {
			lUriType = UriError;
		} else {
			if (lSegmentType == SegmentData) {
				if (sMessageFindVersion(pUriPtr)) {
					lUriType = UriUpgrade;
				} else {
					lUriType = UriError;
				}
			} else {
				lUriType = UriError;
			}
		}
		break;
	case SegmentRestart:
		lSegmentType = sMessageUriSegment(pUriPtr);
		if (lSegmentType == SegmentEnd) {
			lUriType = UriRestart;
		} else {
			lUriType = UriError;
		}
		break;
	default:
		lUriType = UriError;
		break;
	}
	return lUriType;
}

static int ICACHE_FLASH_ATTR sMessageTestUri(char *pUri) {
	char *lUriPtr;
	int lUriType;
	int lSegmentType;

	lUriPtr = pUri;
	lSegmentType = sMessageUriSegment(&lUriPtr);
	if (lSegmentType == SegmentSwitch) {
		lUriType = sMessageTestUriSwitch(&lUriPtr);
	} else {
		if (lSegmentType == SegmentFavicon) {
			lUriType = UriFavicon;
		} else {
			lUriType = UriError;
		}
	}
	return lUriType;
}

void ICACHE_FLASH_ATTR xMessageMakeErrorReply(char *pText, char *pMessage) {
	mProcessOk = false;
	os_strcpy(mText, pText);

	sMessageErrorReply(pMessage);
}

static bool ICACHE_FLASH_ATTR sParseVersion(struct version *pVersion,
		const char *pVersionStr) {
	const char *lPos;
	int lVolgNr;
	bool lResult;
	uint16 lVersionPart[3] = { 0 };

	lPos = pVersionStr;
	lVolgNr = 0;
	if (*lPos == 'v' || *lPos == 'V') {
		lPos++;
		lResult = true;
		while (*lPos != '\0') {
			if (isdigit(*lPos)) {
				lVersionPart[lVolgNr] *= 10;
				lVersionPart[lVolgNr] += *lPos - '0';
			} else {
				if (*lPos == '.') {
					lVolgNr++;
					if (lVolgNr >= 3) {
						lResult = false;
						break;
					}
				} else {
					lResult = false;
				}
			}
			lPos++;
		}
	} else {
		lResult = false;
	}
	if (lResult == true) {
		pVersion->sMajor = lVersionPart[0];
		pVersion->sMinor = lVersionPart[1];
		pVersion->sRevision = lVersionPart[2];
	}
	return lResult;
}

static int sCompareVersion(struct version *pVersion1, struct version *pVersion2) {
	int lResult;

	lResult = pVersion1->sMajor - pVersion2->sMajor;
	if (lResult == 0) {
		lResult = pVersion1->sMinor - pVersion2->sMinor;
		if (lResult == 0) {
			lResult = pVersion1->sRevision - pVersion2->sRevision;
		}
	}
	return lResult;
}

static enum upgradetestresult ICACHE_FLASH_ATTR sTestUpgrade() {
	struct version lVersionOld;
	struct version lVersionNew;
	int lCmp;
	bool lParseResult;
	enum upgradetestresult lTestResult;

	lParseResult = sParseVersion(&lVersionOld, VERSION);
	if (lParseResult == true) {
		lParseResult = sParseVersion(&lVersionNew, mVersion);
		if (lParseResult == true) {
			lCmp = sCompareVersion(&lVersionNew, &lVersionOld);
			if (lCmp > 0) {
				lTestResult = UpgradeTestOK;
			} else {
				if (lCmp == 0) {
					lTestResult = UpgradeTestVersionEqual;
				} else {
					lTestResult = UpgradeTestVersionLow;
				}
			}
		} else {
			lTestResult = UpgradeTestVersionNewError;
		}
	} else {
		lTestResult = UpgradeTestVersionOldError;
	}

	return lTestResult;
}

void ICACHE_FLASH_ATTR eMessageProcess(struct HttpConnectionSlot *pSlot) {
	enum uri lUriType;
	enum action lAction;
	int lStart;
	enum upgradetestresult lTestResult;
	bool lUpgrade;
	bool lRestart;

	lUpgrade = false;
	lRestart = false;
	if (os_strcmp(pSlot->sVerb, "GET") == 0) {
		lUriType = sMessageTestUri(pSlot->sUri);
		switch (lUriType) {
		case UriBasis:
			mProcessOk = true;
			sMessageReply(pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_OK;
			xLogEntry(LOG_GET_SWITCH, pSlot->sRemoteIp.addr);
			break;
		case UriSetting:
			mProcessOk = true;
			sMessageSettingReply(pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_OK;
			xLogEntry(LOG_GET_SETTING, pSlot->sRemoteIp.addr);
			break;
		case UriLog:
			mProcessOk = true;
			sMessageLogReply(-1, pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_OK;
			break;
		case UriLogPar:
			mProcessOk = true;
			sMessageLogReply(mStart, pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_OK;
			break;
		case UriUpgrade:
			if (mForce == true) {
				ets_uart_printf("Force\r\n");
				mProcessOk = true;
				os_sprintf(mText, "Force Upgrade requested to version %s",
						mVersion);
				sMessageErrorReply(pSlot->sAnswer);
				pSlot->sProcessResult = HTTP_OK;
				xLogEntry(LOG_UPGRADE, pSlot->sRemoteIp.addr);
				lUpgrade = true;
			} else {
				lTestResult = sTestUpgrade();
				switch (lTestResult) {
				case UpgradeTestOK:
					os_sprintf(mText, "Upgrade to version %s", mVersion);
					ets_uart_printf("Upgrade OK\r\n");
					mProcessOk = true;
					xLogEntry(LOG_UPGRADE, pSlot->sRemoteIp.addr);
					lUpgrade = true;
					break;
				case UpgradeTestVersionLow:
					os_strcpy(mText,
							"Upgrade: Requested version before current one");
					mProcessOk = false;
					break;
				case UpgradeTestVersionEqual:
					os_strcpy(mText,
							"Upgrade: Requested version already installed");
					mProcessOk = false;
					break;
				case UpgradeTestVersionOldError:
					os_strcpy(mText, "Upgrade: Old version wrong, try force");
					mProcessOk = false;
					break;
				case UpgradeTestVersionNewError:
					os_strcpy(mText, "Upgrade: Version wrong");
					mProcessOk = false;
					break;
				default:
					os_strcpy(mText, "Upgrade: Undefined error");
					mProcessOk = false;
					break;

				}
				sMessageErrorReply(pSlot->sAnswer);
				pSlot->sProcessResult = HTTP_OK;
			}
			break;
		case UriRestart:
			ets_uart_printf("Restart\r\n");
			mProcessOk = true;
			os_sprintf(mText, "Restart requested");
			sMessageErrorReply(pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_OK;
			xLogEntry(LOG_RESTART, pSlot->sRemoteIp.addr);
			lRestart = true;
			break;
		default:
			mProcessOk = false;
			os_strcpy(mText, "URI Invalid");
			sMessageErrorReply(pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_NOTFOUND;
			if (lUriType != UriFavicon) {
				xLogEntry(LOG_GET_ERROR, pSlot->sRemoteIp.addr);
			}
			break;
		}
	} else {
		if (os_strcmp(pSlot->sVerb, "PUT") == 0) {
			lUriType = sMessageTestUri(pSlot->sUri);
			switch (lUriType) {
			case UriBasis:
				lAction = sMessageActionParse(pSlot->sBuffer);
				if (lAction == ActionOn) {
					xSwitchOn(mAutoOff);
					mProcessOk = true;
					sMessageReply(pSlot->sAnswer);
					pSlot->sProcessResult = HTTP_OK;
					xLogEntry(LOG_PUT_SWITCH_ON, pSlot->sRemoteIp);
				} else {
					if (lAction == ActionOff) {
						xSwitchOff();
						mProcessOk = true;
						sMessageReply(pSlot->sAnswer);
						pSlot->sProcessResult = HTTP_OK;
						xLogEntry(LOG_PUT_SWITCH_OFF, pSlot->sRemoteIp);
					} else {
						mProcessOk = false;
						os_strcpy(mText, "Incorrect status");
						sMessageErrorReply(pSlot->sAnswer);
						pSlot->sProcessResult = HTTP_BADREQUEST;
						xLogEntry(LOG_PUT_SWITCH_ERROR, pSlot->sRemoteIp);
					}
				}
				break;
			case UriSetting:
				sMessageSetSetting(pSlot->sBuffer);
				mProcessOk = true;
				sMessageSettingReply(pSlot->sAnswer);
				pSlot->sProcessResult = HTTP_OK;
				xLogEntry(LOG_PUT_SETTING, pSlot->sRemoteIp);
				break;
			default:
				mProcessOk = false;
				os_strcpy(mText, "URI Invalid");
				sMessageErrorReply(pSlot->sAnswer);
				pSlot->sProcessResult = HTTP_NOTFOUND;
				if (lUriType != UriFavicon) {
					xLogEntry(LOG_PUT_ERROR, pSlot->sRemoteIp);
				}
				break;
			}
		} else {
			mProcessOk = false;
			os_strcpy(mText, "Verb not (yet) supported");
			sMessageErrorReply(pSlot->sAnswer);
			pSlot->sProcessResult = HTTP_NOTALLOWED;
			xLogEntry(LOG_VERB_ERROR, pSlot->sRemoteIp.addr);
		}
	}
	system_os_post(0, EventMessageProcessed, pSlot);
	if (lUpgrade) {
		system_os_post(0, EventStartUpgrade, mVersion);
	}
	if (lRestart) {
		system_os_post(0, EventRestart, mVersion);
	}
}

