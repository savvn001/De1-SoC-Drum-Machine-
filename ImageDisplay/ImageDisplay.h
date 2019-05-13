/*
 * ImageDisplay.h
 *
 *  Created on: 13 May 2019
 *      Author: Nick
 */

#ifndef IMAGEDISPLAY_IMAGEDISPLAY_H_
#define IMAGEDISPLAY_IMAGEDISPLAY_H_

#include "../Graphics/Graphics.h"
#include "../DE1SoC_LT24/DE1SoC_LT24.h"
#include "Images.h"
#include "../HPS_Watchdog/HPS_Watchdog.h"
#include <stdint.h>

void initDisplay();
void drawUI(int channel, int16_t *sample_buffer, int length );
void plot(int16_t *sample_buffer, int length);

#endif /* IMAGEDISPLAY_IMAGEDISPLAY_H_ */
