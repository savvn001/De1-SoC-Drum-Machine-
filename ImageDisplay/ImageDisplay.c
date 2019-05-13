/*
 * ImageDisplay.c

 *
 *  Created on: 9 May 2019
 *      Author: Nick
 */

#include "ImageDisplay.h"

void initDisplay() {

	Graphics_initialise(0xFF200060, 0xFF200080);
	HPS_ResetWatchdog();

}

void drawUI(int channel, int16_t *sample_buffer, int length ) {

	plot(sample_buffer, length);

	switch (channel) {

	case 0:

		LT24_copyFrameBuffer(Kick, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 7:
		LT24_copyFrameBuffer(Clap, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 1:
		LT24_copyFrameBuffer(Snare, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 2:
		LT24_copyFrameBuffer(Hatc, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 3:
		LT24_copyFrameBuffer(Hato, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 5:
		LT24_copyFrameBuffer(Ride, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 6:

		LT24_copyFrameBuffer(Crash, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	case 4:

		LT24_copyFrameBuffer(Tom, 0, 0, 240, 100);
		HPS_ResetWatchdog();
		break;
	}

}



void plot(int16_t *sample_buffer, int length){
	unsigned short arr[28800] = {0};
	int stepsPerPlot = length/200;
	int scale = 0.1;
	int val = 0;
	for (int i = 0; i < 200; ++i) {

		//printf(" %d\n", sample_buffer[i*stepsPerPlot]);
		val = (int)(sample_buffer[i*stepsPerPlot])/10;
		arr[14400+i+(val)] = 0x659A;


		//LT24_drawPixel(LT24_RED,i+20, sample_buffer[i*stepsPerPlot]*scale+160);

	}
	LT24_copyFrameBuffer(arr, 0, 80, 240, 100);


}
