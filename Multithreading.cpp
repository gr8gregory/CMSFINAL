/*
 * Multithreading.cpp
 *
 */

#include <stdio.h>
#include <thread>		// std::thread
#include <Windows.h>
#include "Multithreading.h"
#include "RS232Comm.h"

void Multithreading() {
	std::thread RX(readMsgs);
	RX.detach();
}


void readMsgs() {
	if (!flagDiag) {			// Standard mode
		while (running) {
			if (flagMode) {
				RecieveAudio();
				//TransmitAQ();
			}
			else {
				recieveMessage();
				TransmitQ();
			}
		}
	}
	else {						// Diagnostic Mode (2 ports)
		while (running) {
			if (flagMode) {
				RecieveAudio();
			}
			else {
				recieveMessage();
			}
		}
	}
	return;
}
