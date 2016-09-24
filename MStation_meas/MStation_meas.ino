#include <nRF24L01.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Wire.h>
#ifdef DEBUG
#include <printf.h>
#endif
#include <MStation.h>
#include <SdFat.h>
#include <stddef.h>
#include <avr/sleep.h>
#include <OneWire.h>
#include <dht.h>
#include <EEPROM.h>
#include "BMP180.h"
#include "BH1750FVI.h"
#include "DS3231.h"

// DS18B20 pin, search tryouts and dev ID
#define DS18B20_PIN 5
#define DS18B20_TRYOUT 5
#define DS18B20_ID 0x28
// DHT22 sensor pin
#define DHT22_PIN 6
// EEPROM offsets
#define HAS_CONFIG_FLAG 0
#define CONF_STRUCT 1

// nRF24L01 CE, CS and INT pins
enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_RADIO_EVENTS
{
	RX_MSG = 0,
	TX_SUCCESS,
	TX_FAIL,
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
	WAIT_FOR_SEND,
	WAIT_FOR_TIMEOUT,
	PREPARE_SLEEP,
	SLEEP_NOW
};
// result of data sending
enum SEND_RES
{
	SUCCESS = 0,
	FAILED,
	HAS_NOT_SENDED,
	WAITING
};

SdFat SD;
// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// default addresses for base station and module
uint8_t radio_addr[][6] = {"BASEE", "MEAS1"};
// current radio state
volatile nRF24_RADIO_EVENTS radio_state = LISTEN;
// has ACK payload
volatile uint8_t has_ack_payload = 0;
// buffer for received messages
uint8_t r_buffer[32];
// buffer for transfer messages
uint8_t t_buffer[32];
// current datetime
struct datetime curr_datetime;
// latest measured data
struct measure_data measured;
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
// previous state
STATES prevState = currState;
// module settings
struct module_settings conf;
// timeout flag
volatile uint8_t get_timeout = 0;

// DS18B20
OneWire ds(DS18B20_PIN);
// DS18B20 address
uint8_t ds18b20_addr[8];
// DS18B20 temp
float ds18_temp;

// DHT22 sensor
dht DHT22;

#ifdef DEBUG
volatile uint8_t test_rx = 0;
#endif

// Function declarations
void read_conf_from_EEPROM(struct module_settings *);
void write_conf_to_EEPROM(struct module_settings *);
void start_measure();
void write_to_sd();
void send_latest_data();
uint8_t wait_for_send();
void send_data();
void prepare_sleep();
void make_sleep();
void radio_event();
void alarm_int();
uint8_t try_request_datetime();
void parse_rx_data();
void set_sended(uint32_t);
struct file_entry read_entry(uint32_t);
boolean getAddrDS18B20();
boolean getTempDS18B20();
void send_addr_to_base();
void set_alarm();

void setup()
{
	#ifdef DEBUG
	Serial.begin(115200);
	printf_begin();
	#endif
	// check conf in EEPROM
	if (EEPROM.read(HAS_CONFIG_FLAG) == 0)
	{
		write_conf_to_EEPROM(&conf);
		EEPROM.write(HAS_CONFIG_FLAG, 1);
	}
	// read conf from EEPROM
	read_conf_from_EEPROM(&conf);
	#ifdef DEBUG
	dump_conf_to_serial(&conf);
	#endif
	// init i2c bus
	Wire.begin();
	// init BMP180 pressure sensor and read calibration table
	bmp180_init();
	bmp180_read_calibration();
	// init SD card
	SD.begin(4, 2);
	// init radio
	radio.begin();
	radio.setChannel(125);
	radio.setPALevel(RF24_PA_MAX);
	radio.setRetries(4, 15);
	radio.setAutoAck(true);
	radio.enableDynamicPayloads();
	radio.enableAckPayload();
	radio.openWritingPipe(radio_addr[1]);
	radio.openReadingPipe(1, radio_addr[1]);
	#ifdef DEBUG
	radio.printDetails();
	#endif
	send_addr_to_base();
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
	// init and start DS3231
	ds3231_init();
	// init SD card
	meas_file.open("meas.dat", O_RDWR | O_AT_END);
	curr_pos = meas_file.position();
	prev_pos = curr_pos;
	not_sended_pos = curr_pos;
	// search DS18B20
	getAddrDS18B20();
	#ifdef DEBUG
	curr_datetime = ds3231_get_datetime();
	print_datetime_serial(curr_datetime);
	Serial.println(F("BMP180 inited"));
	#endif
	radio.startListening();
	attachInterrupt(0, radio_event, LOW);
	attachInterrupt(1, alarm_int, FALLING);
	currState = MEAS_TIME;
}

void loop()
{
	uint8_t res;

	if (has_ack_payload)
	{
		prevState = currState;
		currState = PARSE_RX_DATA;
		#ifdef DEBUG
		Serial.println(F("Get ACK payload"));
		#endif
	}

	if (radio_state == RX_MSG)
	{
		prevState = currState;
		currState = PARSE_RX_DATA;
	}

	#ifdef DEBUG
	if (test_rx)
	{
		Serial.println(F("Get RX"));
		test_rx = 0;
	}
	#endif

	switch (currState) {
	case MEAS_TIME:
		// if now time for measurement
		start_measure();
		currState = RECORD_DATA;
		break;
	case RECORD_DATA:
		// write data to SD
		write_to_sd();
		currState = SEND_LATEST_DATA;
		break;
	case SEND_LATEST_DATA:
		send_latest_data();
		currState = WAIT_FOR_SEND;
		break;
	case WAIT_FOR_SEND:
		res = wait_for_send();
		switch (res) {
		case HAS_NOT_SENDED:
			currState = SEND_DATA;
			break;
		case SUCCESS:
		case FAILED:
			cli();
			radio_state = LISTEN;
			sei();
			radio.startListening();
			currState = PREPARE_SLEEP;
			break;
		default:
			break;
		}
		break;
	case SEND_DATA:
		send_data();
		currState = WAIT_FOR_SEND;
		break;
	case WAIT_FOR_TIMEOUT:
		if (get_timeout)
		{
			get_timeout = 0;
			currState = MEAS_TIME;
		}
		break;
	case PARSE_RX_DATA:
		if (has_ack_payload)
		{
			cli();
			has_ack_payload = 0;
			sei();
		}
		else
		{
			cli();
			radio_state = LISTEN;
			sei();
		}
		#ifdef DEBUG
		Serial.println(F("Get message !"));
		#endif
		parse_rx_data();
		currState = prevState;
		break;
	case PREPARE_SLEEP:
		prepare_sleep();
		if (!conf.is_sleep_enable)
		{
			#ifdef DEBUG
			Serial.println(F("Wait for meas timeout"));
			#endif
			currState = WAIT_FOR_TIMEOUT;
		}
		else
		{
			currState = SLEEP_NOW;
		}
		break;
	case SLEEP_NOW:
		make_sleep();
		currState = MEAS_TIME;
		get_timeout = 0;
		break;
	default:
		break;
	}
}

void start_measure()
{
	uint8_t oss = conf.sensors_prec % 4;

	measured.pressure = bmp180_get_pressure(oss);
	measured.temperature = bmp180_get_temp() * 0.1;
	switch (conf.sensors_prec) {
	case 0:
		measured.lux = bh1750_meas_Lmode();
		break;
	case 1:
		measured.lux = bh1750_meas_Hmode();
		break;
	case 2:
	case 3:
		measured.lux = bh1750_meas_H2mode();
		break;
	default:
		measured.lux = bh1750_meas_H2mode();
		break;
	}
	measured.lux = bh1750_meas_H2mode();
	getTempDS18B20();
	measured.temperature2 = ds18_temp;
	DHT22.read22(DHT22_PIN);
	measured.temperature3 = DHT22.temperature;
	measured.humidity = DHT22.humidity;
	curr_datetime = ds3231_get_datetime();
	measured.mtime = curr_datetime;
	#ifdef DEBUG
	print_measured_serial(&measured);
	#endif
}

void write_to_sd()
{
	struct file_entry data_entry;

	#ifdef DEBUG
	Serial.println(F("Write measured to SD"));
	#endif
	//prev_pos = curr_pos;
	data_entry.m_data = measured;
	data_entry.is_sended = 0;
	meas_file.seekSet(curr_pos);
	meas_file.write(&data_entry, sizeof(struct file_entry));
	meas_file.sync();
	curr_pos = meas_file.curPosition();
}

void send_latest_data()
{
	// prepare measurement data
	t_buffer[0] = 'M';
	memcpy(t_buffer + 1, &measured, sizeof(struct measure_data));
	// and send it to radio
	radio.stopListening();
	radio.startWrite(t_buffer, sizeof(t_buffer), 0);
	#ifdef DEBUG
	Serial.println(F("Send data to base station"));
	#endif
}

uint8_t wait_for_send()
{
	switch (radio_state) {
		case TX_SUCCESS:
			#ifdef DEBUG
			Serial.println(F("Sending data success."));
			#endif
			// move file pointer and set flag
			set_sended(not_sended_pos);
			not_sended_pos += sizeof(struct file_entry);
			if (not_sended_pos < curr_pos)
			{
				// have not sended data
				return HAS_NOT_SENDED;
			}
			else
			{
				// nothing new to send
				return SUCCESS;
			}
			break;
		case TX_FAIL:
			#ifdef DEBUG
			Serial.println(F("Sending data failed"));
			#endif
			return FAILED;
		default:
			return WAITING;
	}
}

void send_data()
{
	struct file_entry data_entry;

	#ifdef DEBUG
	Serial.println(F("Sending previous data"));
	#endif
	data_entry = read_entry(not_sended_pos);

	// prepare measurement data
	t_buffer[0] = 'M';
	memcpy(t_buffer + 1, &(data_entry.m_data), sizeof(struct measure_data));
	// and send it to radio
	radio.startWrite(t_buffer, sizeof(t_buffer), 0);
}

void prepare_sleep()
{
	set_alarm();
	ds3231_ctrl_INT(1);
	ds3231_clear_alarm2();
}

void make_sleep()
{
	#ifdef DEBUG
	Serial.println(F("Begin sleep"));
	delay(3);
	#endif
	radio.powerDown();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	/* in this place we will sleep */
	sleep_mode();
	#ifdef DEBUG
	Serial.println(F("Wake up from sleep"));
	#endif
	radio.powerUp();
	radio.startListening();
}

void radio_event()
{
	bool tx, fail, rx;

	radio.whatHappened(tx, fail, rx);

	if (tx)
	{
		radio_state = TX_SUCCESS;
	}
	if (rx)
	{
		radio_state = RX_MSG;
		#ifdef DEBUG
		test_rx = 1;
		#endif
	}
	if (fail)
	{
		radio_state = TX_FAIL;
	}
	if (tx && rx)
	{
		radio_state = TX_SUCCESS;
		has_ack_payload = 1;
	}
}

void alarm_int()
{
	get_timeout = 1;
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
			radio.read(r_buffer, 32);
			if (r_buffer[0] == 'T')
			{
				#ifdef DEBUG
				Serial.println(F("It's time ! Setting"));
				#endif
				memcpy(&curr_datetime, r_buffer + 1, sizeof(struct datetime));
				return 1;
			}
		}
	}

	return 0;
}

void parse_rx_data()
{
	uint8_t rxBytes;

	while (radio.available()) {
		rxBytes = radio.getDynamicPayloadSize();
		if (rxBytes > 0)
		{
			radio.read(r_buffer, 32);
			switch (r_buffer[0]) {
				case 'S':
					// get settings
					memcpy(
						&conf,
						r_buffer + 1,
						sizeof(struct module_settings)
					);
					write_conf_to_EEPROM(&conf);
					#ifdef DEBUG
					Serial.println(F("Get config from radio"));
					#endif
					break;
			    case 'T':
					// do something
					memcpy(
						&curr_datetime,
						r_buffer + 1,
						sizeof(curr_datetime)
					);
					ds3231_set_datetime(curr_datetime);
					#ifdef DEBUG
					Serial.println(F("Get time from radio"));
					print_datetime_serial(curr_datetime);
					#endif
					break;
				case 'A':
					send_addr_to_base();
					break;
				default:
					// do something
					break;
			}
		}
	}
}

void set_sended(uint32_t pos)
{
	meas_file.seekSet(pos + offsetof(file_entry, is_sended));
	meas_file.write((uint8_t) 1);
	meas_file.seekSet(pos);
}

struct file_entry read_entry(uint32_t pos)
{
	struct file_entry tmp;

	meas_file.read(&tmp, sizeof(struct file_entry));
	meas_file.seekSet(pos);

	return tmp;
}

boolean getAddrDS18B20()
{
	byte i;
	i = 0;
	while (!ds.search(ds18b20_addr)) 
	{
		ds.reset_search();
		i++;
		if (i == DS18B20_TRYOUT)
		{
			//ds18_err = NOT_FOUND;
			return false;
		}
		delay(250);
	}

	if (OneWire::crc8(ds18b20_addr, 7) != ds18b20_addr[7])
	{
		//ds18_err = CRC_ADDR_FAIL;
		return false;
	}

	if (ds18b20_addr[0] != DS18B20_ID)
	{
		//ds18_err = NOT_DS18B20_DEV;
		return false;
	}

	return true;
}

boolean getTempDS18B20()
{
	uint8_t i;
	uint8_t data[9];
	
	ds.reset();
	ds.select(ds18b20_addr);
	ds.write(0x44, 0); // start conversion, without parasite power
	delay(1000); // delay must be >750ms
	
	ds.reset();
	ds.select(ds18b20_addr);
	ds.write(0xBE); //read scratchpad
	
	for (i = 0; i < 9; i++) {
		data[i] = ds.read();
	}
	// check CRC
	if (OneWire::crc8(data, 8) != data[8]) {
		//ds18_err = CRC_SCRATCH_FAIL;
		return false;
	}
		
	ds18_temp = ((data[1] << 8) | data[0]) * 0.0625; // convert temperature
	
	return true;
}

void write_conf_to_EEPROM(struct module_settings *conf)
{
	uint8_t i;

	for (i = 0; i < sizeof(struct module_settings); i++)
	{
		EEPROM.write(CONF_STRUCT + i, *((uint8_t *) conf + i));
	}
}

void read_conf_from_EEPROM(struct module_settings *conf)
{
	uint8_t i;
	uint8_t *conf_byte_ptr = (uint8_t *) conf;

	for (i = 0; i < sizeof(struct module_settings); i++, conf_byte_ptr++)
	{
		*conf_byte_ptr = EEPROM.read(CONF_STRUCT + i);
	}
}

void send_addr_to_base()
{
	t_buffer[0] = 'A';
	memcpy(t_buffer + 1, &(radio_addr[1]), 6);
	radio.stopListening();
	radio.write(t_buffer, 7);
	radio.startListening();
}

void set_alarm()
{
	struct alarm_conf aconf;
	uint8_t minutes;

	minutes = curr_datetime.minutes;
	minutes = sum_mod(minutes, conf.meas_period, 60);
	aconf.minute = minutes;

	#ifdef DEBUG
	Serial.print(F("Set alarm 2 to: "));
	Serial.println(minutes);
	#endif
	ds3231_set_alarm2(&aconf, A2_MINUTES_MATCH);
	ds3231_alarm2_ctrl(true);
}
