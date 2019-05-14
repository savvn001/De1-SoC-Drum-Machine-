/*
 * ImageDisplay.c
 *
 * 	For plotting a waveform of the current sample onto the LCD display
 *
 *  Created on: May 2019
 *      Author: Calumn Bostead, Samrudh Sharma
 */

#include "ImageDisplay.h"

void initDisplay() {

	Graphics_initialise(0xFF200060, 0xFF200080);
	HPS_ResetWatchdog();

}

void drawUI(int channel, int16_t *sample_buffer, int length ) {

	// Copy background image to screen
	LT24_copyFrameBuffer(border, 0, 80, 240, 240);

	// Channel is incremented using an interrupt
	// Channel values are mapped to sounds
	switch (channel) {
	// Kick - 0
	case 0:
		LT24_copyFrameBuffer(Kick, 0, 0, 240, 100); //Title
		plot(sample_buffer, length); //Plot graph
		HPS_ResetWatchdog();
		break;
	// Clap - 7
	case 7:
		LT24_copyFrameBuffer(Clap, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Snare - 1
	case 1:
		LT24_copyFrameBuffer(Snare, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Hat closed - 2
	case 2:
		LT24_copyFrameBuffer(Hatc, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Hat open - 3
	case 3:
		LT24_copyFrameBuffer(Hato, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Ride - 5
	case 5:
		LT24_copyFrameBuffer(Ride, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Crash - 6
	case 6:
		LT24_copyFrameBuffer(Crash, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	// Tom - 4
	case 4:
		LT24_copyFrameBuffer(Tom, 0, 0, 240, 100);
		plot(sample_buffer, length);
		HPS_ResetWatchdog();
		break;
	}

}


// Function for plotting WAV waveform to screen
// sample_buffer - WAV file buffer
// length - Number of elements in WAV file buffer
void plot(int16_t *sample_buffer, int length){
	int val = 0;
	int max = 0;

	// Calculate required steps for incrementing through buffer
	int stepsPerPlot = length/(200*2);

	// For loop for finding the maximum value in the array
	for(int i=50;i<200;i++){

		// If this value is greater than current max found then set this as new max
		if((int)(sample_buffer[i*stepsPerPlot])/200 > max){
			max = (int)(sample_buffer[i*stepsPerPlot])/200;
		}
	}
	// For loop to plot every element to screen
	for(int i=50;i<199;i++){

		// Get sampling point from buffer
		val = (int)(sample_buffer[i*stepsPerPlot])/200; // 200 is scaling value

		// Invert value
		// This is necessary because ymax is at the bottom of LCD
		val = ((-val)*60)/max;

		// Plot points
		LT24_drawPixel(LT24_CYAN,i+20-25,200+val);
		LT24_drawPixel(LT24_CYAN,i+20-25,200+val+1);
		LT24_drawPixel(LT24_CYAN,i+20-25+1,200+val);
		LT24_drawPixel(LT24_CYAN,i+20-25+1,200+val+1);

	}
}
