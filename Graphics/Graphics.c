/*
 * Graphics.c
 *
 *  Created on: 13 Mar 2019
 *      Author: Nick
 */

#include "Graphics.h"
#include <stdlib.h>
#include "../DE1SoC_LT24/DE1SoC_LT24.h"
//#include "../VFP_Enable/VFP_Enable.c"

/*	Driver to create graphics to display on LT24 LCD display
 * 	module
 *
 */

//Initialise LT24 display driver and check if initialised correctly
Graphics_StatusTypeDef Graphics_initialise(unsigned int lcd_pio_base,
		unsigned int lcd_hw_base) {

	bool initialised = LT24_isInitialised();

	LT24_initialise(lcd_pio_base, lcd_hw_base);  //Initialise LCD

	if (!initialised) {
		LT24_initialise(lcd_pio_base, lcd_hw_base);  //Initialise LCD again
	}
	return GR_SUCCESS;

}

//Draw a line of colour starting at x1, y1 and finishing at x2, y2
//Adapted from https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
Graphics_StatusTypeDef Graphics_drawLine(unsigned int x1, unsigned int y1,
		unsigned int x2, unsigned int y2, unsigned short colour) {

	int dx = abs(x2 - x1);
	int sx = x1 < x2 ? 1 : -1; //Increment value for x, either 1 or -1 depending on direction
	//Same for sy
	int dy = abs(y2 - y1);
	int sy = y1 < y2 ? 1 : -1; //Increment value for y, either 1 or -1 depending on direction

	//Error is either dx/2 or dy/2, depending on orientation of line
	int err = (dx > dy ? dx : -dy) >> 1;
	int e2;

	//Check input co ordinates
	if (x1 > LT24_WIDTH || x2 > LT24_WIDTH || y1 > LT24_HEIGHT
			|| y2 > LT24_HEIGHT) {

		return GR_FAIL_INVALID_COORDINATES;
	}

	for (;;) {
		LT24_drawPixel(colour, x1, y1); //Not checking for error message here as x&y values have already been checked
		if (x1 == x2 && y1 == y2) //Exit when done
			break;
		e2 = err;
		if (e2 > -dx) { //Increment x when x axis error is greater than 0.5
			err -= dy;
			x1 += sx;
		}
		if (e2 < dy) { //Same for y axis, increment when y axis error is greater than 0.5
			err += dx;
			y1 += sy;
		}
	}

	return GR_SUCCESS;

}

//Draw a rectangle with border in colour
Graphics_StatusTypeDef Graphics_drawBox(unsigned int x1, unsigned int y1,
		unsigned int x2, unsigned int y2, unsigned short colour,
		bool noFill, unsigned short fillColour) {

	//Check input co ordinates
	if (x1 > LT24_WIDTH || x2 > LT24_WIDTH || y1 > LT24_HEIGHT
			|| y2 > LT24_HEIGHT) {

		return GR_FAIL_INVALID_COORDINATES;
	}

	//Draw lines for box borders
	Graphics_drawLine(x1, y1, x2, y1, colour);
	Graphics_drawLine(x2, y1, x2, y2, colour);
	Graphics_drawLine(x2, y2, x1, y2, colour);
	Graphics_drawLine(x1, y2, x1, y1, colour);

	if (noFill == false) {

		unsigned int i;
		//Simply draw lines from bottom of box to top
		for (i = (y1 + 1); i < y2; i++) {
			Graphics_drawLine(x1 + 1, i, x2 - 1, i, fillColour);
		}

	}
	//If no fill then done
	return GR_SUCCESS;

}

//Draw a triangle, uses rasterization method adapted from https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
Graphics_StatusTypeDef Graphics_drawTriangle(unsigned int x1, unsigned int y1,
		unsigned int x2, unsigned int y2, unsigned int x3, unsigned int y3,
		unsigned short colour, bool noFill, unsigned short fillColour) {

	//Check input co ordinates
	if (x1 > LT24_WIDTH || x2 > LT24_WIDTH || y1 > LT24_HEIGHT
			|| y2 > LT24_HEIGHT) {

		return GR_FAIL_INVALID_COORDINATES;
	}

	//Check if no fill first
	if (noFill == false) {

		//P is the point that will be checked if within triangle
		unsigned int px, py;
		int w1, w2, w3;

		//Get co ordinates of bounding box of triangle
		unsigned int box_xmin = min3(x1, x2, x3);
		unsigned int box_xmax = max3(x1, x2, x3);
		unsigned int box_ymin = min3(y1, y2, y3);
		unsigned int box_ymax = max3(y1, y2, y3);

		//Iterate through bounding box
		for (py = box_ymin; py < box_ymax; py++) {

			for (px = box_xmin; px < box_xmax; px++) {

				//Calculate barycentric coordinates
				w1 = calcBarycentric(x2, y2, x3, y3, px, py);
				w2 = calcBarycentric(x3, y3, x1, y1, px, py);
				w3 = calcBarycentric(x1, y1, x2, y2, px, py);

				//If within triangle (weights are less than 0)
				if (w1 <= 0 && w2 <= 0 && w3 <= 0) {
					LT24_drawPixel(fillColour, px, py);

				}
			}
		}
	}

	//Draw border lines on top
	Graphics_drawLine(x1, y1, x2, y2, colour);
	Graphics_drawLine(x2, y2, x3, y3, colour);
	Graphics_drawLine(x3, y3, x1, y1, colour);

	return GR_SUCCESS;

}

//Draw a circle
Graphics_StatusTypeDef Graphics_drawCircle(unsigned int x, unsigned int y,
		unsigned int r, unsigned short colour, bool noFill,
		unsigned short fillColour) {

	//Midpoint circle drawing algorithm >> https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
	int x_ = r - 1;
	int y_ = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (r << 1);

	//Check input co ordinates
	if (x > LT24_WIDTH || y > LT24_HEIGHT || r > LT24_WIDTH / 2) {

		return GR_FAIL_INVALID_COORDINATES;
	}

	while (x_ >= y_) {

		if (noFill) {

			//If no fill just draw outline
			//Plot 8 octants at the same time,
			LT24_drawPixel(colour, x + x_, y + y_); //First octant
			LT24_drawPixel(colour, x - x_, y + y_); //Mirror to other 7 octants
			LT24_drawPixel(colour, x + x_, y - y_);
			LT24_drawPixel(colour, x - x_, y - y_);
			LT24_drawPixel(colour, x + y_, y + x_);
			LT24_drawPixel(colour, x - y_, y + x_);
			LT24_drawPixel(colour, x + y_, y - x_);
			LT24_drawPixel(colour, x - y_, y - x_);
		} else {

			//In order to fill just draw line across to outline at same time
			Graphics_drawLine(x_ + x, y_ + y, -x_ + x, y_ + y, colour);
			Graphics_drawLine(y_ + x, x_ + y, -y_ + x, x_ + y, colour);
			Graphics_drawLine(y_ + x, -x_ + y, -y_ + x, -x_ + y, colour);
			Graphics_drawLine(x_ + x, -y_ + y, -x_ + x, -y_ + y, colour);

		}

		//Decide whether to increment y or de increment x based off of err value
		if (err <= 0)

		{
			y_++;
			err += dy;
			dy += 2;
		}

		if (err > 0) {
			x_--;
			dx += 2;
			err += dx - (r << 1);
		}

	}

	return GR_SUCCESS;
}

//***************************** Private functions ****************************************

__inline int calcBarycentric(int x1, int y1, int x2, int y2, int px, int py) {

	return ((x2 - x1) * (py - y1)) - ((y2 - y1) * (px - x1));

}

__inline int min3(int a, int b, int c) {

	int min = (a <= b) ? a : b;

	return ((min > c) ? c : min);

}

__inline int max3(int a, int b, int c) {

	int max = (a < b) ? b : a;

	return ((max < c) ? c : max);

}
