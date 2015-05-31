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
	Wire.write(0x04);
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

uint8_t ds3231_write_reg(uint8_t addr, uint8_t reg)
{
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(addr);
	Wire.write(reg);
	Wire.endTransmission();
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

void ds3231_set_alarm2(struct alarm_conf *dt, ALARM_MODE mask)
{
	uint8_t minutes, hour, date_day;

	minutes = dec2bcd(dt->minute);
	hour = dec2bcd(dt->hour);
	date_day = dec2bcd(dt->date_day);

	if (mask & 0x02)
		minutes |= _BV(7);
	if (mask & 0x04)
		hour |= _BV(7);
	if (mask & 0x10)
		date_day |= _BV(6);
	if (mask & 0x08)
		date_day |= _BV(7);

	Wire.beginTransmission(DS3231_ADDR);
	// set write addr to 0x0B
	Wire.write(DS3231_A2_ADDR);
	// write data
	Wire.write(minutes);
	Wire.write(hour);
	Wire.write(date_day);
	Wire.endTransmission();
}

void ds3231_alarm2_ctrl(ALARM2_CFG cfg)
{
	uint8_t reg;

	reg = ds3231_read_reg(0x0E);
	if (cfg == A2_ENABLE)
		reg |= _BV(2);
	if (cfg == A2_DISABLE)
		reg &= ~(_BV(2));

	ds3231_write_reg(0x0E, reg);
}

void ds3231_ctrl_INT(uint8_t enable)
{
	uint8_t reg;

	reg = ds3231_read_reg(0x0E);
	if (enable)
		reg |= _BV(3);
	else
		reg &= ~(_BV(3));

	ds3231_write_reg(0x0E, reg);
}

#ifdef DEBUG
void ds3231_dump_alarm2()
{
	uint8_t a2_1, a2_2, a2_3;

	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(DS3231_A2_ADDR);
	Wire.endTransmission();
	Wire.requestFrom(DS3231_ADDR, 3);
	if (Wire.available())
	{
		a2_1 = Wire.read();
		a2_2 = Wire.read();
		a2_3 = Wire.read();
	}
	Serial.println(a2_1, HEX);
	Serial.println(a2_2, HEX);
	Serial.println(a2_3, HEX);
}

void ds3231_dump_cfg()
{
	uint8_t reg;

	reg = ds3231_read_reg(0x0E);
	Serial.println(reg, HEX);
}
#endif

uint8_t dec2bcd(uint8_t dec)
{
	return (((dec / 10) << 4) | (dec % 10));
}