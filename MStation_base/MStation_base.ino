#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <printf.h>

struct measure_data
{
	int32_t pressure;
	int32_t temperature;
	float lux;
};

enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_STATE{NOTHING = 0, RX = 1, TX = 2, FAIL = 3};

// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// adress width (in bytes)
#define RF24_ADDR_WIDTH 4
// default address for base station and modules
uint8_t radio_addr[][6] = {"BASEE", "MEAS1"};
// pipes addresses
//uint32_t pipes[6] = {0xFAF1};
// current radio state
volatile uint8_t radio_state = NOTHING;
// buffer for received messages
uint8_t r_buffer[32];

struct measure_data data;

void setup()
{
	// init radio
	Serial.begin(115200);
	printf_begin();
	radio.begin();
	radio.setChannel(127);
	//radio.setAddressWidth(RF24_ADDR_WIDTH);
	radio.setPALevel(RF24_PA_MAX);
	radio.setRetries(4, 15);
	radio.setAutoAck(true);
	radio.enableDynamicAck();
	radio.enableDynamicPayloads();
	//radio.setPayloadSize(sizeof(struct measure_data));
	radio.openReadingPipe(1, radio_addr[1]);
	radio.writeAckPayload(1, "Ok", 2);
	radio.printDetails();
	radio.startListening();
}

void loop()
{
	if (radio.available())
	{
		while(radio.available()){
			radio.read(&data, sizeof(struct measure_data));
		}
		radio.writeAckPayload(1, "Ok", 2);

		Serial.print(F("Pressure: "));
		Serial.print(data.pressure);
		Serial.println(F(" Pa"));
		Serial.print(F("Temp: "));
		Serial.println(data.temperature);
		Serial.print(F("Ambient light HR2: "));
		Serial.println(data.lux);
	}
}