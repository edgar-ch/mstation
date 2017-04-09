#include <Frame.h>
#include <MData.h>
#include <avr/sleep.h>
#include <Time.h>

#define EXEC_TIME(func, msg) do { delay(10); cli(); st_time = micros(); func; \
		end_time = micros(); sei(); print_time(st_time, end_time, msg); } while(0)

#define BUF_SIZE 127

uint8_t test_data[FRAME_PAYLOAD_LEN];
uint8_t out_data[FRAME_PAYLOAD_LEN];
struct frame t_frame;
struct datetime meas_time;
struct mdata_packet test_packet;
struct frames_buffer t_fbuf;
struct frames_buffer meas_t_fbuf;
uint8_t t_packet_buf[sizeof(struct mdata_packet)];
uint8_t *t_packet_buf_ptr;
uint8_t t_packet_data_len = 0;
uint8_t t_packet_data_free = sizeof(struct mdata_packet);

float test_temp = 23.56;
float test_temp2 = 12.90;
float test_temp3 = -23.56;
float test_temp4 = 21.45;
uint32_t test_pressure = 99458;
uint8_t test_humidity = 78;

int tests_run = 0;
int tests_failed = 0;

void print_raw_data(void *, uint16_t);
void check_test(int8_t, const char *, const char*);
void print_time(unsigned long, unsigned long, const char *);
struct datetime conv_time(time_t);

void setup()
{
	Serial.begin(115200);
	int i;
	for (i = 0; i < FRAME_PAYLOAD_LEN; i++)
	{
		test_data[i] = i+2;
	}
}

void loop()
{
	int8_t res, ret, frame_cnt;
	uint8_t data_len, data_type;
	unsigned long st_time, end_time;
	uint8_t i;
	meas_time = conv_time(1482836626UL);

	/* Test packet generation */
	mdata_init(&test_packet);
	res = mdata_add_datetime(&meas_time, MDATA_DATETIME);
	check_test(res, "MDATA_ADD_DATETIME", "");
	res = mdata_add_temp(&test_temp, MDATA_TEMP);
	check_test(res, "MDATA_ADD_TEMP", "");
	res = mdata_add_temp(&test_temp2, MDATA_TEMP2);
	check_test(res, "MDATA_ADD_TEMP2", "");
	res = mdata_add_temp(&test_temp3, MDATA_TEMP3);
	check_test(res, "MDATA_ADD_TEMP3", "");
	res = mdata_add_temp(&test_temp4, MDATA_TEMP4);
	check_test(res, "MDATA_ADD_TEMP4", "");
	res = mdata_add_humidity(&test_humidity, MDATA_HUMIDITY);
	check_test(res, "MDATA_ADD_HUMID", "");
	res = mdata_add_pressure(&test_pressure, MDATA_PRESSURE);
	check_test(res, "MDATA_ADD_PRESS", "");
	mdata_fin_packet(&test_packet);
	check_test(test_packet.header.length, "PACKET_DATA_LENGHT", "");
	check_test(mdata_packet_len(&test_packet), "PACKET_LENGHT", "");
	check_test(test_packet.header.checksum, "PACKET_CRC", "");
	/* Check packet */
	EXEC_TIME(res = mdata_check_packet(&test_packet), "CHECK_PACKET");
	check_test(res, "CHECK_PACKET", "");
	/* Print packet data in raw format */
	Serial.print("PACKET_RAW:\n");
	print_raw_data(&test_packet, mdata_packet_len(&test_packet));
	Serial.println();
	
	/* Test frame generaton from packet */
	frame_cnt = frame_data_to_frames(&test_packet, mdata_packet_len(&test_packet),
		&meas_t_fbuf, FRAMES_BUF_LEN);
	check_test(frame_cnt, "PACKET_TO_FRAMES", "");

	/* Test checking of frame buffer on base */
	res = frame_get_stream(&t_fbuf, t_packet_buf, t_packet_data_free, &data_len);
	check_test(res, "CHECK_FBUF", "");
	EXEC_TIME(frame_get_stream(&t_fbuf, t_packet_buf, t_packet_data_free, &data_len), "CHECK_FBUF");
	Serial.println();

	/* Test frame buffer on base */
	t_fbuf.tail = 0;
	t_fbuf.head = 0;
	// Copy measure module data to base station buffer and measure copy time
	for (i = 0; i < frame_cnt; i++) {
		st_time = micros();
		memcpy(&t_fbuf.frame_record[t_fbuf.tail],
			&meas_t_fbuf.frame_record[i], sizeof(struct frame));
		FRAME_BUF_PTR_INC(t_fbuf.tail);
		end_time = micros();
		print_time(st_time, end_time, "FRAME_COPY");
		check_test(t_fbuf.tail, "FBUF_TAIL", "");
	}

	/* Check frame buffer for incoming packets */
	t_packet_buf_ptr = t_packet_buf;
	do {
		check_test(t_fbuf.head, "FBUF_HEAD", "");
		EXEC_TIME(
			(res = frame_get_stream(
				&t_fbuf, t_packet_buf_ptr,
				t_packet_data_free, &data_len)
			),
			"GET_FBUF");
		check_test(res, "GET_FBUF", "");
		Serial.print("Take DATA size = ");
		Serial.println(data_len);
		t_packet_buf_ptr += data_len;
		t_packet_data_free -= data_len;
	} while (res != FRAME_BUF_EMPTY);

	Serial.println();

	/* Check packet */
	EXEC_TIME(res = mdata_check_packet((struct mdata_packet *) t_packet_buf), "CHECK_PACKET");
	check_test(res, "CHECK_PACKET", "");
	/* Print packet data in raw format */
	Serial.print("PACKET_RAW:\n");
	print_raw_data(&test_packet, mdata_packet_len(&test_packet));
	Serial.println();

	/* Testing frame generation */
	res = frame_generate(test_data, &t_frame, sizeof(test_data), FRAME_START);
	check_test(res, "FRAME_GEN", "");
	/* Testing frame decoding */
	res = frame_decode(&t_frame, out_data, &data_len, &data_type);
	check_test(res, "FRAME_DEC", "");
	EXEC_TIME(frame_decode(&t_frame, out_data, &data_len, &data_type), "FRAME_DEC");
	Serial.print("Frame type: ");
	Serial.println(t_frame.header);
	/* Testing data */
	st_time = micros();
	res = memcmp(test_data, out_data, FRAME_PAYLOAD_LEN);
	end_time = micros();
	print_time(st_time, end_time, "FRAME_CHECK_DATA");
	check_test(res, "FRAME_CHECK_DATA", "");
	/* Stop testing */
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	delay(10);
	sleep_mode();
}

void print_raw_data(void *data, uint16_t len)
{
	uint16_t i;

	for (i = 0; i < len; i++) {
		Serial.print(*((uint8_t *) data + i), HEX);
		Serial.print(' ');
	}
	Serial.print('\n');
}

void check_test(int8_t res, const char *test_name, const char *err_msg)
{
	if (res >= 0) {
		Serial.print("Test ");
		Serial.print(test_name);
		Serial.print(" PASSED with res = ");
		Serial.println(res);
		tests_run++;
	} else {
		Serial.print("Test ");
		Serial.print(test_name);
		Serial.print(" FAILED with res = ");
		Serial.print(res);
		Serial.print(": ");
		Serial.println(err_msg);
		tests_failed++;
	}
}

void print_time(unsigned long b_time, unsigned long e_time, const char *msg)
{
	Serial.print(msg);
	Serial.print(" elapsed ");
	Serial.print(e_time - b_time);
	Serial.println(" usec.");
}

struct datetime conv_time(time_t curr)
{
	struct datetime curr_datetime;

	curr_datetime.seconds = second(curr);
	curr_datetime.minutes = minute(curr);
	curr_datetime.hours = hour(curr);
	curr_datetime.day = weekday(curr);
	curr_datetime.date = day(curr);
	curr_datetime.month = month(curr);
	curr_datetime.year = year(curr) - 2000;

	return curr_datetime;
}
