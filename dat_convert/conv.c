#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

int main(int argc, char const *argv[])
{
	int fd;
	ssize_t readed;
	struct file_entry tmp;

	fd = open("meas.dat", O_RDONLY);

	while ((readed = read(fd, &tmp, sizeof(struct file_entry))) > 0) {
		printf(
			"%d/%d/%d %d:%d:%d\n",
			tmp.m_data.mtime.year + 2000,
			tmp.m_data.mtime.month,
			tmp.m_data.mtime.date,
			tmp.m_data.mtime.hours,
			tmp.m_data.mtime.minutes,
			tmp.m_data.mtime.seconds
		);
		printf(
			"%d,%f,%f,%f,%d,%f\n", 
			tmp.m_data.pressure,
			tmp.m_data.temperature,
			tmp.m_data.temperature2,
			tmp.m_data.temperature3,
			tmp.m_data.humidity,
			tmp.m_data.lux
		);
	}

	return 0;
}