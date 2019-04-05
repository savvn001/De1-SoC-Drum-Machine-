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
#include "DE1SoC_WM8731/DE1SoC_WM8731.h"
#include "wav/wav.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void exitOnFail(signed int status, signed int successStatus) {
	if (status != successStatus) {
		exit((int) status); //Add breakpoint here to catch failure
	}
}

//Define some useful constants
#define F_SAMPLE 44100.0        //Sampling rate of WM8731 Codec
#define PI2      6.28318530718  //2 x Pi      (Apple or Peach?)

int main(void) {

	///////////////////////////////////////////////////////////////////
	//////////////////// Timer Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////

	// Red LEDs base address
	volatile unsigned int *LEDR_ptr = (unsigned int *) 0xFF200000;
	// ARM A9 Private Timer Load
	volatile unsigned int *private_timer_load = (unsigned int *) 0xFFFEC600;
	// ARM A9 Private Timer Value
	volatile unsigned int *private_timer_value = (unsigned int *) 0xFFFEC604;
	// ARM A9 Private Timer Control
	volatile unsigned int *private_timer_control = (unsigned int *) 0xFFFEC608;
	// ARM A9 Private Timer Interrupt
	volatile unsigned int *private_timer_interrupt = (unsigned int *) 0xFFFEC60C;
	/* Local Variables */
	unsigned int lastBlinkTimerValue = *private_timer_value;
	const unsigned int blinkPeriod = 600000000;
	/* Initialisation */
	// Set initial value of LEDs
	*LEDR_ptr = 0x1;
	// Configure the ARM Private Timer
	// Set the "Load" value of the timer to max value:
	*private_timer_load = 0xFFFFFFFF;
	// Set the "Prescaler" value to 0, Enable the timer (E = 1), Set Automatic reload
	// on overflow (A = 1), and disable ISR (I = 0)
	*private_timer_control = (0 << 8) | (0 << 2) | (1 << 1) | (1 << 0);

	///////////////////////////////////////////////////////////////////
	//////////////////// SD Card Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////

	initFatFS();

	//Calculate size of buffer from header value which returns no. of bytes in data
	const int header = readWavFileHeader("kick2.wav");
	const int kick_buffer_size = 45224;

	//Create array to store samples
	//int16_t *kick_buffer = malloc(kick_buffer_size * sizeof(*kick_buffer));
	int16_t kick_buffer[kick_buffer_size];

	for (int i = 0; i < kick_buffer_size; i++) {
		kick_buffer[i] = 0;
	}

	//Fill with data
	FillBufferFromSDcard(kick_buffer);

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Audio Codec Setup //////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	//Phase Accumulator
	double phase = 0.0;  // Phase accumulator
	double inc = 0.0;  // Phase increment
	double ampl = 0.0;  // Tone amplitude (i.e. volume)
	signed int audio_sample = 0;

	volatile unsigned char* fifospace_ptr;
	volatile unsigned int* audio_left_ptr;
	volatile unsigned int* audio_right_ptr;

	inc = 440.0 * PI2 / F_SAMPLE; // Calculate the phase increment based on desired frequency - e.g. 440Hz
	ampl = 8388608.0; // Pick desired amplitude (e.g. 2^23). WARNING: If too high = deafening!
	phase = 0.0;

	//Initialise the Audio Codec.
	exitOnFail(WM8731_initialise(0xFF203040),  //Initialise Audio Codec
	WM8731_SUCCESS);                //Exit if not successful

	//Clear both FIFOs
	WM8731_clearFIFO(true, true);

	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

	int i = 0;

	while (1) {

		//Check for space in outgoing FIFO (128 samples wide)
		if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {

			//While there's space, fill it up with samples

			// Output tone to left and right channels.
			*audio_left_ptr =  (signed int) kick_buffer[i] * 10000;
			*audio_right_ptr = (signed int) kick_buffer[i + 1] * 10000;

			i += 2;
		}

		if(i == kick_buffer_size){
			i = 0;
		}


		//Reset the watchdog.
		HPS_ResetWatchdog();

	}
}
