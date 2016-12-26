#include <util/crc16.h>
#include <string.h>

#define FRAME_PAYLOAD_LEN 30
#define FRAME_TYPE_OFFS 5
#define FRAME_TYPE_MASK 0xE0
#define FRAME_LEN_MASK 0x1F

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
	FRAME_ERR_TYPE, // wrong frame type
	FRAME_ERR_CRC
};

struct frame {
	uint8_t header;
	uint8_t payload[FRAME_PAYLOAD_LEN];
	uint8_t checksum;
};

#define FRAME_TYPE(A) (((A)->header & FRAME_TYPE_MASK) >> FRAME_TYPE_OFFS)
#define FRAME_LEN(A) ((A)->header & FRAME_LEN_MASK)

uint8_t frame_generate(void *, struct frame *, uint8_t, uint8_t);
uint8_t frame_decode(struct frame *, void *, uint8_t *, uint8_t *);
uint8_t frame_crc_calc(uint8_t, uint8_t);
