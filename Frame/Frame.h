#include <util/crc16.h>
#include <string.h>

#define FRAME_PAYLOAD_LEN 30
#define FRAME_FULL_LEN 32
#define FRAME_SERVICE_LEN 2
#define FRAME_TYPE_OFFS 5
#define FRAME_TYPE_MASK 0xE0
#define FRAME_LEN_MASK 0x1F

#ifndef FRAMES_BUF_LEN
#define FRAMES_BUF_LEN 6
#endif

enum FRAME_TYPE
{
	FRAME_START = 0x4,
	FRAME_REGULAR = 0x5,
	FRAME_END = 0x6,
	FRAME_SIMPLE = 0x7
};

enum FRAME_ERR
{
	FRAME_OK = 0,
	FRAME_ERR_LEN = 1, // data length too long
	FRAME_ERR_TYPE = 2, // wrong frame type
	FRAME_ERR_CRC = 3,
	FRAME_BRK_STREAM = 4,
	FRAME_ERR_NOMEM = 5,
	FRAME_OK_END = 6,
	FRAME_BUF_EMPTY = 7
};

struct __attribute__((packed)) frame {
	uint8_t header;
	uint8_t payload[FRAME_PAYLOAD_LEN];
	uint8_t checksum;
};

struct __attribute__((packed)) frames_buffer
{
	struct frame frame_record[FRAMES_BUF_LEN];
	uint8_t tail = 0;
	uint8_t head = 0;
	uint8_t start_found = 0;
};

#define FRAME_TYPE(A) (((A)->header & FRAME_TYPE_MASK) >> FRAME_TYPE_OFFS)
#define FRAME_LEN(A) ((A)->header & FRAME_LEN_MASK)
#define FRAME_BUF_PTR_INC(A) (A = (A + 1) % FRAMES_BUF_LEN)

int8_t frame_generate(void *, struct frame *, uint8_t, uint8_t);
int8_t frame_decode(struct frame *, void *, uint8_t *, uint8_t *);
int8_t frame_data_to_frames(void *, uint16_t, uint8_t *, uint16_t);
uint8_t frame_crc_calc(uint8_t, uint8_t);
int8_t frame_get_stream(struct frames_buffer *, void *, uint16_t, uint8_t *);
