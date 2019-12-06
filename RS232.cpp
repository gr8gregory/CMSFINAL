#include "OperatingSystem.h"


//#include <Windows.h>    // Includes the functions for serial communication via RS232
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "RS232Comm.h"
#include "menu.h"
#include "queues.h"
#include "Frame.h"
#include "Multithreading.h"
#include "phonebook.h"
#include "Heap.h"

short iBigBuf[SAMPLES_SEC * RECORD_TIME];
long lBigBufSize = SAMPLES_SEC * RECORD_TIME;

int SettingFlag;
int recordTime;
HANDLE RCom;										// Pointer to recieving COM port
HANDLE TCom;										// Pointer to sending COM port
int nComRate = 9600;								// Baud (Bit) rate in bits/second 
int nComBits = 8;									// Number of bits per frame
COMMTIMEOUTS timeout;

char COMPORTT[5] = "COM7";
char COMPORTR[5] = "COM5";
int flagDiag = false;
int flagMode = false;

void setupPort(void) {

	char input;
	int error = 1;

	while (error == 1) {
		printf("Select the number of ports to use: 1 or 2\n");
		input = getchar();
		if (input == '1') {
			strcpy_s(COMPORTT, COMPORTR);
			printf("Using %s (%s) ONLY\n", COMPORTR, COMPORTT);
			error = 0;
		}
		else if (input == '2') {
			flagDiag = true;
			printf("Using the ports %s and %s (DIAGNOSTIC MODE)\n", COMPORTR, COMPORTT);
			error = 0;
		}
		else {
			printf("Invalid input\n");
			error = 1;
		}
	}
	system("pause");	// Make it so they can read the screen
	while ((input = getchar()) != '\n' && (input != EOF)) {}				//flushes input 
	system("cls");		// Clear the screen
}

// com 0 com transmission
//#define COMPORT "COM4"
// Ensure that default character set is not Unicode
// Communication variables and parameters

// Initializes the port and sets the communication parameters
void initPortT(int type) {										// Transmit only
	createPortFileSend();							// Initializes hCom to point to PORT4 (port 4 is used by USB-Serial adapter on my laptop)
	purgePort(TCom);									// Purges the COM port
	SetComParms(TCom, type);									// Uses the DCB structure to set up the COM port
	purgePort(TCom);
}

void initPortR(int type) {										// Recieve only
	createPortFileRec();							// Initializes hCom to point to PORT4 (port 4 is used by USB-Serial adapter on my laptop)
	purgePort(RCom);									// Purges the COM port
	SetComParms(RCom, type);									// Uses the DCB structure to set up the COM port
	purgePort(RCom);
}

// Purge any outstanding requests on the serial port (initialize)
void purgePort(HANDLE hCom) {														// function for either port
	PurgeComm(hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
}

// Output message to port (COM4)
void outputToPort(LPCVOID buf, DWORD szBuf, int type) {										// Transmit only
	int i = 0;
	initPortT(type);
	DWORD NumberofBytesTransmitted;
	LPDWORD lpErrors = 0;
	LPCOMSTAT lpStat = 0;

	i = WriteFile(
		TCom,										// Write handle pointing to transmission port
		buf,										// Buffer (string)
		szBuf,										// Size of buffer
		&NumberofBytesTransmitted,					// Written number of bytes
		NULL
	);

	// Handle the timeout error
	if (i == 0) {
		printf("\nTx Port Failed (%d): Error # 0x%x\n", type, GetLastError());
		ClearCommError(TCom, lpErrors, lpStat);		// Clears the device error flag to enable additional input and output operations. Retrieves information ofthe communications error.	
	}
	else {
		printf("\nSuccessful transmission, there were %ld bytes transmitted\n", NumberofBytesTransmitted);
	}
}

// Dont use in this file - This file only outputs not inputs

int inputFromPort(LPVOID buf, DWORD szBuf, int type) {								// Recieve only function
	int i = 0;
	initPortR(type);
	DWORD NumberofBytesRead;
	LPDWORD lpErrors = 0;
	LPCOMSTAT lpStat = 0;

	i = ReadFile(
		RCom,										// Read handle pointing to recieving COM port
		buf,										// Buffer size
		szBuf,  									// Size of buffer - Maximum number of bytes to read
		&NumberofBytesRead,
		NULL
	);

	// Handle the timeout error
	if (i == 0) {
		printf("\nRx Port Failed (%d): Error # 0x%x\n", type, GetLastError());
		ClearCommError(RCom, lpErrors, lpStat);		// Clears the device error flag to enable additional input and output operations. Retrieves information ofthe communications error.
	}
	else {
		//printf("\nSuccessful reception!, There were %ld bytes read\n", NumberofBytesRead);
		return(NumberofBytesRead);
	}
	return 0;
}

// Sub functions called by above functions
/**************************************************************************************/

// Set the hCom HANDLE to point to a COM port, initialize for reading and writing, open the port and set securities
void createPortFileRec() {				// Recieve only
	// Call the CreateFile() function 
	RCom = CreateFile(
		COMPORTR,									// COM port number  --> If COM# is larger than 9 then use the following syntax--> "\\\\.\\COM10"
		GENERIC_READ | GENERIC_WRITE,				// Open for read and write
		NULL,										// No sharing allowed
		NULL,										// No security
		OPEN_EXISTING,								// Opens the existing com port
		FILE_ATTRIBUTE_NORMAL,						// Do not set any file attributes --> Use synchronous operation
		NULL										// No template
	);

	if (RCom == INVALID_HANDLE_VALUE) {
		printf("\nFatal Error 0x%x: Unable to open Rx\n", GetLastError());
	}
	else {
		//printf("\nCOM is now open\n");
	}
}
void createPortFileSend() {				// Transmit only
	// Call the CreateFile() function 
	TCom = CreateFile(
		COMPORTT,									// COM port number  --> If COM# is larger than 9 then use the following syntax--> "\\\\.\\COM10"
		GENERIC_READ | GENERIC_WRITE,				// Open for read and write
		NULL,										// No sharing allowed
		NULL,										// No security
		OPEN_EXISTING,								// Opens the existing com port
		FILE_ATTRIBUTE_NORMAL,						// Do not set any file attributes --> Use synchronous operation
		NULL										// No template
	);

	if (TCom == INVALID_HANDLE_VALUE) {
		printf("\nFatal Error 0x%x: Unable to open Tx\n", GetLastError());
	}
	else {
		printf("\nCOM is now open\n");
	}
}

// Defines and sets values for COM port.
static int SetComParms(HANDLE hCom, int type) {				// For either port
	DCB dcb;										// Windows device control block
	// Clear DCB to start out clean, then get current settings
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (!GetCommState(hCom, &dcb))
		return(0);

	// Set our own parameters from Globals
	dcb.BaudRate = nComRate;						// Baud (bit) rate
	dcb.ByteSize = (BYTE)nComBits;					// Number of bits(8)
	dcb.Parity = 0;									// No parity	
	dcb.StopBits = ONESTOPBIT;						// One stop bit
	if (!SetCommState(hCom, &dcb))
		return(0);

	// Set communication timeouts (SEE COMMTIMEOUTS structure in MSDN) - want a fairly long timeout
	// This sets the time for how long the com port will be open to reception or transmission.
	memset((void*)& timeout, 0, sizeof(timeout));
	if (type == 1) {		// Audio Message
		timeout.ReadIntervalTimeout = 10000;				// Maximum time allowed to elapse before arival of next byte in milliseconds. If the interval between the arrival of any two bytes exceeds this amount, the ReadFile operation is completed and buffered data is returned
		timeout.ReadTotalTimeoutMultiplier = .5;			// The multiplier used to calculate the total time-out period for read operations in milliseconds. For each read operation this value is multiplied by the requested number of bytes to be read
		timeout.ReadTotalTimeoutConstant = 5000;		// A constant added to the calculation of the total time-out period. This constant is added to the resulting product of the ReadTotalTimeoutMultiplier and the number of bytes (above).
		SetCommTimeouts(hCom, &timeout);

		//printf("Audio\n");
	}
	else {				// Text Message (Default)
		timeout.ReadIntervalTimeout = 500;				// Maximum time allowed to elapse before arival of next byte in milliseconds. If the interval between the arrival of any two bytes exceeds this amount, the ReadFile operation is completed and buffered data is returned
		timeout.ReadTotalTimeoutMultiplier = 1;			// The multiplier used to calculate the total time-out period for read operations in milliseconds. For each read operation this value is multiplied by the requested number of bytes to be read
		timeout.ReadTotalTimeoutConstant = 5000;		// A constant added to the calculation of the total time-out period. This constant is added to the resulting product of the ReadTotalTimeoutMultiplier and the number of bytes (above).
		SetCommTimeouts(hCom, &timeout);

		//printf("Text\n");
	}
	return(1);
}







/************************************SEND MESSAGE******************************************/
void send_message(int status) {												// Transmission only
	// Initialize the port
	char c;
	int count = 0;
	int flag = 0;
	char SenderID[4];
	char ReceiverID[4];
	int P[2];
	int Priority = 0;
	MessageCounter++;		// Gets initalized to zero every time the program starts
	char buf[TXTBUFF];		//Text payload is 140 byes, 16 bytes for the header
	link pointer = (link)malloc(sizeof(Node) + sizeof(cvt.convert) + 1);///////////////////////////////////////////////////////////////////////////////


/************************************PERSONAL MESSAGE******************************************/
	if (status == 1) {
		char msgOut[140];
		// Sender Program -- comment out these 3 lines if receiver
		printf("Enter a string up to 140 characters to send another computer.\n");

		while ((c = getchar()) != '\n' && (c != EOF)) {

			if (count == 139) {
				msgOut[139] = '\0';
				flag = 1;
				break;
			}

			msgOut[count] = c;
			count++;
		}
		if (flag == 0) {
			msgOut[count] = '\0';
		}
		
		printf("Enter your 3 digit sender ID.\n");
		fgets(SenderID, 4, stdin);

		while ((c = getchar()) != '\n' && (c != EOF)) {}	//Flushes input

		printf("Enter your one Digit Priority of a message from 1 (highest) to 9 (lowest).\n");
		scanf_s("%d", P, 2, stdin);
		Priority = P[0];
		while ((c = getchar()) != '\n' && (c != EOF)) {}	//Flushes input

		printf("Enter your 3 digit reciever ID. Type fff to transmit to all stations.\n");
		fgets(ReceiverID, 4, stdin);
	


		memcpy_s(buf, sizeof(buf), AppendMsg(msgOut, SenderID, ReceiverID, Priority), TXTBUFF);	// Append the header and payload to a char buffer to send;
		memcpy_s(cvt.convert, sizeof(cvt.convert) + 5, buf, sizeof(buf));


		if (flagDiag) {									// Diagnostic Mode, using 2 ports
			outputToPort(cvt.convert, sizeof(buf) + 1, 0);
			Sleep(1000);
			purgePort(TCom);
			CloseHandle(TCom);
		}
		else {											// Only one port, normal operation
			memcpy_s(pointer->Data.message, sizeof(pointer->Data.message), cvt.convert, sizeof(cvt.convert));
			pointer->Data.lDataLength = sizeof(buf) + 1;
			Send.AddToQueue(pointer);
		}					// Closes the handle pointing to the COM port

		//AddToQueue(msgOut);
		printf("\nPress any key to return to previous Menu\n");
		getchar();
		textMenu();

	}

	/************************************DEFAULT MESSAGE******************************************/
	else if (status == 2) {
		char msgOut[] = "Hi there person\n";

		printf("Enter your 3 digit sender ID.\n");
		fgets(SenderID, 4, stdin);
		printf("\n%s\n", SenderID);

		printf("Enter your 3 digit sender ID. Type fff to transmit to all stations.\n");
		fgets(ReceiverID, 4, stdin);
		printf("\n%s\n", ReceiverID);


		memcpy_s(buf, sizeof(buf), AppendMsg(msgOut, SenderID, ReceiverID,0), TXTBUFF);	// Append the header and payload to a char buffer to send;
		memcpy_s(cvt.convert, sizeof(cvt.convert), buf, sizeof(buf));
		//printf("\n%lx\n", cvt.FRAME.Head.lstartSequence);

		if (flagDiag) {									// Diagnostic Mode, using 2 ports
			outputToPort(cvt.convert, sizeof(buf)/* + 1*/, 0);
			Sleep(1000);
			purgePort(TCom);
			CloseHandle(TCom);
		}
		else {											// Only one port, normal operation
			memcpy_s(pointer->Data.message, sizeof(pointer->Data.message), cvt.convert, sizeof(cvt.convert));
			pointer->Data.lDataLength = sizeof(buf)/* + 1*/;
			Send.AddToQueue(pointer);
		}

		printf("\nPress any key to return to previous Menu\n");
		getchar();
		textMenu();
	}

	printf("\nPress any key to return to previous Menu\n");
	getchar();
	textMenu();
}

/************************************RANDOM MESSAGE******************************************/
void randMessage(char* msg) {												// Transmit only
	// Initialize the port
	int count = 0;
	int flag = 0;
	char buf[sizeof(MssgTxt)];		//Text payload is 140 byes, 16 bytes for the header
	char ReceiverID[4] = { 'f', 'f', 'f', '\0' };
	char SenderID[4] = { 'f', 'f', 'f', '\0' };

	memcpy_s(buf, sizeof(buf), AppendMsg(msg, SenderID, ReceiverID, 0), TXTBUFF);	// Append the header and payload to a char buffer to send;
	memcpy_s(cvt.convert, sizeof(cvt.convert), buf, sizeof(buf));

	if (flagDiag) {									// Diagnostic Mode, using 2 ports
		outputToPort(cvt.convert, sizeof(buf)/* + 1*/, 0);
		Sleep(500);
		purgePort(TCom);
		CloseHandle(TCom);
	}
	else {											// Only one port, normal operation
		memcpy_s(pointer->Data.message, sizeof(pointer->Data.message), cvt.convert, sizeof(cvt.convert));
		pointer->Data.lDataLength = sizeof(buf)/* + 1*/;
		Send.AddToQueue(pointer);
	}

	textMenu();
}

/************************************RECIEVE MESSAGE******************************************/
void recieveMessage() {									// Receive only

	pointer = (link)malloc(sizeof(Node) + sizeof(cvt.convert));
	char msgIn[TXTBUFF + 1];									// Initialize the port
	char cESC = '%';
	int rc;
	int flag = 0;
	int index = 0;
	char RecID[4] = { '0', '0', '0', '\0' };			// Change this line depending on what you want to make the Receive ID of this station is.
	char TRANSMIT[4] = { 'f', 'f', 'f', '\0' };
	// The input message is saved to msgIn

	int MsgBytes = inputFromPort(msgIn, (TXTBUFF + 1), 0);	// Receive string from port
	char *Buff = (char*)malloc(cvt.FRAME.Head.luncompressedLen * sizeof(char));
	if (MsgBytes > 0) {								// if there is actually a message
		msgIn[MsgBytes - 1] = '\0';

		memcpy_s(cvt.convert, sizeof(cvt.convert) + 2, msgIn, MsgBytes);	//This copys the whole buffer (header and message) into memory at cvt.convert the union
		RLEdeCompress((unsigned char*)cvt.FRAME.Text.message, cvt.FRAME.Head.lcompressedLen, (unsigned char*)Buff, cvt.FRAME.Head.lcompressedLen, cESC);

		if ((strcmp(RecID, cvt.FRAME.Head.rid) == 0) || (strcmp(TRANSMIT, cvt.FRAME.Head.rid) == 0)) {		
		/**** If the incoming RID does not fall under the reciver ID of this station or the transmit case (fff) the message is ignored. *****/

			/*****STORE VALUES OF THE UNION INTO THE QUEUE*****/
			for (int i = 0; i < phoneBookCount; i++) {

				if (strcmp(Array[i].SID, cvt.FRAME.Head.sid) == 0) {
					flag = 1;
					index = i;
				}
			}

			if (flag == 0) {
				AddToPhonebook(cvt.FRAME.Head.sid);
			}
			else if (flag == 1) {
				AnotherMessage(index);
			}

			strcpy_s(pointer->Data.message, 140, Buff);
			pointer->Data.lDataLength = cvt.FRAME.Head.luncompressedLen;
			pointer->Data.rid = (char*)malloc(4 * sizeof(char));
			pointer->Data.sid = (char*)malloc(4 * sizeof(char));
			strcpy_s(pointer->Data.sid, 4, cvt.FRAME.Head.sid);
			strcpy_s(pointer->Data.rid, 4, cvt.FRAME.Head.rid);
			pointer->Data.sid = cvt.FRAME.Head.sid;
			pointer->Data.flag = cvt.FRAME.Head.flag;
			pointer->Data.priority = cvt.FRAME.Head.priority;
			Recieve.AddToQueue(pointer);


			//MinHeap heaper = MinHeap( 140 + sizeof(int));
			cvt.FRAME.Text.message[cvt.FRAME.Head.luncompressedLen-1] = '\0';
			heaper.insertKey(cvt.FRAME.Head.priority, cvt.FRAME.Text.message);



			msgFlag++;
		}
		else {
			printf("\nMessage was ignored.\nMake sure you know whe the ID of the station you are trying to send to.\n");
		}
	}
	//free(pointer);
	purgePort(RCom);									// Purge the port
	CloseHandle(RCom);						// Closes the handle pointing to the COM port
}

void SendAudio() {					//Transmit only

	char* buf = (char*)calloc(sizeof(MssgAud), sizeof(char));
	printf("\n%d\n", sizeof(MssgAud));
	printf("\n%d\n", sizeof(buf));
	if (SettingFlag == 1) {
		lBigBufSize = SAMPLES_SEC * recordTime;
		iBigBuf[SAMPLES_SEC * recordTime];
	}
	if ((buf) == NULL) {
		printf("Can't allocate enough heap space, ending the program\n");
		exit(1);
	}

	// Record
	InitializeRecording();
	RecordBuffer(iBigBuf, lBigBufSize);
	CloseRecording();

	//InitializePlayback();
	//PlayBuffer(iBigBuf, lBigBufSize);
	//ClosePlayback();

	memcpy_s(buf, sizeof(MssgAud), AppendAudio(iBigBuf), 96000);	// Append the header and payload to a char buffer to send;
	memcpy_s(cvtA.convert, sizeof(cvtA.convert), buf, sizeof(MssgAud));
	
	//InitializePlayback();
	//PlayBuffer(cvtA.FRAMEAUD.Aud.audioBuf, lBigBufSize);
	//ClosePlayback();

	printf("\nPress any key to send the recording\n");

	getchar();

	if (flagDiag) {
		outputToPort(cvtA.convert, sizeof(MssgAud), 1);
		Sleep(1000);
		purgePort(TCom);
		CloseHandle(TCom);
	}
	else {
		// save to a queue node
		// save the length in the queue node
		// enqueue
	}

	free(buf);

	printf("\nPress any key to return to previous Menu\n");
	getchar();
	soundMenu();
}
int counter = 0;
void RecieveAudio() {						// Recieve only fn
	
	link ptr = (link)malloc(sizeof(Node) + sizeof(cvtA.convert));
	short* msgIn = (short*)calloc(lBigBufSize + 10000, sizeof(short));
	if (msgIn == NULL) {
		printf("Cannot allocate enough heap space\n");
		exit;
	}

	int MsgBytes = inputFromPort(msgIn, (lBigBufSize), 1);	// Receive string from port
	//INCLUDE COMPRESSION BUFFER ALLOCATION HERE
	//printf("Count is: %d\n", MsgBytes);
	if (MsgBytes > 0) {
		msgIn[MsgBytes - 1] = '\0';											// Needed?
		memcpy_s(cvtA.convert, sizeof(cvtA.convert), msgIn, MsgBytes);
		free(msgIn);

		ptr->Data.Buffer = (short*)calloc((SAMPLES_SEC * RECORD_TIME), sizeof(short));
		if (ptr->Data.Buffer == NULL) {
			printf("Cannot allocate enough heap space to recieve\n");
			exit;
		}
		if (MsgBytes > 48000) {
			counter = 0;
		}


		ptr->Data.lDataLength = lBigBufSize;
		//ptr->Data.Buffer = cvtA.FRAMEAUD.Aud.audioBuf;
		//ptr->Data.size = cvtA.FRAMEAUD.Head.luncompressedLen;
		memcpy_s(ptr->Data.Buffer, 48000 * 2, cvtA.FRAMEAUD.Aud.audioBuf, sizeof(cvtA.FRAMEAUD.Aud.audioBuf));
		//		strcpy_s(ptr->Data.Buffer, sizeof(short)*48017, (short*)cvtA.FRAMEAUD.Aud.audioBuf);
				//pointer->Data.lDataLength = cvt.FRAME.Head.luncompressedLen;
				//pointer->Data.ReceiveAddr = cvt.FRAME.Head.ReceiveAddr;
				//pointer->Data.flag = cvt.FRAME.Head.flag;
				//Recieve.AddToQueue(pointer);
		if (counter == 0) {
			counter = 1;
			InitializePlayback();
			PlayBuffer(ptr->Data.Buffer, lBigBufSize);
			ClosePlayback();
			aRecieve.AddToQueue(ptr);
		}
		
		
		AmsgFlag++;

	}
	free(ptr);
	purgePort(RCom);
	CloseHandle(RCom);
	//textMenu();
}

void AudioSettings() {

	printf("Enter length of recording (s).\n");
	scanf_s("%d", &recordTime, 2);


	SettingFlag = 1;
	printf("\nYour Audio settings have been updated!\n");
	settings();
}



// Send everything waiting to be sent
void TransmitQ() {

	link q;
	while (!Send.IsQueueEmpty()) {								// if there is no head's avaiable
		q = Send.DeQueue();										// deque sends the current head to main and then moves head to next spot

		//initPortT(0);

		//memcpy_s(cvt.convert, sizeof(cvt.convert), q->Data.message, sizeof(q->Data.message));

		printf("\n\n %s \n\n %d \n\n", q->Data.message, q->Data.lDataLength);

		outputToPort(q->Data.message, q->Data.lDataLength, 0);	// Send string to port - include space for '\0' termination

		//outputToPort("For why\n", strlen("For why\n"),0);

		printf("Just sent: %s\n", q->Data.message);
		free(q);												// free the head
		Sleep(1000);											// Allow time for signal propagation on cable 
		purgePort(TCom);										// Purge the port
		CloseHandle(TCom);										// Closes the handle pointing to the COM port
	}

}