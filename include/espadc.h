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


// ESP ADC Level Comepnsation (rewired for  Vin range between 0..2,8V)
// (by now not used)
#define ADC_BaseComp  0		// Corr. Start level of samples
#define ADC_Factor   100	// factorized angle correction (e.g. *107/100)

#define ADC_MaxVal	 3100	// ADC Sample 4096 = 3,100V

// assign analog ADC pin to BeeIot board
#define	Battery_pin  ADC_VBATT
#define	Charge_pin   ADC_VCHARGE

uint32_t getespadc(int pin);