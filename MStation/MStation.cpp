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
	Serial.print(dt.year);
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
	Serial.print(' ');
	Serial.println(dayShortStr(dt.day));
}

void print_measured_serial(struct measure_data *md)
{
	print_datetime_serial(md->mtime);
	Serial.print(md->pressure);
	Serial.write(';');
	Serial.print(md->temperature);
	Serial.write(';');
	Serial.print(md->temperature2);
	Serial.write(';');
	Serial.print(md->temperature3);
	Serial.write(';');
	Serial.print(md->humidity);
	Serial.write(';');
	Serial.print(md->lux);
	Serial.println();
}