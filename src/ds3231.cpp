
//*******************************************************************
// ds3231.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// RTC DS3231 I2C access routine based on Espressif driver/i2c library
//
//----------------------------------------------------------
/*
Forked from here:
https://github.com/UncleRus/esp-idf-lib/blob/master/components/ds3231/

The MIT License (MIT)

Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
Copyright (C) 2018 Ruslan V. Uss <unclerus@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ds3231.h"
#include "i2cdev.h"

#define CHECK_ARG(ARG) do { if (!ARG) return ESP_ERR_INVALID_ARG; } while (0)

uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

//Berechnet den Tag der Woche aus dem übergebenen Datumswerten: 1(SU) - 7(SAT)
byte calcDayOfWeek(int jahr, byte monat, byte tag) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    jahr -= monat < 3;
    return ((jahr + jahr/4 - jahr/100 + jahr/400 + t[monat-1] + tag) % 7); 
}

esp_err_t ds3231_init_desc(i2c_dev_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->port = port;
    dev->addr = RTC_ADDR;
    dev->sda_io_num = sda_gpio;
    dev->scl_io_num = scl_gpio;
    dev->clk_speed = 400000; 	// I2C_FREQ_HZ;
 
//    i2c_master_init(port, sda_gpio, scl_gpio);	// already done before
	return(ESP_OK);
}

esp_err_t ds3231_set_time(i2c_dev_t *dev, struct tm *time)
{
    CHECK_ARG(dev);
    CHECK_ARG(time);

    uint8_t data[7];
//    Serial.printf("  DS3231: Set NTP-Time: %i-%02i-%02iT%02i:%02i:%02i\n",time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    /* time/date data */
    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = dec2bcd(calcDayOfWeek(time->tm_year+1900, time->tm_mon+1, time->tm_mday)-1);	// 1-7 -> 0-6
//    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);		// 1-31
    data[5] = dec2bcd(time->tm_mon + 1);	// 0-11 -> 1-12
    data[6] = dec2bcd(time->tm_year - 100);	// base1900 -> base2000

    return i2c_dev_write_reg(dev, DS3231_ADDR_TIME, data, 7);
}

esp_err_t ds3231_get_raw_temp(i2c_dev_t *dev, int16_t *temp)
{
    CHECK_ARG(dev);
    CHECK_ARG(temp);

    uint8_t data[2];

    esp_err_t res = i2c_dev_read_reg(dev, DS3231_ADDR_TEMP, data, sizeof(data));
    if (res == ESP_OK)
        *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;

    return res;
}

esp_err_t ds3231_get_temp_integer(i2c_dev_t *dev, int8_t *temp)
{
    CHECK_ARG(temp);

    int16_t t_int;

    esp_err_t res = ds3231_get_raw_temp(dev, &t_int);
    if (res == ESP_OK)
        *temp = t_int >> 2;

    return res;
}

esp_err_t ds3231_get_temp_float(i2c_dev_t *dev, float *temp)
{
    CHECK_ARG(temp);

    int16_t t_int;

    esp_err_t res = ds3231_get_raw_temp(dev, &t_int);
    if (res == ESP_OK)
        *temp = t_int * 0.25;

    return res;
}

esp_err_t ds3231_get_time(i2c_dev_t *dev, struct tm *time)
{
    CHECK_ARG(dev);
    CHECK_ARG(time);

    uint8_t data[7];

    /* read time */
    esp_err_t res = i2c_dev_read_reg(dev, DS3231_ADDR_TIME, data, 7);
	if (res != ESP_OK) return res;

    /* convert to unix time structure */
    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);					
    if (data[2] & DS3231_12HOUR_FLAG)
    {
        /* 12H */
        time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
    }
    else time->tm_hour = bcd2dec(data[2]); 			// 24H
    time->tm_wday = bcd2dec(data[3]) - 1;			// 1-7 -> 0-6
    time->tm_mday = bcd2dec(data[4]);				// 1-31
    time->tm_mon  = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;	// 1-12 -> 0-11
    time->tm_year = bcd2dec(data[6]) + 100;			// base2000 -> base1900
    time->tm_isdst = 0;

//    Serial.printf("  DS3231: Get NTP-Time: %i-%02i-%02iT%02i:%02i:%02i\n", time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return ESP_OK;
}
