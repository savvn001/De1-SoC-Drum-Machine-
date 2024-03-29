/*
 * Playback_Engine.h
 *
 *  Created on: 10 May 2019
 *      Author: Nick
 */

#ifndef PLAYBACK_ENGINE_PLAYBACK_ENGINE_H_
#define PLAYBACK_ENGINE_PLAYBACK_ENGINE_H_

#include <stdlib.h>
#include <stdio.h>
#include "../DE1SoC_WM8731/DE1SoC_WM8731.h"
#include "../HPS_Watchdog/HPS_Watchdog.h"
#include "../HPS_usleep/HPS_usleep.h"
#include "../HPS_IRQ/HPS_IRQ.h"
#include "../wav/wav.h"
#include "../ImageDisplay/ImageDisplay.h"
#include <math.h>


void setup_playback();
void setup_codec();
void setup_IRQ();
void update7seg(int bpm);
void step_seq(HPSIRQSource interruptID, bool isInit, void* initParams);
void audioISR(HPSIRQSource interruptID, bool isInit, void* initParams);
int dec_to_BCD_table(int no);
int getNthDigit(int digit, int number);

void exitOnFail(signed int status, signed int successStatus);
void pushbuttonISR(HPSIRQSource interruptID, bool isInit, void* initParams);
void fillFIFO();
void updateTimer(int _timer_val);
void audioPlaybackPolling();
void latchSequence();
void incrementCH();
void updateBPM();
void setup_graphics();
#endif /* PLAYBACK_ENGINE_PLAYBACK_ENGINE_H_ */
