#include "Frame.h"

uint8_t frame_generate(void *data, struct frame *fr, uint8_t data_len,
	uint8_t type)
{
	uint8_t crc = 0xff, i;

	if (data_len > FRAME_PAYLOAD_LEN)
		return FRAME_ERR_LEN;

	if ((type < FRAME_START) || (type > FRAME_SIMPLE))
		return FRAME_ERR_TYPE;

	fr->header = type << FRAME_TYPE_OFFS;
	fr->header |= data_len;

	memcpy(fr->payload, data, data_len);

	crc = frame_crc_calc(crc, fr->header);
	for (i = 0; i < data_len; i++)
	{
		crc = frame_crc_calc(crc, ((uint8_t *) data)[i]);
	}

	fr->checksum = crc;

	return FRAME_OK;
}

uint8_t frame_decode(struct frame *fr, void *data, uint8_t *data_len,
	uint8_t *type)
{
	uint8_t data_crc = 0xff, i;

	*data_len = FRAME_LEN(fr);
	*type = FRAME_TYPE(fr);

	data_crc = frame_crc_calc(data_crc, fr->header);
	for (i = 0; i < *data_len; i++) {
		data_crc = frame_crc_calc(data_crc, fr->payload[i]);
	}

	if (data_crc == fr->checksum) {
		memcpy(data, fr->payload, data_len);
	} else {
		return FRAME_ERR_CRC;
	}

	return FRAME_OK;
}

uint8_t frame_crc_calc(uint8_t crc, uint8_t data)
{
	return _crc8_ccitt_update(crc, data);
}
