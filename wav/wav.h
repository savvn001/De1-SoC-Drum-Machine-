/*
 * wav.h
 *
 *  Created on: 29 Mar 2019
 *      Author: Nick
 */
/*
 * Library for reading .wav files from SD card using FatFS driver
 * Has functions to read a .wav file with a given name, inspect its'
 * 44-byte header and store the raw audio data into a buffer
 *
 *
 */
#ifndef WAV_WAV_H_
#define WAV_WAV_H_

#include "../FatFS/ff.h"		/* Declarations of FatFs API */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

FATFS FatFs; /* FatFs work area needed for each volume */
FIL Fil; /* File object needed for each open file */
FIL WAVfile; //Wav file object


/* Memory addresses for DDR memory. DDR memory is split into 1G x 32 bit fields
 * (Address 0x00000000 to 0x3FFFFFFF).
 */

uint32_t ByteCounter = 0;



/** WAV header structure */
typedef struct {

	uint8_t id[4]; 			/** should always contain "RIFF"      */
	uint32_t totallength;	/** total file length minus 8         */
	uint8_t wavefmt[8];  	/** should be "WAVEfmt "              */
	uint32_t format; 		/** Sample format. 16 for PCM format. */
	uint16_t pcm; 			/** 1 for PCM format                  */
	uint16_t channels; 			/** Channels                          */
	uint32_t frequency; 		/** sampling frequency                */
	uint32_t bytes_per_second; 		/** Bytes per second                  */
	uint16_t bytes_per_capture; 		/** Bytes per capture                 */
	uint16_t bits_per_sample;		/** Bits per sample                   */
	uint8_t data[4];				 /** should always contain "data"      */
	uint32_t bytes_in_data; 		/** No. bytes in data                 */
} WAV_Header_TypeDef;

/** Wav header. Global as it is used in callbacks. */
WAV_Header_TypeDef wavHeader;


/*** Function Prototypes ****/


/*
 *  Init fatFS file system
 *
 */
void initFatFS(){

	f_mount(&FatFs, "", 0); /* Give a work area to the default drive */

}

/*
 *	This function reads the contents of the wav files 44-byte header and returns it
 *	in the above struct
 *
 */
uint32_t readWavFileHeader(const TCHAR* filename) {

	UINT bytes_read; //Output number of bytes read from wav file

	if (f_open(&WAVfile, filename, FA_READ) != FR_OK) {
		/* No micro-SD with FAT32, or no WAV_FILENAME found */
		while (1);
	}

	/* Read header and place in header struct */
	f_read(&WAVfile, &wavHeader, sizeof(wavHeader), &bytes_read);

	int bytes =  wavHeader.bytes_in_data;
	return bytes;

}

/*
 * This function fills up a memory buffer with the raw audio data from
 * the given .wav file. The raw audio section starts after the header section,
 * so after byte 44.
 *
 */
void FillBufferFromSDcard(int16_t *buffer)
{

	UINT bytes_read;



	/* Stereo, Store Left and Right data interlaced as in wavfile */
	/* Basically, wav file data section interleaves the samples for
	 * left and right channels so this single array represents the Left and
	 * Right channels of data and is interleaved like [L] [R] [L] [R] [L] [R] etc...
	 * this gets separated later when outputting to codec
	 */

	//Fill buffer with audio data from wav file
	/*As the FIL object fptr value is 44 now, this will now conveniently start collecting everything from
	 * byte 44 onwards
	 */

	f_read(&WAVfile, buffer, wavHeader.bytes_in_data, &bytes_read);
	ByteCounter += bytes_read;


}


#endif /* WAV_WAV_H_ */
