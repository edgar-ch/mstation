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

// DS3231 I2C address
#define DS3231_ADDR 0x68

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
// timeout of time request (in ms)
unsigned int time_req_timeout = 50000;

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
	//attachInterrupt(0, radio_event, LOW);
	/* check for first run of DS3231 and try
	 * to request datetime from base station
	 * */
	if (1/*ds3231_is_first_run()*/)
	{
		if (try_request_datetime())
		{
			#ifdef DEBUG
			Serial.println(F("Trying to request datetime"));
			print_datetime_serial(curr_datetime);
			#endif
		}
	}
	// read datetime
	ds3231_init();
	curr_datetime = ds3231_get_datetime();
	#ifdef DEBUG
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
	/*
	lux = bh1750_meas_Lmode();
	Serial.print(F("Ambient light LR: "));
	Serial.println(lux);
	delay(5000);

	lux = bh1750_meas_Hmode();
	Serial.print(F("Ambient light HR: "));
	Serial.println(lux);
	delay(5000);
	*/
	measured.lux = bh1750_meas_H2mode();

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
/*
#ifdef DEBUG
void print_datetime_serial(struct datetime dt)
{
	Serial.println(F("DS3231 Time:"));
	Serial.print(F("Seconds: "));
	Serial.println(dt.seconds);
	Serial.print(F("Minutes: "));
	Serial.println(dt.minutes);
	Serial.print(F("Hours: "));
	Serial.println(dt.hours);
	Serial.print(F("Day: "));
	Serial.println(dt.day);
	Serial.print(F("Date: "));
	Serial.println(dt.date);
	Serial.print(F("Month: "));
	Serial.println(dt.month);
	Serial.print(F("Year: "));
	Serial.println(dt.year);
}
#endif
*/
/*
Functions for controlling DS3231 RTC clock
*/
void ds3231_init()
{
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(0x0E);
	Wire.write(0x00);
	Wire.write(0x00);
	Wire.endTransmission();
}

uint8_t ds3231_read_reg(uint8_t reg)
{
	uint8_t tmp = 0x00;
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(reg);
	Wire.endTransmission();
	Wire.requestFrom(DS3231_ADDR, 1);
	if (Wire.available())
	{
		tmp = Wire.read();
	}

	return tmp;
}

uint8_t ds3231_is_first_run()
{
	uint8_t stat_reg;
	stat_reg = ds3231_read_reg(0x0F);

	return (stat_reg & 0x80);
}

struct datetime ds3231_get_datetime()
{
	uint8_t seconds, minutes, hours, day, date, month, year;
	struct datetime curr_dt;
	// set read addr to 0x00
	Wire.beginTransmission(DS3231_ADDR);
	Wire.write(0x00);
	Wire.endTransmission();
	// request data from module
	Wire.requestFrom(DS3231_ADDR, 7);
	if (Wire.available())
	{
		seconds = Wire.read();
		minutes = Wire.read();
		hours = Wire.read();
		day = Wire.read();
		date = Wire.read();
		month = Wire.read();
		year = Wire.read();
	}
	Wire.endTransmission();

	#ifdef DEBUG
	Serial.println(F("DS3231 Time:"));
	Serial.print(F("Seconds: "));
	Serial.println(seconds);
	Serial.print(F("Minutes: "));
	Serial.println(minutes);
	Serial.print(F("Hours: "));
	Serial.println(hours);
	Serial.print(F("Day: "));
	Serial.println(day);
	Serial.print(F("Date: "));
	Serial.println(date);
	Serial.print(F("Month: "));
	Serial.println(month);
	Serial.print(F("Year: "));
	Serial.println(year);
	#endif

	// convert from ds3231 format to MSF-like format
	curr_dt.seconds = (((seconds & 0xF0) >> 4) * 10) + (seconds & 0x0F);
	curr_dt.minutes = (((minutes & 0xF0) >> 4) * 10) + (minutes & 0x0F);
	curr_dt.hours = (((hours & 0x30) >> 4) * 10) + (hours & 0x0F);
	curr_dt.day = day & 0x07;
	curr_dt.date = (((date & 0x30) >> 4) * 10) + (date & 0x0F);
	curr_dt.month = (((month & 0x01) >> 4) * 10) + (month & 0x0F);
	curr_dt.year = (((year & 0xF0) >> 4) * 10) + (year & 0x0F);

	return curr_dt;
}