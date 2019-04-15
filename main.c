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
#include "HPS_IRQ/HPS_IRQ.h"
#include "wav/wav.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define HPS_GPIO_PORT  0
#define HPS_GPIO_DDR   1
#define HPS_GPIO_PIN  20
#define NO_OF_CHANNELS 2

#define BUFFERSIZE 45224

void exitOnFail(signed int status, signed int successStatus) {
	if (status != successStatus) {
		exit((int) status); //Add breakpoint here to catch failure
	}
}

int BPM = 128;
int sequence_step = 0;
int16_t kick_sequence[8] = { 1, 0, 0, 0, 1, 0, 0, 0 };
int16_t hat_sequence[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };

int sample_val = 0;

int16_t kick_buffer[45224];

/********************* Function Prototypes ******************************/
void step16(HPSIRQSource interruptID, bool isInit, void* initParams);
void audioISR(HPSIRQSource interruptID, bool isInit, void* initParams);

int main(void) {

	///////////////////////////////////////////////////////////////////
	//////////////////// Timer Interrupt Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////

	/****** Timer 0 will be called every 1/6th of a beat, so used for the step sequencer *******/
	volatile unsigned int * HPS_timer0_ptr = (unsigned int *) 0xFFC08000;
	volatile unsigned int * HPS_gpio_ptr = (unsigned int *) 0xFF709000;

	// Set GPIO LED to output, and low
	unsigned int gpio_rmw;
	//Set HPS LED low
	gpio_rmw = HPS_gpio_ptr[HPS_GPIO_PORT];
	gpio_rmw = gpio_rmw & ~(1 << 24);
	HPS_gpio_ptr[HPS_GPIO_PORT] = gpio_rmw;
	//Set HPS LED to output
	gpio_rmw = HPS_gpio_ptr[HPS_GPIO_DDR];
	gpio_rmw = gpio_rmw | (1 << 24);
	HPS_gpio_ptr[HPS_GPIO_DDR] = gpio_rmw;
	HPS_ResetWatchdog();

	//Initialise IRQs
	HPS_IRQ_initialise(NULL);
	HPS_ResetWatchdog();

	// Timer base address
	HPS_timer0_ptr[2] = 0; // write to control register to stop timer

	/* Set the timer period, this should mean step function gets called every 1/4 of a beat
	 * meaning each step is 1/6th of a bar (4 beats in a bar)
	 */
	HPS_timer0_ptr[0] = (100000000 * (BPM / 60)) / 16;
	// Write to control register to start timer, with interrupts
	HPS_timer0_ptr[2] = 0x03; // mode = 1, enable = 1
	// Register interrupt handler for timer
	HPS_IRQ_registerHandler(IRQ_TIMER_L4SP_0, step16);
	HPS_ResetWatchdog();

	/*********** Timer 1 will be called every time period of the sampling rate Fs, so 1/Fs ******/

	volatile unsigned int * HPS_timer1_ptr = (unsigned int *) 0xFFC09000;

	// Timer base address
	HPS_timer1_ptr[2] = 0; // write to control register to stop timer

	/* Set timer period to 1/48kHz (or as close as it can get to it)
	 */
	HPS_timer1_ptr[0] = 2083.33333333; //2083.33333333
	// Write to control register to start timer, with interrupts
	HPS_timer1_ptr[2] = 0x03; // mode = 1, enable = 1

	/* Call function before registering to handler with IRQ driver
	 * so can set some of the values inside before interrupts enabled
	 */
//	audioISR(IRQ_TIMER_L4SP_1, true, 0);
	// Register interrupt handler for timer
	//HPS_IRQ_registerHandler(IRQ_TIMER_L4SP_1, audioISR);
	HPS_ResetWatchdog();

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Playback Setup /////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	struct channel {

		int16_t play_sequence[8];
		int16_t *sample_buffer;
		bool isPlaying;

	} kick, snare, hatc, hato, tom, crash, ride, clap;

	/**************** Kick *******************/

	kick.play_sequence[0] = 1;
	kick.play_sequence[1] = 0;
	kick.play_sequence[2] = 0;
	kick.play_sequence[3] = 0;
	kick.play_sequence[4] = 1;
	kick.play_sequence[5] = 0;
	kick.play_sequence[6] = 0;
	kick.play_sequence[7] = 0;
	kick.isPlaying = false;

	/**************** Clap *******************/



	/**************** Closed Hat *******************/


	signed int audioOutputL = 0;
	signed int audioOutputR = 0;

	///////////////////////////////////////////////////////////////////
	//////////////////// SD Card Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////

	initFatFS();

	//Calculate size of buffer from header value which returns no. of bytes in data
	const int kickBufferSize = readWavFileHeader("kick.wav")/2;
	//Create array to store audio samples
	kick.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * kickBufferSize);
	//Init all elements to 0 to avoid any potential issues
	for (int i = 0; i < kickBufferSize; i++) {
		kick.sample_buffer[i] = 0;
	}

	//Fill with data
	FillBufferFromSDcard(kick.sample_buffer);

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Audio Codec Setup //////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	volatile unsigned char* fifospace_ptr;
	volatile unsigned int* audio_left_ptr;
	volatile unsigned int* audio_right_ptr;

	//Initialise the Audio Codec.
	exitOnFail(WM8731_initialise(0xFF203040),  //Initialise Audio Codec
	WM8731_SUCCESS);                //Exit if not successful

	//Clear both FIFOs
	WM8731_clearFIFO(true, true);

	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

	//////////////////////////////////////////////////////////////////////////////////////////////

	while (1) {

		//Check for space in outgoing FIFOs (each 128 samples wide)
		if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {
			//While there's space, fill it up with samples

			//for(k = 0; k < (no. of channels); k++)

			//Get output from kick
			if (sample_val < kickBufferSize) {

				audioOutputL += kick.sample_buffer[sample_val] * 1000;
				audioOutputR += kick.sample_buffer[sample_val + 1] * 1000;
				sample_val += 2;
			}

			//Get output from hi hat1

			// Output summed master output to FIFO buffer
			*audio_left_ptr = audioOutputL;
			*audio_right_ptr = audioOutputR;

			//Reset back to  0
			audioOutputL = 0;
			audioOutputR = 0;

		}

		//Reset the watchdog.
		HPS_ResetWatchdog();

	}
}

void step16(HPSIRQSource interruptID, bool isInit, void* initParams) {
	if (!isInit) {

		volatile unsigned int * HPS_timer0_ptr = (unsigned int *) 0xFFC08000;
		volatile unsigned int * HPS_gpio_ptr = (unsigned int *) 0xFF709000;
		volatile unsigned int * LED_step_ptr = (unsigned int *) 0xFF200000;

		//Toggle board green LED every step
		unsigned int gpio_rmw;
		gpio_rmw = HPS_gpio_ptr[HPS_GPIO_PORT];
		gpio_rmw = gpio_rmw ^ (1 << 24);
		HPS_gpio_ptr[HPS_GPIO_PORT] = gpio_rmw;

		//Clear the Timer Interrupt Flag
		//By reading timer end of interrupt register
		gpio_rmw = HPS_timer0_ptr[3];

		*LED_step_ptr = 0x200 >> sequence_step;

		//Check if sound set to trigger on current step
		if (kick_sequence[sequence_step]) {
			sample_val = 0; //Set kick drum sample buffer to 0 - plays from start
		}

		//Increment LEDs to show step then start from beginning again if 7
		if (sequence_step < 7) {
			sequence_step++;
		} else {
			sequence_step = 0;
		}

	}

	HPS_ResetWatchdog();

}

void audioISR(HPSIRQSource interruptID, bool isInit, void* initParams) {

	if (!isInit) {

		//Clear the Timer Interrupt Flag
		//By reading timer end of interrupt register
		//unsigned int clear = HPS_timer1_ptr[3];
	}

	HPS_ResetWatchdog();

}


