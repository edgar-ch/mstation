#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Wire.h>
#ifdef DEBUG
#include <printf.h>
#endif
#include <Time.h> /* Not really uses, workaround for Stino */
#include <MStation.h>
#include <SdFat.h>
#include <stddef.h>
#include "BMP180.h"
#include "BH1750FVI.h"
#include "DS3231.h"

// nRF24L01 CE, CS and INT pins
enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_RADIO_EVENTS
{
	RX_MSG = 0,
	TX_SUCCESS,
	TX_FAIL,
	NOW_SENDING,
	LISTEN
};
// states of measurement and data
enum STATES
{
	INIT = 0, // initial state
	MEAS_TIME,
	RECORD_DATA,
	SEND_LATEST_DATA,
	SEND_DATA,
	PARSE_RX_DATA,
	RADIO_EVENT,
	WAIT_FOR_SEND,
	PREPARE_SLEEP
};

SdFat SD;
// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// default addresses for base station and module
uint8_t radio_addr[][6] = {"BASEE", "MEAS1"};
// current radio state
volatile nRF24_RADIO_EVENTS radio_state = LISTEN;
// buffer for received messages
uint8_t r_buffer[32];
// buffer for transfer messages
uint8_t t_buffer[32];
// current datetime
struct datetime curr_datetime;
// datetime is set ?
uint8_t is_time_set = 0;
// timeout of time request (in millis)
unsigned int time_req_timeout = 30000;
// file for measured data
File meas_file;
// last position in file, which was sended
uint32_t not_sended_pos = 0;
// position for writing
uint32_t curr_pos = 0;
// postion before write
uint32_t prev_pos = 0;
// current state
STATES currState = INIT;

void setup()
{
	#ifdef DEBUG
	Serial.begin(115200);
	printf_begin();
	#endif
	// init i2c bus
	Wire.begin();
	// init BMP180 pressure sensor and read calibration table
	bmp180_init();
	bmp180_read_calibration();
	// init radio
	radio.begin();
	radio.setChannel(125);
	radio.setPALevel(RF24_PA_MAX);
	radio.setRetries(4, 15);
	radio.setAutoAck(true);
	radio.enableDynamicPayloads();
	radio.openWritingPipe(radio_addr[0]);
	radio.openReadingPipe(1, radio_addr[1]);
	#ifdef DEBUG
	radio.printDetails();
	#endif
	radio.startListening();
	/* check for first run of DS3231 and try
	 * to request datetime from base station
	 * */
	if (ds3231_is_first_run())
	{
		#ifdef DEBUG
		Serial.println(F("Trying to request datetime"));
		#endif
		if (try_request_datetime())
		{
			#ifdef DEBUG
			Serial.println(F("Get datetime"));
			print_datetime_serial(curr_datetime);
			#endif
			ds3231_set_datetime(curr_datetime);
		}
	}
	// read datetime
	ds3231_init();
	curr_datetime = ds3231_get_datetime();
	// init SD card
	SD.begin(4, 2);
	meas_file = SD.open("meas.dat", FILE_WRITE);
	curr_pos = meas_file.position();
	not_sended_pos = curr_pos;
	#ifdef DEBUG
	print_datetime_serial(curr_datetime);
	Serial.println(F("BMP180 inited"));
	#endif
	attachInterrupt(0, radio_event, LOW);
	currState = MEAS_TIME;
}

void loop()
{
	uint8_t oss = 0;
	uint16_t lux;
	struct measure_data measured;
	struct file_entry data_entry;

	// if now time for measurement
	if (currState == MEAS_TIME)
	{
		measured.pressure = bmp180_get_pressure(oss);
		measured.temperature = bmp180_get_temp();
		measured.lux = bh1750_meas_H2mode();
		measured.mtime = ds3231_get_datetime();
		#ifdef DEBUG
		Serial.print(F("Pressure: "));
		Serial.print(measured.pressure);
		Serial.println(F(" Pa"));
		Serial.print(F("Temp: "));
		Serial.println(measured.temperature);
		Serial.print(F("Ambient light HR2: "));
		Serial.println(measured.lux);
		#endif
		currState = RECORD_DATA;
	}
	// write data to SD
	if (currState == RECORD_DATA)
	{
		#ifdef DEBUG
		Serial.println(F("Write measured to SD"));
		#endif
		prev_pos = curr_pos;
		data_entry.m_data = measured;
		data_entry.is_sended = 0;
		meas_file.seek(curr_pos);
		meas_file.write((uint8_t *) &measured, sizeof(struct file_entry));
		meas_file.flush();
		curr_pos = meas_file.position();
		currState = SEND_LATEST_DATA;
	}

	if (currState == SEND_LATEST_DATA)
	{
		#ifdef DEBUG
		Serial.println(F("Send data to base station"));
		#endif
		// prepare measurement data
		t_buffer[0] = 'M';
		memcpy(t_buffer + 1, &measured, sizeof(struct measure_data));
		// and send it to radio
		radio.stopListening();
		radio_state = NOW_SENDING;
		currState = WAIT_FOR_SEND;
		radio.startWrite(t_buffer, sizeof(t_buffer), 0);
	}

	if (currState == WAIT_FOR_SEND)
	{
		switch (radio_state) {
			case TX_SUCCESS:
				// move file pointer and set flag
				set_sended(not_sended_pos);
				not_sended_pos += sizeof(struct file_entry);
				// if we have not sended data
				if (not_sended_pos != curr_pos)
				{
					currState = SEND_DATA;
				}
				else
				{
					not_sended_pos = curr_pos;
					radio_state = LISTEN;
					radio.startListening();
				}
				#ifdef DEBUG
				Serial.println(F("Sending data success."));
				#endif
				break;
			case TX_FAIL:
				currState = PREPARE_SLEEP;
				radio_state = LISTEN;
				radio.startListening();
				#ifdef DEBUG
				Serial.println(F("Sending data failed"));
				#endif
			default:
				break;
		}
	}

	if (currState == SEND_DATA)
	{
		#ifdef DEBUG
		Serial.println(F("Sending previous data"));
		#endif
		data_entry = read_entry(not_sended_pos);
		// prepare measurement data
		t_buffer[0] = 'M';
		memcpy(t_buffer + 1, &(data_entry.m_data), sizeof(struct measure_data));
		// and send it to radio
		radio_state = NOW_SENDING;
		currState = WAIT_FOR_SEND;
		radio.startWrite(t_buffer, sizeof(t_buffer), 0);
	}

	if (currState == PREPARE_SLEEP)
	{
		#ifdef DEBUG
		Serial.println(F("Fake sleep"));
		#endif
		// doing fake sleep
		delay(30000);
		currState == MEAS_TIME;
	}
}

void radio_event()
{
	bool tx, fail, rx;
	//uint8_t rxBytes;

	radio.whatHappened(tx, fail, rx);

	if (tx)
	{
		radio_state = TX_SUCCESS;
	}
	else if (rx)
	{
		radio_state = RX_MSG;
		/*
		rxBytes = radio.getDynamicPayloadSize();
		if (rxBytes < 1)
		{
			#ifdef DEBUG
			Serial.println(F("Get corrupted data"));
			#endif
		}
		else
		{
			radio.read(r_buffer, 32);
			#ifdef DEBUG
			Serial.println(F("Get some data"));
			Serial.write(r_buffer, rxBytes);
			#endif
		}
		*/
	}
	else
	{
		radio_state = TX_FAIL;
	}
}

/**
 * @brief [request time]
 * @details [trying request time from base station]
 * 
 * @param long [description]
 * @return [1 if success, 0 if fail]
 */
uint8_t try_request_datetime()
{
	unsigned long int curr_millis = millis();
	radio.stopListening();
	#ifdef DEBUG
	Serial.println(F("Sending 'T' command"));
	#endif
	radio.write("T", 2);  // second byte for \0 character
	radio.startListening();
	while ((millis() - curr_millis) < time_req_timeout)
	{
		if (radio.available())
		{
			#ifdef DEBUG
			Serial.println(F("Get something"));
			#endif
			radio.read(r_buffer, 32);
			if (r_buffer[0] == 'T')
			{
				#ifdef DEBUG
				Serial.println(F("It's time ! Setting"));
				#endif
				memcpy(&curr_datetime, r_buffer + 1, sizeof(struct datetime));
				is_time_set = 1;
				return 1;
			}
		}
	}

	return 0;
}

void set_sended(uint32_t pos)
{
	meas_file.seek(pos + offsetof(file_entry, is_sended));
	meas_file.write((uint8_t) 1);
	meas_file.seek(pos);
}

struct file_entry read_entry(uint32_t pos)
{
	uint8_t tmp_array[sizeof(struct file_entry)];
	struct file_entry tmp;

	for(uint32_t i = pos; i < (pos + sizeof(struct file_entry)); i++)
	{
	    tmp_array[i] = meas_file.read();
	}

	meas_file.seek(pos);
	memcpy(&tmp, tmp_array, sizeof(struct file_entry));
	return tmp;
}