// Copyright 2022 Randolph Esser. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

void setup_RGB(void);
void setRGB(uint8_t r, uint8_t g, uint8_t b);

void RGBtest();
void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);

