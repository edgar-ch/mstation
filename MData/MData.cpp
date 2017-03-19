#include "MData.h"
#include <util/crc16.h>

static uint8_t *mdata_data_ptr;
static uint8_t *mdata_start_ptr;

void mdata_init(struct mdata_packet *mdata)
{
	mdata->header.type = MDATA_MEAS;
	mdata->header.version = MDATA_VERSION;
	mdata->header.length = 0;
	mdata_data_ptr = mdata->data;
	mdata_start_ptr = mdata_data_ptr;
}

int8_t mdata_add_temp(float *temp, uint8_t temp_type)
{
	struct temp_rec temp_data;

	if ((temp_type & 0xF0) != MDATA_TEMP) {
		return -1;
	}
	if ((mdata_data_ptr + sizeof(struct temp_rec)) - mdata_start_ptr \
		 > MDATA_MAX_DATA_LENGTH) {
		return -2;
	}

	temp_data.type = temp_type;
	temp_data.temp = *temp;

	memcpy(mdata_data_ptr, &temp_data, sizeof(struct temp_rec));
	mdata_data_ptr += sizeof(struct temp_rec);

	return 0;
}

int8_t mdata_add_humidity(uint8_t *humid, uint8_t humid_type)
{
	struct humid_rec humid_data;

	if ((humid_type & 0xF0) != MDATA_HUMIDITY) {
		return -1;
	}
	if ((mdata_data_ptr + sizeof(struct humid_rec)) - mdata_start_ptr \
		 > MDATA_MAX_DATA_LENGTH) {
		return -2;
	}

	humid_data.type = humid_type;
	humid_data.humid = *humid;

	memcpy(mdata_data_ptr, &humid_data, sizeof(struct humid_rec));
	mdata_data_ptr += sizeof(struct humid_rec);

	return 0;
}

int8_t mdata_add_pressure(uint32_t *pressure, uint8_t pressure_type)
{
	struct press_rec press_data;

	if ((pressure_type & 0xF0) != MDATA_PRESSURE) {
		return -1;
	}
	if ((mdata_data_ptr + sizeof(struct press_rec)) - mdata_start_ptr \
		 > MDATA_MAX_DATA_LENGTH) {
		return -2;
	}

	press_data.type = pressure_type;
	press_data.press = *pressure;

	memcpy(mdata_data_ptr, &press_data, sizeof(struct press_rec));
	mdata_data_ptr += sizeof(struct press_rec);

	return 0;
}

int8_t mdata_add_datetime(struct datetime *dt, uint8_t datetime_type)
{
	struct datetime_rec dt_rec;

	if ((datetime_type & 0xF0) != MDATA_DATETIME) {
		return -1;
	}
	if ((mdata_data_ptr + sizeof(struct datetime_rec)) - mdata_start_ptr \
		 > MDATA_MAX_DATA_LENGTH) {
		return -2;
	}

	dt_rec.type = datetime_type;
	memcpy(&dt_rec.dt, dt, sizeof(struct datetime));

	memcpy(mdata_data_ptr, &dt_rec, sizeof(struct datetime_rec));
	mdata_data_ptr += sizeof(struct datetime_rec);

	return 0;
}

int8_t mdata_fin_packet(struct mdata_packet *mdata)
{
	uint8_t data_size = mdata_data_ptr - mdata->data;
	uint8_t i;
	uint16_t crc = 0xffff; // init for CCITT CRC16
	
	// calc data checksum
	for (i = 0; i < data_size; ++i) {
		crc = mdata_crc_update(crc, mdata->data[i]);
	}
	mdata->header.checksum = crc;
	mdata->header.length = data_size;

	return 0;
}

uint16_t mdata_packet_len(struct mdata_packet *mdata)
{
	return mdata->header.length + sizeof(struct mdata_header);
}

uint16_t mdata_crc_update(uint16_t crc, uint8_t data)
{
	return _crc_ccitt_update(crc, data);
}

int8_t mdata_check_packet(struct mdata_packet *mdata)
{
	uint8_t data_size = mdata->header.length;
	uint8_t i;
	uint16_t crc = 0xffff; // init for CCITT CRC16

	if (mdata->header.type != MDATA_MEAS)
		return -1;
	
	// calc data checksum
	for (i = 0; i < data_size; ++i) {
		crc = mdata_crc_update(crc, mdata->data[i]);
	}

	if (mdata->header.checksum == crc)
		return 0;
	else
		return -2;
}