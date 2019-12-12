//***************************************************
// HX711 connect weight scale parameter file
//***************************************************



// Bosche H40A with 2mV/V sensibility
// HX711: Vdd=3.3V, Port A with GAIN:128
// -> 3.3V x 2mV / 100kg = 66µV/kg = 66nV/gr    of weight cell
// => 128 x 3.3V/2hoch24 = ~1,5 nV              of HX711 24Bit ADC
// 66nV/Gr / 1,5nV = 44 /gr == 44000/kg
#define scale_DIVIDER 44000     // Kilo unit value (2mV/V, GAIN=128 Vdd=3.3V)

// HX711 Weight Scale offset values for 0.000 kg
#define scale_OFFSET 243000   // 243000 = 44000 * 5.523kg (of the cover weight)


// end of HX711Scale.h