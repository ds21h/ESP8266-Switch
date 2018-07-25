/*
 * logger.h
 *
 *  Created on: 9 jun. 2017
 *      Author: Jan
 */

#ifndef USER_LOGGER_H_
#define USER_LOGGER_H_

#define LOG_NUMBER_ENTRIES	250

#define LOG_INIT				1
#define LOG_RESET				2
#define LOG_RESET_READERR		3
#define LOG_SET					4
#define LOG_GET_SWITCH			10
#define LOG_GET_SETTING			11
#define LOG_GET_ERROR			19
#define LOG_PUT_SWITCH_ON		20
#define LOG_PUT_SWITCH_OFF		21
#define LOG_PUT_SWITCH_FLIP		22
#define LOG_PUT_SWITCH_ERROR	29
#define LOG_PUT_BUTTON_ON		30
#define LOG_PUT_BUTTON_OFF		31
#define LOG_PUT_BUTTON_ERROR	39
#define LOG_PUT_SETTING			40
#define LOG_PUT_ERROR			50
#define LOG_VERB_ERROR			60
#define LOG_UPGRADE				70

void xLogInit();
void xLogSetLevel();
uint8 xLogNumber();
uint8 xLogCurrent();
uint8 xLogAction(uint8 pEntry);
const char* xLogActionText(uint8 pEntry);
uint32 xLogTime(uint8 pEntry);
uint32 xLogIp(uint8 pEntry);

#endif /* USER_LOGGER_H_ */
