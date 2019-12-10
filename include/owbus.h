//***************************************************
// OneWire connected device parameter file
//***************************************************

#define OW_DEVICES  3
#define ONE_WIRE_RETRY	3

// Physical ID of each DS18B20 OW Device
#define TEMP_0   { 0x28, 0x20, 0x1F, 0x8E, 0x1F, 0x13, 0x1, 0x85}    // -> Internal Weight cell temperature
#define TEMP_1   { 0x28, 0xAA, 0xE4, 0x6D, 0x18, 0x13, 0x2, 0x2F}    // -> BeeHive internal temperature
#define TEMP_2   { 0x28, 0xAA, 0xCA, 0x6A, 0x18, 0x13, 0x2, 0xF3}    // -> External temperature

#define TEMPERATURE_PRECISION 12

// end of owbus.h