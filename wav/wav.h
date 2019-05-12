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


void initFatFS();

uint32_t readWavFileHeader(const TCHAR* filename);
void FillBufferFromSDcard(int16_t *buffer);

#endif /* WAV_WAV_H_ */
