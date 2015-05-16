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
#include "BMP180.h"
#include "BH1750FVI.h"
#include "DS3231.h"

// nRF24L01 CE, CS and INT pins
enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_STATE{NOTHING = 0, RX = 1, TX = 2, FAIL = 3};

// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// default addresses for base station and module
uint8_t radio_addr[][6] = {"BASEE", "MEAS1"};
// current radio state
volatile uint8_t radio_state = NOTHING;
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
	radio.startListening();
	/* check for first run of DS3231 and try
	 * to request datetime from base station
	 * */
	if (ds3231_is_first_run())
	{
		if (try_request_datetime())
		{
			#ifdef DEBUG
			Serial.println(F("Trying to request datetime"));
			print_datetime_serial(curr_datetime);
			#endif
			ds3231_set_datetime(curr_datetime);
		}
	}
	// read datetime
	ds3231_init();
	curr_datetime = ds3231_get_datetime();
	#ifdef DEBUG
	print_datetime_serial(curr_datetime);
	radio.printDetails();
	Serial.println(F("BMP180 inited"));
	#endif
	attachInterrupt(0, radio_event, LOW);
}

void loop()
{
	uint8_t oss = 0;
	uint16_t lux;
	struct measure_data measured;
	uint8_t rxBytes;

	measured.pressure = bmp180_get_pressure(oss);
	measured.temperature = bmp180_get_temp();
	measured.lux = bh1750_meas_H2mode();
	measured.mtime = ds3231_get_datetime();

	// prepare measurement data
	t_buffer[0] = 'M';
	memcpy(t_buffer + 1, &measured, sizeof(struct measure_data));
	radio.stopListening();
	radio.startWrite(t_buffer, sizeof(t_buffer), 0);

	switch (radio_state) {
		case TX:
			radio_state = NOTHING;
			#ifdef DEBUG
			Serial.println(F("Sending data success."));
			#endif
			break;
		case RX:
			radio_state = NOTHING;
			#ifdef DEBUG
			Serial.println(F("Receive failed."));
			#endif
			break;
		case FAIL:
			radio_state = NOTHING;
			#ifdef DEBUG
			Serial.println(F("Receive failed"));
			#endif
		default:
			break;
	}
	
	#ifdef DEBUG
	Serial.print(F("Pressure: "));
	Serial.print(measured.pressure);
	Serial.println(F(" Pa"));
	Serial.print(F("Temp: "));
	Serial.println(measured.temperature);
	Serial.print(F("Ambient light HR2: "));
	Serial.println(measured.lux);
	#endif

	delay(50000);
}

void radio_event()
{
	bool tx, fail, rx;
	uint8_t rxBytes;

	radio.whatHappened(tx, fail, rx);

	if (tx)
	{
		radio_state = TX;
		//radio.startListening();
		/*
		if (radio.isAckPayloadAvailable())
		{
			// move pointer
			#ifdef DEBUG
			Serial.println(F("Sending data success."));
			#endif
		}
		*/
	}
	else if (rx)
	{
		radio_state = RX;
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
		//radio.writeAckPayload(0, "Ok", 2);
	}
	else
	{
		radio_state = FAIL;
	}
	radio.startListening();
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
	radio.write("T", 2);  // second byte for \0 character
	radio.startListening();
	while ((millis() - curr_millis) < time_req_timeout)
	{
		if (radio.available())
		{
			radio.read(r_buffer, 32);
			if (r_buffer[0] == 'T')
			{
				memcpy(&curr_datetime, r_buffer + 1, sizeof(struct datetime));
				is_time_set = 1;
				return 1;
			}
		}
	}

	return false;
}
