/*
 * Playback_Engine.c
 *
 *  Created on: 11 May 2019
 *      Author: Nick
 */
#include "Playback_Engine.h"

#define NO_OF_CHANNELS 2
#define HPS_GPIO_PORT  0
#define HPS_GPIO_DDR   1
#define HPS_GPIO_PIN  20
#define NO_OF_CHANNELS 2
#define SEQUENCE_STEPS 8

//Get switches pointer
#define SWITCH_PTR 0xFF200040

//Define some useful constants
#define F_SAMPLE 48000.0        //Sampling rate of WM8731 Codec (Do not change)
#define PI2      6.28318530718  //2 x Pi      (Apple or Peach?)

/***************** Global variables for this driver *********************/
int BPM = 128;
int sequence_step = 0;

//Codec memory mapped peripheral pointers
volatile unsigned char* fifospace_ptr;
volatile unsigned int* audio_left_ptr;
volatile unsigned int* audio_right_ptr;

//For audio playback
signed int audioOutputL = 0;
signed int audioOutputR = 0;

//Phase Accumulator
double phase = 0.0;  // Phase accumulator
double inc = 0.0;  // Phase increment
double ampl = 0.0;  // Tone amplitude (i.e. volume)
signed int audio_sample = 0;

struct channel {

	bool play_sequence[SEQUENCE_STEPS];	//Playback sequence
	unsigned int bufferSize; 		//Number of elements in sample data
	int16_t *sample_buffer;		//Pointer to buffer storing sample data
	bool isPlaying;	//Used later on to determine whether instrument currently playing or not
	unsigned int volume;
	unsigned int sample_pt;	//Which element in the sample's buffer is currently being output

} kick, snare, hatc, hato, tom, crash, ride, clap;

//List of channel names
char channel_names[8][10] = {

"kick", "snare", "hatc", "hato", "tom", "crash", "ride", "clap" };

//This represents what channel we're currently sequencing/modifying so we know what channel to latch the sequence to
//CH0 = Kick, CH1 = snare, CH2 = hatc etc... as ordered in struct declaration
int current_channel = 0;

void setup_playback() {

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Playback Setup /////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	/**************** Kick *******************/

//	kick.play_sequence[0] = 1;
//	kick.play_sequence[1] = 0;
//	kick.play_sequence[2] = 0;
//	kick.play_sequence[3] = 0;
//	kick.play_sequence[4] = 1;
//	kick.play_sequence[5] = 0;
//	kick.play_sequence[6] = 0;
//	kick.play_sequence[7] = 0;
//	kick.isPlaying = false;
	/**************** Clap *******************/

	/**************** Closed Hat *******************/

	///////////////////////////////////////////////////////////////////
	//////////////////// SD Card Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////
	initFatFS();

	/*** Get audio from .wav files for each channel and store in buffer ***/

	///////////////////Kick/////////////////////////////////////////////////////////////////////
	//Calculate size of buffer from header value which returns no. of bytes in data
	kick.bufferSize = readWavFileHeader("kick.wav") / 2;
	//Create array to store audio samples
	kick.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * kick.bufferSize);
	//Fill with data
	FillBufferFromSDcard(kick.sample_buffer);
	kick.sample_pt = kick.bufferSize + 2; //Set it to outside range intially so it doesn't trigger
	kick.volume = 6000;

	///////////////////clap/////////////////////////////////////////////////////////////////////
	clap.bufferSize = readWavFileHeader("clap.wav") / 2;
	clap.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * clap.bufferSize);
	FillBufferFromSDcard(clap.sample_buffer);
	clap.sample_pt = 0;
	clap.volume = 6000;
	///////////////////snare/////////////////////////////////////////////////////////////////////
	snare.bufferSize = readWavFileHeader("snare.wav") / 2;
	snare.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * snare.bufferSize);
	FillBufferFromSDcard(snare.sample_buffer);
	snare.sample_pt = 0;
	snare.volume = 6000;
	///////////////////ride/////////////////////////////////////////////////////////////////////
	ride.bufferSize = readWavFileHeader("ride.wav") / 2;
	ride.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * ride.bufferSize);
	FillBufferFromSDcard(ride.sample_buffer);
	ride.sample_pt = 0;
	ride.volume = 6000;
	///////////////////tom/////////////////////////////////////////////////////////////////////
	tom.bufferSize = readWavFileHeader("tom.wav") / 2;
	tom.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * tom.bufferSize);
	FillBufferFromSDcard(tom.sample_buffer);
	tom.sample_pt = 0;
	tom.volume = 6000;
	///////////////////hatc/////////////////////////////////////////////////////////////////////
	hatc.bufferSize = readWavFileHeader("hatc.wav") / 2;
	hatc.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * hatc.bufferSize);
	FillBufferFromSDcard(hatc.sample_buffer);
	hatc.sample_pt = 0;
	hatc.volume = 6000;
	///////////////////hato/////////////////////////////////////////////////////////////////////
	hato.bufferSize = readWavFileHeader("hato.wav") / 2;
	hato.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * hato.bufferSize);
	FillBufferFromSDcard(hato.sample_buffer);
	hato.sample_pt = 0;
	hato.volume = 6000;
	///////////////////crash/////////////////////////////////////////////////////////////////////
	crash.bufferSize = readWavFileHeader("crash.wav") / 2;
	crash.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * crash.bufferSize);
	FillBufferFromSDcard(crash.sample_buffer);
	crash.sample_pt = 0;
	crash.volume = 6000;
}

void setup_codec() {

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Audio Codec Setup //////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	//Initialise the Audio Codec.
	exitOnFail(WM8731_initialise(0xFF203040),  //Initialise Audio Codec
	WM8731_SUCCESS);                //Exit if not successful

	//Clear both FIFOs
	WM8731_clearFIFO(true, true);

	//Enable interrupts on codec
	//WM8731_enableIRQ();

	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

	//Initialise Phase Accumulator
	inc = 440.0 * PI2 / F_SAMPLE; // Calculate the phase increment based on desired frequency - e.g. 440Hz
	ampl = 8388608.0; // Pick desired amplitude (e.g. 2^23). WARNING: If too high = deafening!
	phase = 0.0;

}

void step16(HPSIRQSource interruptID, bool isInit, void* initParams) {
	if (!isInit) {

		volatile unsigned int * HPS_timer0_ptr = (unsigned int *) 0xFFC08000;
		volatile unsigned int * LED_step_ptr = (unsigned int *) 0xFF200000;

		//Toggle board green LED every step
		unsigned int gpio_rmw;

		*LED_step_ptr = 0x200 >> sequence_step;

		//Check if sound set to trigger on current step
		if (kick.play_sequence[sequence_step]) {
			kick.sample_pt = 0; //Set kick drum sample buffer to 0 - plays from start
		}

		//Increment LEDs to show step then start from beginning again if 7
		if (sequence_step < SEQUENCE_STEPS - 1) {
			sequence_step++;
		} else {
			sequence_step = 0;
		}

		//Clear the Timer Interrupt Flag
		//By reading timer end of interrupt register
		gpio_rmw = HPS_timer0_ptr[3];

	}

	HPS_ResetWatchdog();

}

//When out FIFO is 75% empty
void audioISR(HPSIRQSource interruptID, bool isInit, void* initParams) {

	if (!isInit) {

		volatile unsigned char* fifospace_ptr;
		volatile unsigned int* audio_left_ptr;
		volatile unsigned int* audio_right_ptr;
		volatile unsigned int * fifo_control_ptr = (unsigned int *) 0xFF203040;
		audio_left_ptr = WM8731_getLeftFIFOPtr();
		audio_right_ptr = WM8731_getRightFIFOPtr();

		//Grab the FIFO Space and Audio Channel Pointers
		fifospace_ptr = WM8731_getFIFOSpacePtr();

		//Fill up FIFOs
		unsigned int fifospace;
		fifospace = fifospace_ptr[2];

		//for (int i = 0; i < fifospace; ++i) {

		while ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {
			/*
			 //Get output from kick
			 if (kick_sample_pt < kick.bufferSize) {

			 audioOutputL += kick.sample_buffer[kick_sample_pt] * 1000;
			 audioOutputR += kick.sample_buffer[kick_sample_pt + 1] * 1000;
			 kick_sample_pt += 2;
			 }

			 //Get output from hi hat1

			 // Output summed master output to FIFO buffer
			 *audio_left_ptr = audioOutputL;
			 *audio_right_ptr = audioOutputR;

			 //Reset back to  0
			 audioOutputL = 0;
			 audioOutputR = 0;*/

			//If there is space in the write FIFO for both channels:
			//Increment the phase
			phase = phase + inc;
			//Ensure phase is wrapped to range 0 to 2Pi (range of sin function)
			while (phase >= PI2) {
				phase = phase - PI2;
			}
			// Calculate next sample of the output tone.
			audio_sample = (signed int) (ampl * sin(phase));
			// Output tone to left and right channels.
			*audio_left_ptr = audio_sample;
			*audio_right_ptr = audio_sample;

			//dummyL = *audio_left_ptr;
			//dummyR = *audio_left_ptr;
		}
		HPS_ResetWatchdog();
	}
}

void setup_IRQ() {

	///////////////////////////////////////////////////////////////////
	//////////////////// Timer Interrupt Setup ////////////////////////
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

	volatile unsigned int * KEY_ptr = (unsigned int *) 0xFF200050;

	// Configure Push Buttons to interrupt on press
	KEY_ptr[2] = 0xF;     // Enable interrupts for all four KEYs

	//Register interrupt handler for keys
	HPS_IRQ_registerHandler(IRQ_LSC_KEYS, pushbuttonISR);

	/*********** Timer 1 will be called every time period of the sampling rate Fs, so 1/Fs ******/

//	volatile unsigned int * HPS_timer1_ptr = (unsigned int *) 0xFFC09000;
//
//	// Timer base address
//	HPS_timer1_ptr[2] = 0; // write to control register to stop timer
//
//	/* Set timer period to 1/48kHz (or as close as it can get to it)
//	 */
//	HPS_timer1_ptr[0] = 2083.33333333; //2083.33333333
//	// Write to control register to start timer, with interrupts
//	HPS_timer1_ptr[2] = 0x03; // mode = 1, enable = 1
//
//	/* Call function before registering to handler with IRQ driver
//	 * so can set some of the values inside before interrupts enabled
//	 */
	// Register interrupt handler for audioISR, for when FIFO 75% empty
	//audioISR(IRQ_LSC_AUDIO, true, 0);
	//HPS_IRQ_registerHandler(IRQ_LSC_AUDIO, audioISR);
	//HPS_ResetWatchdog();
}

void fillFIFO() {

	volatile unsigned char* fifospace_ptr;
	volatile unsigned int* audio_left_ptr;
	volatile unsigned int* audio_right_ptr;
	volatile unsigned int * fifo_control_ptr = (unsigned int *) 0xFF203040;
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();

	while ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {

		*audio_left_ptr = 1;
		*audio_right_ptr = 1;
	}

}

void audioPlaybackPolling() {

	//Check for space in outgoing FIFOs (each 128 samples wide)
	if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {
		//While there's space, fill it up with samples

		//for(k = 0; k < (no. of channels); k++)

		//Get output from kick
		if (kick.sample_pt < kick.bufferSize) {
			audioOutputL += kick.sample_buffer[kick.sample_pt] * kick.volume;
			audioOutputR += kick.sample_buffer[kick.sample_pt + 1]
					* kick.volume;
			kick.sample_pt += 2;
		}

		//Get output from clap
		if (clap.sample_pt < clap.bufferSize) {
			audioOutputL += clap.sample_buffer[clap.sample_pt] * clap.volume;
			audioOutputR += clap.sample_buffer[clap.sample_pt + 1]
					* clap.volume;
			clap.sample_pt += 2;
		}

		//Get output from snare
		if (snare.sample_pt < snare.bufferSize) {
			audioOutputL += snare.sample_buffer[snare.sample_pt] * snare.volume;
			audioOutputR += snare.sample_buffer[snare.sample_pt + 1]
					* snare.volume;
			snare.sample_pt += 2;
		}

		//Get output from tom
		if (tom.sample_pt < tom.bufferSize) {
			audioOutputL += tom.sample_buffer[tom.sample_pt] * tom.volume;
			audioOutputR += tom.sample_buffer[tom.sample_pt + 1] * tom.volume;
			tom.sample_pt += 2;
		}

		//Get output from ride
		if (ride.sample_pt < ride.bufferSize) {
			audioOutputL += ride.sample_buffer[ride.sample_pt] * ride.volume;
			audioOutputR += ride.sample_buffer[ride.sample_pt + 1]
					* ride.volume;
			ride.sample_pt += 2;
		}

		//Get output from hato
		if (hato.sample_pt < hato.bufferSize) {
			audioOutputL += hato.sample_buffer[hato.sample_pt] * hato.volume;
			audioOutputR += hato.sample_buffer[hato.sample_pt + 1]
					* hato.volume;
			hato.sample_pt += 2;
		}

		//Get output from hatc
		if (hatc.sample_pt < hatc.bufferSize) {
			audioOutputL += hatc.sample_buffer[hatc.sample_pt] * hatc.volume;
			audioOutputR += hatc.sample_buffer[hatc.sample_pt + 1]
					* hatc.volume;
			hatc.sample_pt += 2;
		}

		//Get output from crash
		if (crash.sample_pt < crash.bufferSize) {
			audioOutputL += crash.sample_buffer[crash.sample_pt] * crash.volume;
			audioOutputR += crash.sample_buffer[crash.sample_pt + 1]
					* crash.volume;
			crash.sample_pt += 2;
		}

		// Output summed master output to FIFO buffer
		*audio_left_ptr = audioOutputL;
		*audio_right_ptr = audioOutputR;

		//Reset back to  0
		audioOutputL = 0;
		audioOutputR = 0;

	}
}

void exitOnFail(signed int status, signed int successStatus) {
	if (status != successStatus) {
		exit((int) status); //Add breakpoint here to catch failure
	}
}

void update7seg() {

	//Display BPM on 7 segment display
	//Hex 0-3
	volatile unsigned int * HEX0to3_ptr = (unsigned int *) 0xFF200020;
	volatile unsigned int * HEX4to5_ptr = (unsigned int *) 0xFF200030;

	//Get digits from  BPMs
	int digit0 = getNthDigit(0, BPM);
	int digit1 = getNthDigit(1, BPM);
	int digit2 = getNthDigit(2, BPM);
	//Get 7 segment display representation
	int hex3 = dec_to_BCD_table(digit0);
	int hex4 = dec_to_BCD_table(digit1);
	int hex5 = dec_to_BCD_table(digit2);

	//Write to registers
	*HEX4to5_ptr = hex4 | (hex5 << 8);
	*HEX0to3_ptr = hex3 << 24;
}

//Lookup table for decimal to 7 segment display BCD value
int dec_to_BCD_table(int no) {

	int result;

	switch (no) {
	case 0:
		result = 0x3F;
		break;
	case 1:
		result = 0x06;
		break;
	case 2:
		result = 0x5B;
		break;
	case 3:
		result = 0x4F;
		break;
	case 4:
		result = 0x66;
		break;
	case 5:
		result = 0x6D;
		break;
	case 6:
		result = 0x7D;
		break;
	case 7:
		result = 0x07;
		break;
	case 8:
		result = 0x7F;
		break;
	case 9:
		result = 0x67;
		break;

	}

	return result;
}

//Get Nth digit of input number
int getNthDigit(int digit, int number) {

	while (digit--)
		number /= 10;
	return number % 10;

}

//For updating playback sequence for the channel currently selected when 'latch,
//button pressed
void latchSequence() {

	//Get position of switches
	volatile unsigned int * sw_ptr = (unsigned int *) SWITCH_PTR;

	volatile unsigned int * HPS_gpio_ptr = (unsigned int *) 0xFF709000;

	unsigned int sw_value = *sw_ptr;

	//Toggle board green LED every step
	unsigned int gpio_rmw;
	gpio_rmw = HPS_gpio_ptr[HPS_GPIO_PORT];
	gpio_rmw = gpio_rmw ^ (1 << 24);
	HPS_gpio_ptr[HPS_GPIO_PORT] = gpio_rmw;

	for (unsigned int i = 0; i < SEQUENCE_STEPS; i++) {

		switch (current_channel) {

		case 0:
			kick.play_sequence[i] = (sw_value >> 9 - i) & 1U;
			break;
		case 7:
			clap.play_sequence[i] = 0;
			break;
		case 1:
			snare.play_sequence[i] = 0;
			break;
		case 2:
			hatc.play_sequence[i] = 0;
			break;
		case 3:
			hato.play_sequence[i] = 0;
			break;
		case 5:
			ride.play_sequence[i] = 0;
			break;
		case 6:
			crash.play_sequence[i] = 0;
			break;
		case 4:
			tom.play_sequence[i] = 0;
			break;
		}

	}

}

//Key Released Interrupt Displays Last Button Released
void pushbuttonISR(HPSIRQSource interruptID, bool isInit, void* initParams) {
	if (!isInit) {
		volatile unsigned int * KEY_ptr = (unsigned int *) 0xFF200050;
		unsigned int press;

		//Read the Push-button interrupt register
		press = KEY_ptr[3];

		//If KEY3 pressed
		//if(press & (1 << 3)){
		//Latch playback sequence from switches
		latchSequence();
		//}

		//Then clear the interrupt flag by writing the value back
		KEY_ptr[3] = press;

	}
	//Reset watchdog.
	HPS_ResetWatchdog();
}
