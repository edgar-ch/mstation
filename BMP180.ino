#include <Wire.h>
// BMP180 read/write i2c bus addres
#define BMP180_READ_ADDR 0xF7
#define BMP180_WRITE_ADDR 0x77
// BMP180 calibration registers addresses
#define BMP180_REG_ADDR_AC1 0xAA
#define BMP180_REG_ADDR_AC2 0xAC
#define BMP180_REG_ADDR_AC3 0xAE
#define BMP180_REG_ADDR_AC4 0xB0
#define BMP180_REG_ADDR_AC5 0xB2
#define BMP180_REG_ADDR_AC6 0xB4
#define BMP180_REG_ADDR_B1 0xB6
#define BMP180_REG_ADDR_B2 0xB8
#define BMP180_REG_ADDR_MB 0xBA
#define BMP180_REG_ADDR_MC 0xBC
#define BMP180_REG_ADDR_MD 0xBE
// BMP180 ADC and control registers addresses
#define BMP180_REG_ADDR_XLSB 0xF8
#define BMP180_REG_ADDR_LSB 0xF7
#define BMP180_REG_ADDR_MSB 0xF6
#define BMP180_REG_ADDR_CTRL_MEAS 0xF4
#define BMP180_REG_ADDR_RES 0xE0
#define BMP180_REG_ADDR_ID 0xD0
// BMP180 control commands
#define BMP180_CMD_TEMP 0x2E
#define BMP180_CMD_PRESS 0x34

// BH1750 i2c bus address
#define BH1750_ADDR 0x23
// BH1750 commands
#define BH1750_PWR_DOWN 0x00
#define BH1750_PWR_ON 0x01
#define BH1750_RESET 0x07
#define BH1750_CONT_HR 0x10
#define BH1750_CONT_HR2 0x11
#define BH1750_CINT_LR 0x13
#define BH1750_1TIME_HR 0x20
#define BH1750_1TIME_HR2 0x21
#define BH1750_1TIME_LR 0x23
// BH1750 resolution mods
enum BH1750_MOD{LR = 0, HR, HR2};

#define DEBUG 1

struct BMP180_COEFF
{
	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;
} bmp180_coeff;

struct BMP180_REGS
{
	uint8_t xlsb;
	uint8_t lsb;
	uint8_t msb;
	uint8_t ctrl;
	uint8_t id;
} bmp180_regs;

void setup()
{
	Serial.begin(115200);
	Wire.begin();
	bmp180_init();
	bmp180_read_calibration();
	Serial.println(F("BMP180 inited"));
}

void loop()
{
	uint8_t oss = 0;
	long int pressure;
	long int temp;
	uint16_t lux;
	float lux_h;

	pressure = bmp180_get_pressure(oss);
	Serial.print(F("Pressure: "));
	Serial.print(pressure);
	Serial.println(F(" Pa"));
	delay(1000);

	temp = bmp180_get_temp();
	Serial.print(F("Temp: "));
	Serial.println(temp);
	delay(1000);

	lux = bh1750_meas_Lmode();
	Serial.print(F("Ambient light LR: "));
	Serial.println(lux);
	delay(5000);

	lux = bh1750_meas_Hmode();
	Serial.print(F("Ambient light HR: "));
	Serial.println(lux);
	delay(5000);

	lux_h = bh1750_meas_H2mode();
	Serial.print(F("Ambient light HR2: "));
	Serial.println(lux_h);
	delay(5000);
}
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
	Wire.endTransmission();
	
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
	Wire.endTransmission();

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
		Serial.println(F("Not BMP180 device ID !!!"));
		Serial.println(bmp180_regs.id);
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
	Wire.endTransmission();

	return tmp;
}

uint16_t bh1750_meas_Lmode()
{
	uint16_t val, tmp;

	bh1750_write_cmd(BH1750_PWR_ON);
	bh1750_write_cmd(BH1750_1TIME_LR);
	delay(24);
	tmp = bh1750_read_data();
	#if DEBUG
	Serial.print(F("Raw value: "));
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
	#if DEBUG
	Serial.print(F("Raw value: "));
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
	#if DEBUG
	Serial.print(F("Raw value: "));
	Serial.println(tmp);
	#endif
	fl = tmp & 0x0001;
	#if DEBUG
	Serial.println(fl);
	#endif
	tmp >>= 1;
	#if DEBUG
	Serial.println(tmp);
	#endif
	val = (float) tmp / 1.2 + fl * 0.5;
	return val;
}