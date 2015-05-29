#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Time.h>
#include <MStation.h>

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
	setSyncProvider(request_time);
	setSyncInterval(600);
}

void loop()
{
	uint8_t in_byte;

	if (Serial.available() > 0)
	{
		in_byte = Serial.peek();
		if (in_byte == 'T')
		{
			proc_time_msg();
			#ifdef DEBUG
			Serial.println(F("Get time from serial"));
			#endif
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
	    	memcpy(&data, r_buffer + 1, sizeof(struct measure_data));
			print_measured_serial(&data);
	    	break;
		default:
	    	// do something
	    	break;
	}
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
}

void send_time_to_module()
{	
	struct datetime curr;

	curr = conv_time(now());
	#ifdef DEBUG
	Serial.println(F("Sending time to module"));
	print_datetime_serial(curr);
	#endif
	t_buffer[0] = 'T';
	memcpy(t_buffer + 1, &curr, sizeof(struct datetime));
	radio.stopListening();
	radio.write(t_buffer, 32);
	radio.startListening();
}

time_t request_time()
{
	Serial.println('T');
	return 0;
}

void proc_time_msg()
{
	unsigned long received_time;
	const unsigned long def_time = 1430427600UL;

	if (Serial.find("T"))
	{
		received_time = Serial.parseInt();
		if (received_time >= def_time)
		{
			setTime(received_time);
		}
	}
}