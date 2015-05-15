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
};

struct BMP180_REGS
{
	uint8_t xlsb;
	uint8_t lsb;
	uint8_t msb;
	uint8_t ctrl;
	uint8_t id;
};

/* Headers for functions */
uint16_t bmp180_read16(uint8_t);
uint8_t bmp180_read8(uint8_t);
void bmp180_read_calibration();
void bmp180_init();
int32_t bmp180_read_raw_temp();
int32_t bmp180_read_raw_press(uint8_t);
int32_t bmp180_calc_b5(int32_t);
int32_t bmp180_get_temp();
int32_t bmp180_get_pressure(uint8_t);