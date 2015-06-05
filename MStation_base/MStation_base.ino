#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Time.h>
#include <MStation.h>

#define PIPES_TOTAL 5

enum nRF24_ADD_PINS{CE_PIN = 7, CS_PIN = 8, INT_PIN = 2};
// RADIO STATE
enum nRF24_STATE{NOTHING = 0, RX = 1, TX = 2, FAIL = 3};

// init RF24 class
RF24 radio(CE_PIN, CS_PIN);
// default address for base station and modules
uint8_t base_radio_addr[] = {"BASEE"};
// current radio state
volatile uint8_t radio_state = NOTHING;
// buffer for received messages
uint8_t r_buffer[32];
// buffer for transfer messages
uint8_t t_buffer[32];

struct measure_data data;
/* slots for connected modules
 * Note: individual settings are not supported
 * */
struct meas_module modules[5];
struct module_settings mod_conf;
uint8_t send_settings_flag = 0;

uint8_t pipeNum;

void setup()
{
	// by default has only one module with preset addr
	modules[0].is_present = 1;
	memcpy(
		modules[0].address,
		"MEAS1",
		6
	);
	// init radio
	Serial.begin(115200);
	radio.begin();
	radio.setChannel(125);
	radio.setPALevel(RF24_PA_MAX);
	radio.setRetries(4, 15);
	radio.setAutoAck(true);
	radio.enableDynamicPayloads();
	radio.enableAckPayload();
	radio.openReadingPipe(0, base_radio_addr);
	for (int i = 0; i < 5; i++)
	{
		if (modules[i].is_present)
		{
			radio.openReadingPipe(i + 1, modules[i].address);
		}
	}
	//radio.openWritingPipe(radio_addr[1]);
	radio.startListening();
	setSyncProvider(request_time);
	request_time();
	//setSyncInterval(600);
}

void loop()
{
	char in_byte;
	uint8_t i;

	if (Serial.available() > 0)
	{
		in_byte = Serial.read();
		switch (in_byte) {
			case 'T':
				proc_time_msg();
				#ifdef DEBUG
				Serial.println(F("Get time from serial"));
				print_datetime_serial(conv_time(now()));
				Serial.println();
				#endif
				break;
			case 'S':
				/* FIXME: 
				wait for whole string, dirty hack, but it works */
				delay(2);
				proc_cmd();
				break;
			default:
				#ifdef DEBUG
				Serial.println(F("Get unknown data"));
				#endif
				Serial.read();
				break;
		}
	}
	if (radio.available(&pipeNum))
	{
		radio.read(&r_buffer, sizeof(r_buffer));
		if (pipeNum == 0)
		{
			if (r_buffer[0] == 'A')
			{
				Serial.print("Get ADDR from module: ");
				Serial.write(t_buffer + 1, 5);
				Serial.println();
			}
		}
		else
		{
			parse_message(pipeNum - 1);
		}
	}
	if (send_settings_flag)
	{
		t_buffer[0] = 'S';
		memcpy(t_buffer + 1, &mod_conf, sizeof(struct module_settings));

		for (i = 0; i < PIPES_TOTAL; i++)
		{
			if (modules[i].is_present)
			{
				#ifdef DEBUG
				Serial.print(F("Write settings for pipe "));
				Serial.println(i + 1);
				#endif
				radio.openWritingPipe(modules[i].address);
				radio.stopListening();
				if (!radio.write(t_buffer, 32))
				{
					#ifdef DEBUG
					Serial.println(F("Send failed, write to ACK"));
					#endif
					radio.startListening();
					radio.writeAckPayload(i + 1, t_buffer, 32);
				}
				else
				{
					radio.startListening();
				}
			}
		}
		send_settings_flag = 0;
	}
}

void parse_message(uint8_t num)
{
	switch (r_buffer[0])
	{
		case 'T':
	    	// time request, send time
			send_time_to_module(num);
			break;
		case 'A':
			// request settings
			Serial.print("Get ADDR from module: ");
			Serial.write(r_buffer + 1, 5);
			Serial.println();
			break;
		case 'M':
	    	// get measurement data, send to serial
			memcpy(
				&(modules[num].m_data),
				r_buffer + 1,
				sizeof(struct measure_data)
			);
			modules[num].has_data = 1;
			print_measured_serial(&(modules[num].m_data));
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

	return curr_datetime;
}

void send_time_to_module(uint8_t num)
{	
	struct datetime curr;

	curr = conv_time(now());
	#ifdef DEBUG
	Serial.println(F("Sending time to module"));
	print_datetime_serial(curr);
	Serial.println();
	#endif
	t_buffer[0] = 'T';
	memcpy(t_buffer + 1, &curr, sizeof(struct datetime));
	radio.openWritingPipe(modules[num].address);
	radio.stopListening();
	if (!radio.write(t_buffer, 32))
	{
		radio.writeAckPayload(num, t_buffer, 32);
	}
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

	received_time = Serial.parseInt();
	if (received_time >= def_time)
	{
		setTime(received_time);
	}
	else
	{
		setTime(def_time);
	}
}

void proc_cmd()
{
	uint8_t cmd;

	cmd = Serial.read();
	#ifdef DEBUG
	Serial.print(F("Get CMD: "));
	Serial.println(cmd);
	#endif
	switch (cmd) {
		case 'S':
			mod_conf.is_sleep_enable = Serial.parseInt();
			#ifdef DEBUG
			Serial.print(F("Get sleep cmd "));
			Serial.println(mod_conf.is_sleep_enable);
			#endif
			send_settings_flag = 1;
			break;
		case 'T':
			mod_conf.meas_period = Serial.parseInt();
			#ifdef DEBUG
			Serial.print(F("Get meas_period "));
			Serial.println(mod_conf.meas_period);
			#endif
			send_settings_flag = 1;
			break;
		case -1:
			#ifdef DEBUG
			Serial.println(F("Not CMD"));
			#endif
			break;
	}
}