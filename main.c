/*
 * 	main.c


 *
 * "DE808" Classic TR808/909 style drum machine implemented on
 *  a De1-SoC.
 *
 *
 *	Authors: Nicholas Savva, Calum Boustead,  Samrudh Sharma
 */
#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "Playback_Engine/Playback_Engine.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {

	//Init Peripherals
	setup_playback();
	setup_codec();
	setup_graphics();
	//fillFIFO();
	setup_IRQ();

	update7seg(128);

	//Reset the watchdog.
	HPS_ResetWatchdog();

	ResetWDT();
	//////////////////////////////////////////////////////////////////////////////////////////////

	while (1) {

		audioPlaybackPolling();
		HPS_ResetWatchdog();

		//These functions are constantly polled as the push button ISR sets a flag if these need to be called
		//(Keeps button ISR as short as possible)
		latchSequence();
		incrementCH();
		updateBPM();

	}

}

