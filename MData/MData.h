#include <string.h>
#include <stdint.h>
#include <MStation.h>

#define MDATA_MAX_DATA_LENGTH 127
#define MDATA_VERSION 0x01
#define MDATA_PACKET_BUF_LEN 6

enum MDATA_PACKET_TYPE {
	MDATA_MEAS = 'M'
};

enum MDATA_TYPE {
	MDATA_TEMP = 0x10,
	MDATA_TEMP2 = 0x11,
	MDATA_TEMP3 = 0x12,
	MDATA_TEMP4 = 0x13,
	MDATA_SOIL_TEMP = 0x20,
	MDATA_SOIL_TEMP_10 = 0x21,
	MDATA_SOIL_TEMP_20 = 0x22,
	MDATA_SOIL_TEMP_50 = 0x23,
	MDATA_HUMIDITY = 0x30,
	MDATA_PRESSURE = 0x40,
	MDATA_WIND_DIR = 0x50,
	MDATA_WIND_SPEED = 0x51,
	MDATA_SOLAR_RADIATION = 0x60,
	MDATA_SOLAR_LEN = 0x70,
	MDATA_PRECIPITATION = 0x80,
	MDATA_DATETIME = 0x90,
	MDATA_LUX = 0xA0
};

struct __attribute__((packed)) mdata_header {
	uint8_t type;
	uint8_t version;
	uint8_t length;
	uint16_t checksum;
};

struct __attribute__((packed)) mdata_packet {
	struct mdata_header header;
	uint8_t data[MDATA_MAX_DATA_LENGTH];
};

struct __attribute__((packed)) mdata_packet_buf {
	struct mdata_packet mdata_rec[MDATA_PACKET_BUF_LEN];
	uint8_t tail = 0;
	uint8_t head = 0;
};

struct __attribute__((packed)) temp_rec {
	uint8_t type;
	float temp;
};

struct __attribute__((packed)) humid_rec {
	uint8_t type;
	uint8_t humid;
};

struct __attribute__((packed)) press_rec {
	uint8_t type;
	uint32_t press;
};

struct __attribute__((packed)) datetime_rec {
	uint8_t type;
	struct datetime dt;
};

struct __attribute__((packed)) solar_rad_rec {
	uint8_t type;
	uint32_t solar_rad;
};

struct __attribute__((packed)) lux_rec {
	uint8_t type;
	float lux;
};

void mdata_init(struct mdata_packet *);
int8_t mdata_add_temp(float *, uint8_t);
int8_t mdata_add_humidity(uint8_t *, uint8_t);
int8_t mdata_add_pressure(uint32_t *, uint8_t);
int8_t mdata_add_solar_rad(uint32_t *, uint8_t);
int8_t mdata_add_lux(float *, uint8_t);
int8_t mdata_fin_packet(struct mdata_packet *);
uint16_t mdata_packet_len(struct mdata_packet *);
uint16_t mdata_crc_update(uint16_t, uint8_t);
int8_t mdata_check_packet(struct mdata_packet *);
int8_t mdata_add_datetime(struct datetime *, uint8_t);
