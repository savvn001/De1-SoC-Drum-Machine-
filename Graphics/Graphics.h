/*
 * Graphics.h
 *
 *  Created on: 13 Mar 2019
 *      Author: Nick
 */

#ifndef GRAPHICS_GRAPHICS_H_
#define GRAPHICS_GRAPHICS_H_

#include <stdbool.h> //Boolean variable type "bool" and "true"/"false" constants.

/**
 * 	Graphics library return status definitions
 *
 */
typedef enum{

	GR_SUCCESS = 0x00,
	GR_FAIL_INVALID_COORDINATES = 0x01,

} Graphics_StatusTypeDef;

//Initialise LT24 display driver and check if initialised correctly
Graphics_StatusTypeDef Graphics_initialise(unsigned int lcd_pio_base, unsigned int lcd_hw_base);

//Draw a line of colour starting at x1, y1 and finishing at x2, y2
Graphics_StatusTypeDef Graphics_drawLine(unsigned int x1, unsigned int y1, unsigned int x2,	unsigned int y2, unsigned short colour);

//Draw a rectangle with border in colour
Graphics_StatusTypeDef Graphics_drawBox(unsigned int x1, unsigned int y1, unsigned int x2,	unsigned int y2, unsigned short colour, bool noFill,
unsigned short fillColour);

//Draw a triangle
Graphics_StatusTypeDef Graphics_drawTriangle(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned int x3, unsigned int y3,
unsigned short colour, bool noFill, unsigned short fillColour);

//Draw a circle
Graphics_StatusTypeDef Graphics_drawCircle(unsigned int x, unsigned int y, unsigned int r, unsigned short colour,bool noFill, unsigned short fillColour);


//Some 'private' functions
int calcBarycentric(int x1, int y1, int x2, int y2, int px, int py);
int min3(int a, int b, int c);
int max3(int a, int b, int c);

#endif /* GRAPHICS_GRAPHICS_H_ */
