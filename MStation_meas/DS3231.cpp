#include <Arduino.h>
#include <Wire.h>
#include <MStation.h>
#include "DS3231.h"

/*
Functions for controlling DS3231 RTC clock
*/
void ds3231_init()
{
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(0x0E);
	Wire.write(0x00);
	Wire.write(0x00);
	Wire.endTransmission();
}

uint8_t ds3231_read_reg(uint8_t reg)
{
	uint8_t tmp = 0x00;
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(reg);
	Wire.endTransmission();
	Wire.requestFrom(DS3231_ADDR, 1);
	if (Wire.available())
	{
		tmp = Wire.read();
	}

	return tmp;
}

uint8_t ds3231_is_first_run()
{
	uint8_t stat_reg;
	stat_reg = ds3231_read_reg(0x0F);

	return (stat_reg & 0x80);
}

struct datetime ds3231_get_datetime()
{
	uint8_t seconds, minutes, hours, day, date, month, year;
	struct datetime curr_dt;
	// set read addr to 0x00
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(0x00);
	Wire.endTransmission();
	// request data from module
	Wire.requestFrom(DS3231_ADDR, 7);
	if (Wire.available())
	{
		seconds = Wire.read();
		minutes = Wire.read();
		hours = Wire.read();
		day = Wire.read();
		date = Wire.read();
		month = Wire.read();
		year = Wire.read();
	}
	Wire.endTransmission();

	#ifdef DEBUG
	Serial.println(F("DS3231 Time:"));
	Serial.print(F("Seconds: "));
	Serial.println(seconds);
	Serial.print(F("Minutes: "));
	Serial.println(minutes);
	Serial.print(F("Hours: "));
	Serial.println(hours);
	Serial.print(F("Day: "));
	Serial.println(day);
	Serial.print(F("Date: "));
	Serial.println(date);
	Serial.print(F("Month: "));
	Serial.println(month);
	Serial.print(F("Year: "));
	Serial.println(year);
	#endif

	// convert from ds3231 format to MSF-like format
	curr_dt.seconds = (((seconds & 0xF0) >> 4) * 10) + (seconds & 0x0F);
	curr_dt.minutes = (((minutes & 0xF0) >> 4) * 10) + (minutes & 0x0F);
	curr_dt.hours = (((hours & 0x30) >> 4) * 10) + (hours & 0x0F);
	curr_dt.day = day & 0x07;
	curr_dt.date = (((date & 0x30) >> 4) * 10) + (date & 0x0F);
	curr_dt.month = (((month & 0x01) >> 4) * 10) + (month & 0x0F);
	curr_dt.year = (((year & 0xF0) >> 4) * 10) + (year & 0x0F);

	return curr_dt;
}

void ds3231_set_datetime(struct datetime dt)
{
	Wire.beginTransmission(DS3231_ADDR);
	// set addr to 0x00
	Wire.write(0x00);
	// start write data
	Wire.write(dec2bcd(dt.seconds));
	Wire.write(dec2bcd(dt.minutes));
	Wire.write(dec2bcd(dt.hours));
	Wire.write(dec2bcd(dt.day));
	Wire.write(dec2bcd(dt.date));
	Wire.write(dec2bcd(dt.month));
	Wire.write(dec2bcd(dt.year));
	Wire.endTransmission();
}

inline uint8_t dec2bcd(uint8_t dec)
{
	return (((dec / 10) << 4) | (dec % 10));
}