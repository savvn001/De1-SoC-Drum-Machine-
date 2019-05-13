/*
 * main.c
 *
 * This program is a simple piece of code to test the Watchdog Timer.
 *
 * The watchdog will be reset any time a button is pressed.
 *
 * The value of the watchdog timer is displayed on the red LEDs.
 *
 *  Created on: 13 Jan 2018
 *      Author: Doug from Up.
 *       Notes: Squirrel!
 */

#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "Playback_Engine/Playback_Engine.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {

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

		//for(k = 0; k < (no. of channels); k++)
		audioPlaybackPolling();
		HPS_ResetWatchdog();

		//These functions are constantly polled as the push button ISR sets a flag if these need to be called
		//(Keeps button ISR as short as possible)
		latchSequence();
		incrementCH();
		updateBPM();

	}

}

