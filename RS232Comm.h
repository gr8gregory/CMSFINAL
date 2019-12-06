#pragma once

// Prototype the functions to be used

#include <Windows.h>
#include "sound.h"


extern int flagDiag;
extern int flagMode;

#define _CRT_SECURE_NO_WARNINGS

void purgePort(HANDLE hCom);
void outputToPort(LPCVOID buf, DWORD szBuf, int type);
int inputFromPort(LPVOID buf, DWORD szBuf, int type);

void recieveMessage(void);					// Port specific
void send_message(int status);				// Port specific
void SendMultipleCookies(void);				// Port specific
void randMessage(char* msg);				// Port specific

// Sub functions
void createPortFileRec(void);				// Port specific
void createPortFileSend(void);				// Port specific
int SetComParms(HANDLE hCom, int type);		// Port General


// Initializes the port and sets the communication parameters

#define EX_FATAL 1
#define BUFSIZE 140

#define TXTBUFF sizeof(MssgTxt)
							// A commtimout struct variable
#define RID 000

void setupPort(void);

void initPortT(int type);
void initPortR(int type);

void SendAudio(void);
void RecieveAudio(void);

void TransmitQ(void);

