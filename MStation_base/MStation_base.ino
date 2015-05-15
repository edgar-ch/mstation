#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Time.h>
#include "../shared/MStation.h"

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
struct datetime curr_datetime;

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
	uint8_t in_byte;
	time_t ct;

	if (Serial.available() > 0)
	{
		in_byte = Serial.read();
		if (in_byte == 'T')
		{
			ct = Serial.parseInt();
			curr_datetime.seconds = second(ct);
			curr_datetime.minutes = minute(ct);
			curr_datetime.hours = hour(ct);
			curr_datetime.day = weekday(ct);
			curr_datetime.date = day(ct);
			curr_datetime.month = month(ct);
			curr_datetime.year = year(ct) - 2000;
			dump_datetime(curr_datetime);
		}
	}
	if (radio.available())
	{
		while(radio.available())
		{
			radio.read(&r_buffer, sizeof(r_buffer));
		}
		parse_message();
	}
}

void parse_message()
{
	switch (r_buffer[0])
	{
		case 'T':
	    	// time request, send time
	    	send_time_to_module();
	    	break;
		case 'R':
	    	// request settings
	    	break;
	    case 'M':
	    	// get measurement data, send to serial
	    	memcpy(&data, r_buffer, sizeof(struct measure_data));
	    	send_measured_to_serial();
	    	break;
		default:
	    	// do something
	    	break;
	}
}

void send_time_to_module()
{
	t_buffer[0] = 'T';
	memcpy(t_buffer + 1, &curr_datetime, sizeof(struct datetime));
	radio.stopListening();
	radio.write(t_buffer, 32);
	radio.startListening();
}

void send_measured_to_serial()
{
	Serial.print(F("Pressure: "));
	Serial.print(data.pressure);
	Serial.println(F(" Pa"));
	Serial.print(F("Temp: "));
	Serial.println(data.temperature);
	Serial.print(F("Ambient light HR2: "));
	Serial.println(data.lux);
}

void dump_datetime(struct datetime dt)
{
	//Serial.println(F("DS3231 Time:"));
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