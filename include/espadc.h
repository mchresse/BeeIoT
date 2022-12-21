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


#define ADC1_IN3  39 // GPIO 39 is Now AN Input 1 ADC1-channel 3
#define ADC1_IN0  36 // GPIO 36 is Now AN Input 2 ADC1-channel 0
#define ADC1_IN6  34 // GPIO 34 is Now AN Input 3 ADC1-channel 6
#define ADC1_IN7  35 // GPIO 35 is Now AN Input 4 ADC1-channel 7
#define ADC1_IN4  32 // GPIO 32 is Now AN Input 5 ADC1-channel 4
#define ADC1_IN5  33 // GPIO 33 is Now AN Input 6 ADC1-channel 5
#define ADC2_IN7  27 // GPIO 27 is Now AN Input 7 ADC2-channel 7

// ESP ADC Level Comepnsation (rewired for  Vin range between 0..2,8V)
// (by now not used)
#define ADC_BaseComp  0		// Corr. Start level of samples
#define ADC_Factor   100	// factorized angle correction (e.g. *107/100)

#define ADC_MaxVal	 3100	// ADC Sample 4096 = 3,100V

// assign analog ADC pin to BeeIot board
#define	Battery_pin  ADC1_IN3
#define	Charge_pin   ADC1_IN0

uint32_t getespadc(int pin);