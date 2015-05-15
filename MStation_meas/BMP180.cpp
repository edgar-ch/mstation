#include <Arduino.h>
#include <Wire.h>
#include "BMP180.h"

struct BMP180_COEFF bmp180_coeff;
struct BMP180_REGS bmp180_regs;

/*
Functions for controlling BMP180 sensor
*/

// read 2 bytes from sensor
uint16_t bmp180_read16(uint8_t reg)
{
	uint16_t tmp;
	// write address of register
	Wire.beginTransmission(BMP180_WRITE_ADDR);
	Wire.write(reg);
	Wire.endTransmission();
	// request 2 bytes
	Wire.requestFrom(BMP180_READ_ADDR, 2);
	tmp = Wire.read();
	tmp = (tmp << 8) | Wire.read();
	//Wire.endTransmission();
	
	return tmp;
}
// read 1 byte from sensor
uint8_t bmp180_read8(uint8_t reg)
{
	uint8_t tmp;
	// write addres of register
	Wire.beginTransmission(BMP180_WRITE_ADDR);
	Wire.write(reg);
	Wire.endTransmission();
	// request 1 byte
	Wire.requestFrom(BMP180_READ_ADDR, 1);
	tmp = Wire.read();
	//Wire.endTransmission();

	return tmp;
}
// read calibration table from sensor
void bmp180_read_calibration()
{
	bmp180_coeff.ac1 = bmp180_read16(BMP180_REG_ADDR_AC1);
	bmp180_coeff.ac2 = bmp180_read16(BMP180_REG_ADDR_AC2);
	bmp180_coeff.ac3 = bmp180_read16(BMP180_REG_ADDR_AC3);
	bmp180_coeff.ac4 = bmp180_read16(BMP180_REG_ADDR_AC4);
	bmp180_coeff.ac5 = bmp180_read16(BMP180_REG_ADDR_AC5);
	bmp180_coeff.ac6 = bmp180_read16(BMP180_REG_ADDR_AC6);
	bmp180_coeff.b1 = bmp180_read16(BMP180_REG_ADDR_B1);
	bmp180_coeff.b2 = bmp180_read16(BMP180_REG_ADDR_B2);
	bmp180_coeff.mb = bmp180_read16(BMP180_REG_ADDR_MB);
	bmp180_coeff.mc = bmp180_read16(BMP180_REG_ADDR_MC);
	bmp180_coeff.md = bmp180_read16(BMP180_REG_ADDR_MD);
}
// check that device on this address really bmp180 sensor
void bmp180_init()
{
	bmp180_regs.id = bmp180_read8(BMP180_REG_ADDR_ID);
	if (bmp180_regs.id != 0x55)
	{
		#ifdef DEBUG
		Serial.println(F("Not BMP180 device ID !!!"));
		Serial.println(bmp180_regs.id);
		#endif
		return;
	}
	bmp180_regs.ctrl = bmp180_read8(BMP180_REG_ADDR_CTRL_MEAS);
}
// read raw temperature value from regs
int32_t bmp180_read_raw_temp()
{
	int32_t ut;
	// write temperature measurement command
	Wire.beginTransmission(BMP180_WRITE_ADDR);
	Wire.write(BMP180_REG_ADDR_CTRL_MEAS);
	Wire.write(BMP180_CMD_TEMP);
	Wire.endTransmission();
	// wait
	delay(5);
	// read data
	ut = bmp180_read16(BMP180_REG_ADDR_MSB);

	return ut;
}
// read raw pressure value from regs
int32_t bmp180_read_raw_press(uint8_t oss)
{
	uint16_t tmp;
	int32_t up;
	// write pressure measurement command
	Wire.beginTransmission(BMP180_WRITE_ADDR);
	Wire.write(BMP180_REG_ADDR_CTRL_MEAS);
	Wire.write(BMP180_CMD_PRESS | (oss << 6));
	Wire.endTransmission();
	// wait depend on OSS setting
	switch (oss) {
		case 0:
			delay(5);
			break;
		case 1:
			delay(8);
			break;
		case 2:
			delay(14);
			break;
		case 3:
			delay(26);
		default:
			delay(5);
			break;
	}
	// read data from REGs
	tmp = bmp180_read16(BMP180_REG_ADDR_MSB);
	up = (uint32_t) tmp << 8;
	up |= bmp180_read8(BMP180_REG_ADDR_XLSB);
	up = up >> (8 - oss);

	return up;
}
// calc B5 coeff
int32_t bmp180_calc_b5(int32_t ut)
{
	int32_t x1, x2, b5;

	x1 = ((ut - bmp180_coeff.ac6) * bmp180_coeff.ac5) >> 15;
	x2 = ((int32_t) bmp180_coeff.mc << 11) / (x1 + bmp180_coeff.md);
	b5 = x1 + x2;

	return b5;
}
// get real value of temperature
int32_t bmp180_get_temp()
{
	int32_t ut, t;
	// read raw values of regs
	ut = bmp180_read_raw_temp();
	// calc real temperature
	t = (bmp180_calc_b5(ut) + 8) >> 4;

	return t;
}
// get real value of pressure
int32_t bmp180_get_pressure(uint8_t oss)
{
	int32_t ut, up, x1, x2, x3, b5, b3, b6, p;
	uint32_t b4, b7;
	// read raw registers values
	ut = bmp180_read_raw_temp();
	up = bmp180_read_raw_press(oss);
	// doing some *__* MAGICK *v_v*
	b5 = bmp180_calc_b5(ut);
	b6 = b5 - 4000;
	x1 = (bmp180_coeff.b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (bmp180_coeff.ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = ((((bmp180_coeff.ac1 * 4) + x3) << oss) + 2) >> 2;
	x1 = (bmp180_coeff.ac3 * b6) >> 13;
	x2 = (bmp180_coeff.b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (bmp180_coeff.ac4 * (uint32_t) (x3 + 32768)) >> 15;
	b7 = ((uint32_t) up - b3) * (50000 >> oss);
	if (b7 < 0x80000000) {
		p = (b7 << 1) / b4;
	} else {
		p = (b7 / b4) >> 1;
	}
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	p = p + ((x1 + x2 + 3791) >> 4);

	return p;
}