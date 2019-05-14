/*
 * Playback_Engine.c

 *
 * Driver for main playback engine. Handles loading of SD card data into memory and audio playback
 *
 *  Created on: May 2019
 *  Author: Nicholas Savva
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

/***************** Global variables for this driver *********************/
int BPM = 128;
//Current step of sequence
int sequence_step = 0;

//Codec memory mapped peripheral pointers
volatile unsigned char* fifospace_ptr;
volatile unsigned int* audio_left_ptr;
volatile unsigned int* audio_right_ptr;

//Timer
volatile unsigned int * HPS_timer0_ptr = (unsigned int *) 0xFFC08000;

//For audio playback
signed int audioOutputL = 0;
signed int audioOutputR = 0;

int bpm_val = 128;

//Flags for interrupt functions
bool latchSequence_flag = 0;
bool incrementCH_flag = 0;
bool BPM_up_flag = 0;
bool BPM_down_flag = 0;

//Struct for holding information for each sound
struct channel {

	bool play_sequence[SEQUENCE_STEPS];	//Playback sequence
	unsigned int bufferSize; 		//Number of elements in sample data
	int16_t *sample_buffer;		//Pointer to buffer storing sample data
	int16_t channels; 			//Number of channels ie. 1 = mono, 2 = stereo
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

/*
 * Read samples from SD card and store in DDR memory
 */
void setup_playback() {

	///////////////////////////////////////////////////////////////////
	//////////////////// SD Card Setup /////////////////////////////////
	///////////////////////////////////////////////////////////////////
	initFatFS();

	/*** Get audio from .wav files for each channel and store in buffer ***/

	///////////////////Kick/////////////////////////////////////////////////////////////////////
	//Calculate size of buffer from header value which returns no. of bytes in data
	kick.bufferSize = readWavFileHeader("kick.wav");
	//Create array to store audio samples
	kick.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * kick.bufferSize);
	//Fill with data
	FillBufferFromSDcard(kick.sample_buffer);
	kick.sample_pt = kick.bufferSize + 5; //Set it to outside range intially so it doesn't trigger
	kick.volume = 100000;

	HPS_ResetWatchdog();

	///////////////////clap/////////////////////////////////////////////////////////////////////
	clap.bufferSize = readWavFileHeader("clap.wav");
	clap.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * clap.bufferSize);
	FillBufferFromSDcard(clap.sample_buffer);
	clap.sample_pt = clap.bufferSize + 5;
	clap.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////snare/////////////////////////////////////////////////////////////////////
	snare.bufferSize = readWavFileHeader("snare.wav");
	snare.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * snare.bufferSize);
	FillBufferFromSDcard(snare.sample_buffer);
	snare.sample_pt = snare.bufferSize + 5;
	snare.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////ride/////////////////////////////////////////////////////////////////////
	ride.bufferSize = readWavFileHeader("ride.wav");
	ride.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * ride.bufferSize);
	FillBufferFromSDcard(ride.sample_buffer);
	ride.sample_pt = ride.bufferSize + 5;
	ride.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////tom/////////////////////////////////////////////////////////////////////
	tom.bufferSize = readWavFileHeader("tom.wav");
	tom.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * tom.bufferSize);
	FillBufferFromSDcard(tom.sample_buffer);
	tom.sample_pt = tom.bufferSize + 5;
	tom.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////hatc/////////////////////////////////////////////////////////////////////
	hatc.bufferSize = readWavFileHeader("hatc.wav");
	hatc.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * hatc.bufferSize);
	FillBufferFromSDcard(hatc.sample_buffer);
	hatc.sample_pt = hatc.bufferSize + 5;
	hatc.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////hato/////////////////////////////////////////////////////////////////////
	hato.bufferSize = readWavFileHeader("hato.wav");
	hato.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * hato.bufferSize);
	FillBufferFromSDcard(hato.sample_buffer);
	hato.sample_pt = hato.bufferSize + 2;
	hato.volume = 100000;
	HPS_ResetWatchdog();
	///////////////////crash/////////////////////////////////////////////////////////////////////
	crash.bufferSize = readWavFileHeader("crash.wav");
	crash.sample_buffer = (int16_t*) malloc(sizeof(int16_t) * crash.bufferSize);
	FillBufferFromSDcard(crash.sample_buffer);
	crash.sample_pt = crash.bufferSize + 5;
	crash.volume = 100000;
	HPS_ResetWatchdog();
}

/*
 * Initialize WM8731 codec
 */
void setup_codec() {

	/////////////////////////////////////////////////////////////////////////////
	//////////////////////// Audio Codec Setup //////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	//Initialise the Audio Codec.
	exitOnFail(WM8731_initialise(0xFF203040),  //Initialise Audio Codec
	WM8731_SUCCESS);                //Exit if not successful

	//Clear both FIFOs
	WM8731_clearFIFO(true, true);

	//Grab the FIFO Space and Audio Channel Pointers
	fifospace_ptr = WM8731_getFIFOSpacePtr();
	audio_left_ptr = WM8731_getLeftFIFOPtr();
	audio_right_ptr = WM8731_getRightFIFOPtr();

}

/*
 * Initialize LCD and graphics engine
 */
void setup_graphics() {

	initDisplay();
	//Draw box to cover background
	Graphics_drawBox(0, 0, 239, 319, LT24_BLACK, 0, LT24_BLACK);
	HPS_ResetWatchdog();
	//Draw current sample name on LCD
	drawUI(current_channel, kick.sample_buffer, kick.bufferSize);

}

/*
 * Each step of the 8 step sequence this will get called by HPS timer0
 */
void step_seq(HPSIRQSource interruptID, bool isInit, void* initParams) {
	if (!isInit) {

		volatile unsigned int * HPS_timer0_ptr = (unsigned int *) 0xFFC08000;
		volatile unsigned int * LED_step_ptr = (unsigned int *) 0xFF200000;

		//Toggle board green LED every step
		unsigned int gpio_rmw;

		//Move currently lit LED rights
		*LED_step_ptr = 0x200 >> sequence_step;

		//Check if sound set to trigger on current step
		if (kick.play_sequence[sequence_step]) {
			kick.sample_pt = 0; //Set kick drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (snare.play_sequence[sequence_step]) {
			snare.sample_pt = 0; //Set snare drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (clap.play_sequence[sequence_step]) {
			clap.sample_pt = 0; //Set clap drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (tom.play_sequence[sequence_step]) {
			tom.sample_pt = 0; //Set tom drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (crash.play_sequence[sequence_step]) {
			crash.sample_pt = 0; //Set crash drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (hatc.play_sequence[sequence_step]) {
			hatc.sample_pt = 0; //Set hatc drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (hato.play_sequence[sequence_step]) {
			hato.sample_pt = 0; //Set hato drum sample buffer to 0 - plays from start
		}
		//Check if sound set to trigger on current step
		if (ride.play_sequence[sequence_step]) {
			ride.sample_pt = 0; //Set ride drum sample buffer to 0 - plays from start
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

		volatile unsigned int * fifo_ctrl_ptr = (unsigned int *) 0xFF203040;
		unsigned int fifo_ctrl = *fifo_ctrl_ptr;

		bool WI = (fifo_ctrl >> 9) & 1U;

		//while (WI) {

		//for (int k = 0; k < fifospace_ptr[2]; ++k) {

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
		int l = *audio_left_ptr;
		int r = *audio_right_ptr;

		//Reset back to  0
		audioOutputL = 0;
		audioOutputR = 0;

		HPS_ResetWatchdog();
	}

}

void setup_IRQ() {

	///////////////////////////////////////////////////////////////////
	//////////////////// Timer Interrupt Setup ////////////////////////
	///////////////////////////////////////////////////////////////////

	/****** Timer 0 will be called every 1/6th of a beat, used for the step sequencer *******/
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

	HPS_timer0_ptr[0] = 11718750;
	// Write to control register to start timer, with interrupts
	HPS_timer0_ptr[2] = 0x03; // mode = 1, enable = 1
	// Register interrupt handler for timer
	HPS_IRQ_registerHandler(IRQ_TIMER_L4SP_0, step_seq);
	HPS_ResetWatchdog();

	volatile unsigned int * KEY_ptr = (unsigned int *) 0xFF200050;

	// Configure Push Buttons to interrupt on press
	KEY_ptr[2] = 0xF;     // Enable interrupts for all four KEYs

	//Register interrupt handler for keys
	HPS_IRQ_registerHandler(IRQ_LSC_KEYS, pushbuttonISR);

	/*********** Timer 1  ******/

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
//	HPS_IRQ_registerHandler(IRQ_LSC_AUDIO, audioISR);
//	HPS_ResetWatchdog();
//	//Enable interrupts on codec
//	WM8731_enableIRQ();
}

void fillFIFO() {

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
			audioOutputR += kick.sample_buffer[kick.sample_pt] * kick.volume;
			kick.sample_pt++;
		}

		//Get output from clap
		if (clap.sample_pt < clap.bufferSize) {
			audioOutputL += clap.sample_buffer[clap.sample_pt] * clap.volume;
			audioOutputR += clap.sample_buffer[clap.sample_pt] * clap.volume;
			clap.sample_pt += 1;
		}

		//Get output from snare
		if (snare.sample_pt < snare.bufferSize) {
			audioOutputL += snare.sample_buffer[snare.sample_pt] * snare.volume;
			audioOutputR += snare.sample_buffer[snare.sample_pt] * snare.volume;
			snare.sample_pt += 1;
		}

		//Get output from tom
		if (tom.sample_pt < tom.bufferSize) {
			audioOutputL += tom.sample_buffer[tom.sample_pt] * tom.volume;
			audioOutputR += tom.sample_buffer[tom.sample_pt] * tom.volume;
			tom.sample_pt += 1;
		}

		//Get output from ride
		if (ride.sample_pt < ride.bufferSize) {
			audioOutputL += ride.sample_buffer[ride.sample_pt] * ride.volume;
			audioOutputR += ride.sample_buffer[ride.sample_pt] * ride.volume;
			ride.sample_pt += 1;
		}

		//Get output from hato
		if (hato.sample_pt < hato.bufferSize) {
			audioOutputL += hato.sample_buffer[hato.sample_pt] * hato.volume;
			audioOutputR += hato.sample_buffer[hato.sample_pt] * hato.volume;
			hato.sample_pt += 1;
		}

		//Get output from hatc
		if (hatc.sample_pt < hatc.bufferSize) {
			audioOutputL += hatc.sample_buffer[hatc.sample_pt] * hatc.volume;
			audioOutputR += hatc.sample_buffer[hatc.sample_pt] * hatc.volume;
			hatc.sample_pt += 1;
		}

		//Get output from crash
		if (crash.sample_pt < crash.bufferSize) {
			audioOutputL += crash.sample_buffer[crash.sample_pt] * crash.volume;
			audioOutputR += crash.sample_buffer[crash.sample_pt] * crash.volume;
			crash.sample_pt += 1;
		}

		HPS_ResetWatchdog();

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

/*
 * Get Nth digit of input number
 */
int getNthDigit(int digit, int number) {

	while (digit--)
		number /= 10;
	return number % 10;

}



/*
 * This ISR is for handling when any of the push buttons (KEY0-3) are pressed
 */
void pushbuttonISR(HPSIRQSource interruptID, bool isInit, void* initParams) {

	if (!isInit) {
		volatile unsigned int * KEY_ptr = (unsigned int *) 0xFF200050;
		unsigned int press;

		//Read the Push-button interrupt register
		press = KEY_ptr[3];

		//If KEY3 pressed
		if (press & (1 << 3)) {
			//Latch playback sequence from switches
			latchSequence_flag = 1;
		}

		//If KEY2 pressed
		if (press & (1 << 2)) {
			incrementCH_flag = 1;
		}

		//KEY1
		if (press & (1 << 1)) {
			//Reduce BPM
			BPM_down_flag = 1;
		}

		//KEY0
		if (press & (1 << 0)) {
			//Speed up BPM
			BPM_up_flag = 1;
		}

		//Then clear the interrupt flag by writing the value back
		KEY_ptr[3] = press;

	}
	//Reset watchdog.
	HPS_ResetWatchdog();
}

/*
 *  For updating playback sequence for the channel currently selected when 'latch,
 *	button pressed
 *
 */
void latchSequence() {

	if (latchSequence_flag) {
		//Get position of switches
		volatile unsigned int * sw_ptr = (unsigned int *) SWITCH_PTR;

		volatile unsigned int * HPS_gpio_ptr = (unsigned int *) 0xFF709000;

		unsigned int sw_value = *sw_ptr;

		//Toggle board green LED every step
		unsigned int gpio_rmw;
		gpio_rmw = HPS_gpio_ptr[HPS_GPIO_PORT];
		gpio_rmw = gpio_rmw ^ (1 << 24);
		HPS_gpio_ptr[HPS_GPIO_PORT] = gpio_rmw;

		//Loop through and assign switch value to playback array
		for (unsigned int i = 0; i < SEQUENCE_STEPS; i++) {

			switch (current_channel) {

			case 0:
				kick.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 7:
				clap.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 1:
				snare.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 2:
				hatc.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 3:
				hato.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 5:
				ride.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 6:
				crash.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			case 4:
				tom.play_sequence[i] = (sw_value >> 9 - i) & 1U;
				break;
			}

		}

		//Turn off flag
		latchSequence_flag = 0;
	}
}

void incrementCH() {

	if (incrementCH_flag) {

		//Increment channel
		if (current_channel < 7) {
			current_channel++;
		} else {
			current_channel = 0;
		}

		drawUI(current_channel, kick.sample_buffer, kick.bufferSize);
		incrementCH_flag = 0;
	}
}

void updateBPM() {

	int timer_val = 0;

	if (BPM_up_flag || BPM_down_flag) {
		//Calculate new timer value
		timer_val = 1500000000 / BPM;
	}

	if (BPM_up_flag) {
		BPM++;
		update7seg(BPM);
		updateTimer(timer_val);
		BPM_up_flag = 0;
	}

	if (BPM_down_flag) {
		BPM--;
		update7seg(BPM);
		updateTimer(timer_val);
		BPM_down_flag = 0;
	}

}

//Have to update timer every time we change BPM
void updateTimer(int _timer_val) {

	//Stop timer whilst we change it's load value
	HPS_timer0_ptr[2] = 0;
	//Set timer period again
	HPS_timer0_ptr[0] = _timer_val;
	//Set timer on again
	HPS_timer0_ptr[2] = 0x03; // mode = 1, enable = 1

}

void update7seg(int _bpm) {

	//Display BPM on 7 segment display
	//Hex 0-3
	volatile unsigned int * HEX0to3_ptr = (unsigned int *) 0xFF200020;
	volatile unsigned int * HEX4to5_ptr = (unsigned int *) 0xFF200030;

	//Get digits from  BPMs
	int digit0 = getNthDigit(0, _bpm);
	int digit1 = getNthDigit(1, _bpm);
	int digit2 = getNthDigit(2, _bpm);
	//Get 7 segment display representation
	int hex3 = dec_to_BCD_table(digit0);
	int hex4 = dec_to_BCD_table(digit1);
	int hex5 = dec_to_BCD_table(digit2);

	//Write to registers
	*HEX4to5_ptr = hex4 | (hex5 << 8);
	*HEX0to3_ptr = hex3 << 24;

}
