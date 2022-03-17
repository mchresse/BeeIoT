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
//*
//* Input
//*		pin		GPIO pin # of analog port
//* Output
//* 	ADCResult	Port voltage level in MilliVolt
//*
//**************************************************************
uint32_t getespadc(int pin){
uint32_t ADC_Result;	// adc sample value

	ADC_Result = analogRead(pin) + ADC_INCompensation; // read sample value and compensate base level
	ADC_Result = ADC_Result * 3300 / 4095;	// return value in mV

return ADC_Result;
}

