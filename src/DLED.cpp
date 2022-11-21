// Copyright 2022 Randolph Esser. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

//*****************************************************************
// RGB-LED control module for WS2812B
// WS2812B LED Pinout Configuration
//  Pin Number	Pin Name	Description
//	  1			VDD			LED power supply pin 3.5V to 5.3V
//	  2			DOUT		Data signal output pin
//	  3			GND			Ground reference supply pin
//	  4			DIN			Data signal input pin
//		TxHigh	TxLow
//		0,4us	0,85us	-> '0'
//		0,8us	0,45us	-> '1'
//		0		50us	-> Reset
//
// 24bit Bitstream defining colour mode:	G7..G0, R7..R0, B7..B0
//*****************************************************************

#include <Arduino.h>
#include "beeiot.h"
#include <Adafruit_NeoPixel.h>
#include "DLED.h"

// LEDRGB pin IO15
#define LEDNUM	1		// Length of RGB LED chain 1..n

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDNUM, LEDRGB, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup_RGB(void) {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
	pinMode(LEDRGB, OUTPUT);
	digitalWrite(LEDRGB, HIGH); // signal Setup Phase
	gpio_hold_dis(LEDRGB);   	// enable SD_CS

  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
}

// Set 1. RGB-LED colour to ON
void setRGB(uint8_t r, uint8_t g, uint8_t b){
    strip.setPixelColor(0, strip.Color(r,g,b));
    strip.show();
}

//=========================================================
// Setup Test LED Pin
int LEDstate=HIGH;	// LED status =High:Off, =Low:On
void setup_LED(void){
	pinMode(LEDRED, OUTPUT);
	LEDOff();
}

// Switch LED pin ON (assumed connected to 3.3V)
void LEDOn(void){
	LEDstate =LOW;
	digitalWrite(LEDRED, LEDstate);
}

// Switch LED pin Off (assumed connected to 3.3V)
void LEDOff(void){
	LEDstate =HIGH;
	digitalWrite(LEDRED, LEDstate);
}

// Toggle LED pin ON/Off (assumed connected to 3.3V)
void LEDtoggle(void){
	LEDstate ^= 1;	// Xor LED state
	digitalWrite(LEDRED, LEDstate);
}

// Show shortest pulse on LED pin Off/On/Off (assumed connected to 3.3V)
void LEDpulse(int dtime){
	LEDOn();
	delay(dtime);
	LEDOff();
}

//=========================================================


void RGBtest() {
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
//colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  // Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127, 0, 0), 50); // Red
  theaterChase(strip.Color(0, 0, 127), 50); // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}