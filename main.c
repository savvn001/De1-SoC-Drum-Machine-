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
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "HPS_usleep/HPS_usleep.h"
#include "wav/wav.h"

int main (void)
{

	//Buffer to store audio data from sample
	int16_t ramBufferDacData0Stereo[2 * 1024];

	FillBufferFromSDcard(ramBufferDacData0Stereo); //Left CH

	initFatFS();
	readWavFileHeader("kick.wav");

	ResetWDT();


	while(1){
		HPS_ResetWatchdog();
	}
}





