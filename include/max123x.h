//*******************************************************************
// max123x.h
// extracted from Project
// https://gitlab.com/TeeFirefly/linux-kernel/-/blob/941e22553dc5b8ddd0d363296499966c7ad29818/drivers/iio/adc/max1363.c
// Description:
// ADC MAX123x 4/12 bit AD converter
//
//----------------------------------------------------------
 /* Copyright (C) 2008-2010 Jonathan Cameron
  *
  * based on linux/drivers/i2c/chips/max123x
  * Copyright (C) 2002-2004 Stefan Eletzhofer
  *
  * based on linux/drivers/acron/char/pcf8583.c
  * Copyright (C) 2000 Russell King
  *
  * Driver for max1363 and similar chips.
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */
#ifndef __MAX123X_H__
#define __MAX123X_H__

#include <Arduino.h>
//*******************************************************************

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT  I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN	0x1
#define ACK_CHECK_DIS	0x0
#define NACK_VAL		0x1

#define MAX1363_SETUP_BYTE(a)   ((a) | 0x80)
#define MAX1363_SLAVEADDR						0x80
/* There is a fair bit more defined here than currently
 * used, but the intention is to support everything these
 * chips do in the long run
 */

// MAX Init Setup & Config register definitions
#define MAX1363_SETUPREG						0x80

// see data sheets: max1363 and max1236, max1237, max1238, max1239
#define MAX1363_SETUP_AIN3_IS_AIN3_REF_IS_VDD	0x00
#define MAX1363_SETUP_AIN3_IS_REF_EXT_TO_REF	0x20
#define MAX1363_SETUP_AIN3_IS_AIN3_REF_IS_INT	0x40
#define MAX1363_SETUP_AIN3_IS_REF_REF_IS_INT	0x60
#define MAX1363_SETUP_POWER_UP_INT_REF			0x10
#define MAX1363_SETUP_POWER_DOWN_INT_REF		0x00

#define MAX1363_SETUP_EXT_CLOCK					0x08
#define MAX1363_SETUP_INT_CLOCK					0x00
#define MAX1363_SETUP_UNIPOLAR					0x00
#define MAX1363_SETUP_BIPOLAR					0x04
#define MAX1363_SETUP_RESET						0x00
#define MAX1363_SETUP_NORESET					0x02

/* max1363 only - though don't care on others.
 * For now monitor modes are not implemented as the relevant
 * line is not connected on my test board.
 * The definitions are here as I intend to add this soon.
 */
#define MAX1363_SETUP_MONITOR_SETUP				0x01
#define MAX1363_MON_RESET_CHAN(a) 				(1 << ((a) + 4))
#define MAX1363_MON_INT_ENABLE					0x01

#define MAX1363_CONFIGREG						0x00
#define MAX1363_CONFIG_BYTE(a) ((a))
#define MAX1363_CHANNEL_SEL(a) ((a) << 1)

#define MAX1363_CONFIG_SE						0x01	// between AINx and Gnd
#define MAX1363_CONFIG_DE						0x00	// differential ended
#define MAX1363_CONFIG_SCAN_TO_CS				0x00	// Scan from AIN0 ... AIN<CS0+1>
#define MAX1363_CONFIG_SCAN_SINGLE_8			0x20	// convert channel AIN<CS0+1> 8 times
#define MAX1363_CONFIG_SCAN_MONITOR_MODE		0x40	// convert AIN2 - AIN3 (CS0+1 = 3)
#define MAX1363_CONFIG_SCAN_SINGLE_1			0x60	// convert channel AIN<CS0+1> only
#define MAX1363_SCAN_MASK						0x60
/* max123{6-9} only */
#define MAX1236_SCAN_MID_TO_CHANNEL				0x40

/* max1363 only - merely part of channel selects or don't care for others */
#define MAX1363_CONFIG_EN_MON_MODE_READ 		0x18

#define MAX1363_CHANNEL_SEL_MASK				0x1E
/* max123{6-7} only */
#define MAX123x_CHANNEL_SEL_MASK				0x06
#define MAX1363_SE_DE_MASK						0x01

#define MAX1363_MAX_CHANNELS 					25
#define MAX1236_MAX_CHANNELS 					4
#define MAX1237_MAX_CHANNELS 					4

// channel data field
typedef int adata_t;



//*******************************************************************
// some bit handling makros
// e.g. DECLARE_BITMAP(my_bitmap, 64) expands to (((64) + (64) - 1) / (64))
#define BITS_PER_BYTE           	8
#define DIV_ROUND_UP(n,d) 			(((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)       	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define DECLARE_BITMAP(name,bits)	unsigned long name[BITS_TO_LONGS(bits)]

#define ADDR                BITOP_ADDR(addr)
#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#define CONST_MASK_ADDR(nr, addr)    BITOP_ADDR((void *)(addr) + ((nr)>>3))
#define CONST_MASK(nr)            	(1 << ((nr) & 7))	// byte representation of bit nr.
static inline void __set_bit(long nr, volatile unsigned long *addr){
    asm volatile("bts %1,%0" : ADDR : "Ir" (nr) : "memory");
}
static inline void __clear_bit(long nr, volatile unsigned long *addr){
    asm volatile("btr %1,%0" : ADDR : "Ir" (nr));
}
static inline void __toggle_bit(long nr, volatile unsigned long *addr){
    asm volatile("btc %1,%0" : ADDR : "Ir" (nr));
}
//*******************************************************************

int setup_i2c_MAX(int reentry);
int i2c_scan(void);
uint16_t adc_read(int channel);
uint16_t max_read(uint8_t channel);


class MAX123X {
	protected:
	public:
		MAX123X();
		virtual ~MAX123X() {};

	//    esp_err_t adc_read(adata_t* data, int channel);
	private:

};

#endif // __MAX123X_H_
