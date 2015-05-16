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