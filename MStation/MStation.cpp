#include <Arduino.h>
#include <Time.h>
#include "MStation.h"

/**
 * @brief [Print datetine to serial]
 * @details [Print datetime structure to serial port]
 * 
 * @param datetime [datetime structure]
 */
void print_datetime_serial(struct datetime dt)
{
	Serial.print(BASE_YEAR + dt.year);
	Serial.print('/');
	Serial.print(dt.month);
	Serial.print('/');
	Serial.print(dt.date);
	Serial.print(' ');
	Serial.print(dt.hours);
	Serial.print(':');
	Serial.print(dt.minutes);
	Serial.print(':');
	Serial.print(dt.seconds);
	//Serial.print(' ');
	//Serial.println(dayShortStr(dt.day));
}

void print_measured_serial(struct measure_data *md)
{
	Serial.print("MEAS:");
	print_datetime_serial(md->mtime);
	Serial.write(',');
	Serial.print(md->pressure);
	Serial.write(',');
	Serial.print(md->temperature);
	Serial.write(',');
	Serial.print(md->temperature2);
	Serial.write(',');
	Serial.print(md->temperature3);
	Serial.write(',');
	Serial.print(md->humidity);
	Serial.write(',');
	Serial.print(md->lux);
	Serial.println();
}

/**
 * @brief [sum 2 numbers by module]
 * @details [sum 2 numbers by module, its usefull for
 * add measurement period to current minutes]
 * 
 * @param a [a]
 * @param b [b]
 * @param mod [module]
 * @return [result of a + b by module mod]
 */
uint8_t sum_mod(uint8_t a, uint8_t b, uint8_t mod)
{
	if ((a + b >= mod) || (a + b < b))
		return a + b - mod;
	else
		return a + b;
}