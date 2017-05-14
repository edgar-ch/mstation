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
	struct frames_buffer *f_buf, uint8_t frames_buf_len)
{
	uint8_t i, frame_amount;
	uint8_t *data_ptr = (uint8_t *) data;
	/* return error if data length bigger then frame buffer size */
	if (data_len > frames_buf_len * FRAME_PAYLOAD_LEN)
		return -FRAME_ERR_LEN;

	frame_amount = data_len / FRAME_PAYLOAD_LEN;
	frame_amount += (data_len % FRAME_PAYLOAD_LEN > 0) ? 1 : 0;

	if (frame_amount == 1) {
		frame_generate(data_ptr, &f_buf->frame_record[0], data_len, FRAME_SIMPLE);
	} else if (frame_amount == 2) {
		frame_generate(data_ptr, &f_buf->frame_record[0], FRAME_PAYLOAD_LEN, FRAME_START);
		data_ptr += FRAME_PAYLOAD_LEN;
		data_len -= FRAME_PAYLOAD_LEN;
		frame_generate(data_ptr, &f_buf->frame_record[1], data_len, FRAME_END);
	} else {
		frame_generate(data_ptr, &f_buf->frame_record[0], FRAME_PAYLOAD_LEN, FRAME_START);
		data_ptr += FRAME_PAYLOAD_LEN;
		data_len -= FRAME_PAYLOAD_LEN;
		for (i = 1; i < frame_amount - 2; i++) {
			frame_generate(data_ptr, &f_buf->frame_record[i], FRAME_PAYLOAD_LEN, FRAME_REGULAR);
			data_ptr += FRAME_PAYLOAD_LEN;
			data_len -= FRAME_PAYLOAD_LEN;
		}
		frame_generate(data_ptr, &f_buf->frame_record[i], data_len, FRAME_END);
	}
	/* save head of buffer and set tail to begining of buffer */
	f_buf->tail = 0;
	f_buf->head = frame_amount;
	f_buf->cnt = frame_amount;

	return frame_amount;
}

int8_t frame_get_stream(struct frames_buffer *f_buf, void *dest_buf, uint16_t len, uint8_t *data_len)
{
	struct frame *frame_rec = &f_buf->frame_record[f_buf->tail];
	uint8_t fr_type;
	int8_t ret, f_ret = FRAME_OK;

	if (f_buf->cnt == 0 && f_buf->tail == f_buf->head) {
		return FRAME_BUF_EMPTY; // Buffer is EMPTY
	}

	if (len < FRAME_LEN(frame_rec))
		return -FRAME_ERR_NOMEM;
	
	ret = frame_decode(frame_rec, dest_buf,
		data_len, &fr_type);

	if (ret) {
		f_ret = ret;
	} else {
		if (fr_type == FRAME_START) {
			if (f_buf->start_found == 1) {
				f_buf->start_found = 0;
				return -FRAME_BRK_STREAM;
			} else {
				f_buf->start_found = 1;
			}
		} else if (fr_type == FRAME_END) {
			f_buf->start_found = 0;
			f_ret = FRAME_OK_END;
		} else if (fr_type == FRAME_SIMPLE) {
			if (f_buf->start_found == 1) {
				f_buf->start_found = 0;
				return -FRAME_BRK_STREAM;
			}
			f_ret = FRAME_OK_SIMPLE;
		}
	}

	FRAME_BUF_PTR_INC(f_buf->tail);
	f_buf->cnt--;

	return f_ret;
}