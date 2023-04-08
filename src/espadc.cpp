// Copyright 2022 mchresse
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Arduino.h>
#include "espadc.h"

extern uint16_t	lflags;      			// BeeIoT log flag field

//**************************************************************
//* getespadc(pin)
//* Read Sample value from ESP32-port by internal ADC machine.
//* Good Values only between 0 - 2,8V achievable !!
//* 	>2,8V curve flatening undershooting by up to 0,25V (3,3V)
//* Input
//*		pin		GPIO pin # of analog port
//* Output
//* 	ADCResult	Port voltage level in MilliVolt
//*
//**************************************************************
uint32_t getespadc(int pin){
uint32_t ADC_Result;	// adc sample value

	adcAttachPin(pin);

//	ADC_Result = analogRead(pin);
//	Serial.printf("\n    ADC(%d) Sample = %i  -", pin, ADC_Result);
//	ADC_Result = (ADC_Result + ADC_BaseComp) * ADC_Factor/100; 			// read raw sample value and compensate base level
//	ADC_Result = ADC_Result * ADC_MaxVal / 4095;						// return value in mV (by 12 Bit sampling: 0..4095)
// 	Serial.printf(" -> %imV  =>", ADC_Result);

	ADC_Result = analogReadMilliVolts(pin); 							// read mV sample value and compensate base level
//  	Serial.printf(" - AD-Sample[%i] = %imV\n", pin, ADC_Result);

return ADC_Result;
}

