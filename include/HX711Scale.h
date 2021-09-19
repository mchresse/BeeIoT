//*******************************************************************
// HX711Scale.h
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// HX711 connect weight scale parameter file
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
#ifndef HX711SCALE_H
#define HX711SCALE_H

// Bosche H40A with 2mV/V sensibility
// HX711: Vdd=3.3V, Port A with GAIN:128
// -> 3.3V x 2mV / 100kg = 66µV/kg = 66nV/gr    of weight cell
// => 128 x 3.3V/2hoch24 = ~1,5 nV              of HX711 24Bit ADC
// 66nV/Gr / 1,5nV = 44 /gr == 44000/kg
#define scale_DIVIDER 44000     // Kilo unit value (2mV/V, GAIN=128 Vdd=3.3V)

// HX711 Weight Scale offset values for 0.000 kg
#define scale_OFFSET 6748   // in kg -> 297000 = 44000 * 6,748kg (of the cover weight)


#endif // end of HX711Scale.h