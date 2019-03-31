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

void exitOnFail(signed int status, signed int successStatus) {
	if (status != successStatus) {
		exit((int) status); //Add breakpoint here to catch failure
	}
}

//Define some useful constants
#define F_SAMPLE 44100.0        //Sampling rate of WM8731 Codec
#define PI2      6.28318530718  //2 x Pi      (Apple or Peach?)

int main(void) {

	//Buffer to store audio data from sample, interleaved with L R samples
	int32_t audioBuffer[2048];

	volatile unsigned char* fifospace_ptr;
	volatile unsigned int* audio_left_ptr;
	volatile unsigned int* audio_right_ptr;

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
	const unsigned int blinkPeriod = 400000000;
	/* Initialisation */
	// Set initial value of LEDs
	*LEDR_ptr = 0x1;
	// Configure the ARM Private Timer
	// Set the "Load" value of the timer to max value:
	*private_timer_load = 0xFFFFFFFF;
	// Set the "Prescaler" value to 0, Enable the timer (E = 1), Set Automatic reload
	// on overflow (A = 1), and disable ISR (I = 0)
	*private_timer_control = (0 << 8) | (0 << 2) | (1 << 1) | (1 << 0);
	/* Main Run Loop */

	int32_t audio_sample = 0;

	initFatFS();
	readWavFileHeader("kick.wav");
	FillBufferFromSDcard(audioBuffer); //Store interleaved buffer

	//Initialise the Audio Codec.
	exitOnFail(WM8731_initialise(0xFF203040),  //Initialise Audio Codec
	WM8731_SUCCESS);                //Exit if not successful
	//Clear both FIFOs
	WM8731_clearFIFO(true, true);
	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

//ResetWDT();

	while (1) {

		// Read the current time
		unsigned int currentTimerValue = *private_timer_value;
		// Check if it is time to blink
		if ((lastBlinkTimerValue - currentTimerValue) >= blinkPeriod) {
			// When the difference between the last time and current time is greater
			// than the required blink period. We use subtraction to prevent glitches
			// when the timer value overflows ñ e.g. (0x10 ñ 0xFFFFFFFF)%32 = 0x11.
			// If the time elapsed is enough, perform our actions.
			*LEDR_ptr = ~(*LEDR_ptr); // Invert the LEDs

			 for (int i = 0; i < 2048; i++) {

				 if ((i % 2) == 0) {

				 audio_sample = audioBuffer[i];

					 //Always check the FIFO Space before writing or reading left/right channel pointers
					 if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {

					 // Output tone to left and right channels.
					 *audio_left_ptr = audio_sample;
					 *audio_right_ptr = audio_sample;
					 }

				 }
			 }


			// To avoid accumulation errors, we make sure to mark the last time
			// the task was run as when we expected to run it. Counter is going
			// down, so subtract the interval from the last time.
			lastBlinkTimerValue = lastBlinkTimerValue - blinkPeriod;
		}
		// --- You can have many other events here by giving each an if statement
		// --- and its own "last#TimerValue" and "#Period" variables, then using the
		// --- same structure as above.
		// Next, make sure we clear the private timer interrupt flag if it is set
		if (*private_timer_interrupt & 0x1) {
			// If the timer interrupt flag is set, clear the flag
			*private_timer_interrupt = 0x1;
		}
		// Finally, reset the watchdog timer. We can use the ResetWDT() macro.
		ResetWDT();	// This basically writes 0x76 to the watchdog reset register.
	}

}
