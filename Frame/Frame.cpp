#include "Frame.h"

int8_t frame_generate(void *data, struct frame *fr, uint8_t data_len,
	uint8_t type)
{
	uint8_t crc = 0xff, i;

	if (data_len > FRAME_PAYLOAD_LEN)
		return -FRAME_ERR_LEN;

	if ((type < FRAME_START) || (type > FRAME_SIMPLE))
		return -FRAME_ERR_TYPE;

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

int8_t frame_decode(struct frame *fr, void *data, uint8_t *data_len,
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
		memcpy(data, fr->payload, *data_len);
	} else {
		return -FRAME_ERR_CRC;
	}

	return FRAME_OK;
}

uint8_t frame_crc_calc(uint8_t crc, uint8_t data)
{
	return _crc8_ccitt_update(crc, data);
}

int8_t frame_data_to_frames(void *data, uint16_t data_len,
	uint8_t *frames_buf, uint16_t frames_buf_len)
{
	uint8_t i, frame_amount;
	uint8_t *data_ptr = (uint8_t *) data;

	if (data_len <= FRAME_PAYLOAD_LEN) {
		frame_generate(data_ptr, (struct frame *) frames_buf, data_len, FRAME_SIMPLE);
		return 1;
	} else if (data_len <= FRAME_PAYLOAD_LEN * 2) {
		if (data_len > frames_buf_len + FRAME_SERVICE_LEN * 2)
			return -FRAME_ERR_LEN;
		frame_generate(data_ptr, (struct frame *) frames_buf, FRAME_PAYLOAD_LEN, FRAME_START);
		data_ptr += FRAME_PAYLOAD_LEN;
		data_len -= FRAME_PAYLOAD_LEN;
		frames_buf += FRAME_PAYLOAD_LEN;
		frame_generate(data_ptr, (struct frame *) frames_buf, data_len, FRAME_END);
		return 2;
	} else {
		frame_amount = 2 + (data_len - FRAME_PAYLOAD_LEN * 2) / FRAME_PAYLOAD_LEN + 1; 
		if (data_len > frames_buf_len + FRAME_SERVICE_LEN * frame_amount)
			return -FRAME_ERR_LEN;
		frame_generate(data_ptr, (struct frame *) frames_buf, FRAME_PAYLOAD_LEN, FRAME_START);
		data_ptr += FRAME_PAYLOAD_LEN;
		data_len -= FRAME_PAYLOAD_LEN;
		frames_buf += FRAME_PAYLOAD_LEN;
		for (i = 0; i < frame_amount - 2; i++) {
			frame_generate(data_ptr, (struct frame *) frames_buf, FRAME_PAYLOAD_LEN, FRAME_REGULAR);
			data_ptr += FRAME_PAYLOAD_LEN;
			data_len -= FRAME_PAYLOAD_LEN;
			frames_buf += FRAME_PAYLOAD_LEN;
		}
		frame_generate(data_ptr, (struct frame *) frames_buf, data_len, FRAME_END);
		return frame_amount;
	}
}

int8_t frame_get_stream(struct frames_buffer *f_buf, void *dest_buf, uint16_t len, uint8_t *data_len)
{
	struct frame *frame_rec = &f_buf->frame_record[f_buf->head];
	uint8_t tmp_type;
	int8_t ret, f_ret = FRAME_OK;

	if (f_buf->head == f_buf->tail) {
		return FRAME_BUF_EMPTY; // Buffer is EMPTY
	}

	if (len < FRAME_LEN(frame_rec))
		return -FRAME_ERR_NOMEM;
	
	if (FRAME_TYPE(frame_rec) == FRAME_START) {
		if (f_buf->start_found == 1) {
			f_buf->start_found = 0;
			return -FRAME_BRK_STREAM;
		} else {
			f_buf->start_found = 1;
		}
	} else if (FRAME_TYPE(frame_rec) == FRAME_END) {
		f_buf->start_found = 0;
		f_ret = FRAME_OK_END;
	}

	ret = frame_decode(&f_buf->frame_record[f_buf->head], dest_buf,
		data_len, &tmp_type);

	if (ret) {
		f_ret = ret;
	}
	FRAME_BUF_PTR_INC(f_buf->head);


	return f_ret;
}