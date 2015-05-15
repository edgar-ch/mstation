#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

struct __attribute__((packed)) datetime
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
};

struct __attribute__((packed)) measure_data
{
	struct datetime mtime;
	int32_t pressure;
	int32_t temperature;
	float temperature2;
	float temperature3;
	uint8_t humidity;
	float lux;
};

enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_STATE{NOTHING = 0, RX = 1, TX = 2, FAIL = 3};

// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// default address for base station and modules
uint8_t radio_addr[][6] = {"BASEE", "MEAS1"};
// current radio state
volatile uint8_t radio_state = NOTHING;
// buffer for received messages
uint8_t r_buffer[32];
// buffer for transfer messages
uint8_t t_buffer[32];

struct measure_data data;

void setup()
{
	// init radio
	Serial.begin(115200);
	radio.begin();
	radio.setChannel(125);
	radio.setPALevel(RF24_PA_MAX);
	radio.setRetries(4, 15);
	radio.setAutoAck(true);
	radio.enableDynamicPayloads();
	radio.openReadingPipe(1, radio_addr[0]);
	radio.openWritingPipe(radio_addr[1]);
	radio.startListening();
}

void loop()
{
	if (radio.available())
	{
		while(radio.available()){
			radio.read(&data, sizeof(struct measure_data));
		}

		Serial.print(F("Pressure: "));
		Serial.print(data.pressure);
		Serial.println(F(" Pa"));
		Serial.print(F("Temp: "));
		Serial.println(data.temperature);
		Serial.print(F("Ambient light HR2: "));
		Serial.println(data.lux);
	}
}

void parse_message()
{
	switch (r_buffer[0]) {
		case 'T':
	    	// time request
	    	break;
		case 'R':
	    	// request settings
	    	break;
		default:
	    	// do something
	}
}