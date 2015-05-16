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
	int32_t temperature;
	float temperature2;
	float temperature3;
	uint8_t humidity;
	float lux;
};

/* Headers for shared functions
 * */
void print_datetime_serial(struct datetime);