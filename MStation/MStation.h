/* Shared structures for base and measurement modules 
 * */

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
	float temperature;
	float temperature2;
	float temperature3;
	uint8_t humidity;
	float lux;
};

struct __attribute__((packed)) file_entry
{
	struct measure_data m_data;
	uint8_t is_sended;
};

// Measurement module settings
struct __attribute__((packed)) module_settings
{
	uint16_t meas_period = 300; // in seconds
	uint8_t sensors_prec = 3; // 0 - low, 1 - medium, 2 - high, 3 - very high
	uint8_t is_sleep_enable = 0; // disabled by default
	uint8_t radio_channel = 127; // default channel
};

/* Headers for shared functions
 * */
void print_datetime_serial(struct datetime);
void print_measured_serial(struct measure_data *);
