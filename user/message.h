/*
 * bericht.h
 *
 *  Created on: 9 jun. 2017
 *      Author: Jan
 */

#ifndef USER_MESSAGE_H_
#define USER_MESSAGE_H_

#include <c_types.h>

#define MESSAGE_VERB_SIZE		7
#define MESSAGE_URI_SIZE		256
#define MESSAGE_SIZE			1024

struct message {
	char sVerb[MESSAGE_VERB_SIZE];
	char sUri[MESSAGE_URI_SIZE];
	char sMessage[MESSAGE_SIZE];
};

void xMessageMakeErrorReply(char *pText, char *pMessage);
void xMessageProcess(uint32 pIp, struct message *pMessage);

#endif /* USER_MESSAGE_H_ */
