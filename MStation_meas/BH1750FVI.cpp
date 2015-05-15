#include <Arduino.h>
#include <Wire.h>
#include "BH1750FVI.h"

/* 
Function for controlling BH1750FVI sensor 
*/
void bh1750_write_cmd(uint8_t cmd)
{
	Wire.beginTransmission(BH1750_ADDR);
	Wire.write(cmd);
	Wire.endTransmission();
}

uint16_t bh1750_read_data()
{
	uint16_t tmp;

	Wire.requestFrom(BH1750_ADDR, 2);
	tmp = Wire.read();
	tmp = (tmp << 8) | Wire.read();
	//Wire.endTransmission();

	return tmp;
}

uint16_t bh1750_meas_Lmode()
{
	uint16_t val, tmp;

	bh1750_write_cmd(BH1750_PWR_ON);
	bh1750_write_cmd(BH1750_1TIME_LR);
	delay(24);
	tmp = bh1750_read_data();
	#ifdef DEBUG
	Serial.print(F("BH1750 Lmode raw value: "));
	Serial.println(tmp);
	#endif
	val = (float) tmp / 1.2;
	return val;
}

uint16_t bh1750_meas_Hmode()
{
	uint16_t val, tmp;

	bh1750_write_cmd(BH1750_PWR_ON);
	bh1750_write_cmd(BH1750_1TIME_HR);
	delay(180);
	tmp = bh1750_read_data();
	#ifdef DEBUG
	Serial.print(F("BH1750 Hmode raw value: "));
	Serial.println(tmp);
	#endif
	val = (float) tmp / 1.2;
	return val;
}

float bh1750_meas_H2mode()
{
	float val;
	uint16_t tmp, fl;

	bh1750_write_cmd(BH1750_PWR_ON);
	bh1750_write_cmd(BH1750_1TIME_HR2);
	delay(180);
	tmp = bh1750_read_data();
	#ifdef DEBUG
	Serial.print(F("BH1750 H2mode raw value: "));
	Serial.println(tmp);
	#endif
	fl = tmp & 0x0001;
	#ifdef DEBUG
	Serial.println(fl);
	#endif
	tmp >>= 1;
	#ifdef DEBUG
	Serial.println(tmp);
	#endif
	val = (float) tmp / 1.2 + fl * 0.5;
	return val;
}