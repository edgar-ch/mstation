// DS3231 I2C address
#define DS3231_ADDR 0x68
#define DS3231_A2_ADDR 0x0B

// alarm2 modes
enum ALARM_MODE
{
	A2_PER_MINUTE = 0x8E,
	A2_MINUTES_MATCH = 0x8C,
	A2_HOURS_MINUTES_MATCH = 0x88,
	A2_DATE_HOURS_MINUTES_MATCh = 0x80,
	A2_DAY_HOURS_MINUTES_MATCH = 0x90
};

enum ALARM2_CFG
{
	A2_ENABLE = 0x02,
	A2_DISABLE = 0x00
};

// special structs for alarm1 & alarm2
struct alarm_conf
{
	uint8_t seconds = 0;
	uint8_t minute = 0;
	uint8_t hour = 0;
	uint8_t date_day = 0;
};

// functions headers
void ds3231_init();
uint8_t ds3231_read_reg(uint8_t);
uint8_t ds3231_write_reg(uint8_t, uint8_t);
uint8_t ds3231_is_first_run();
struct datetime ds3231_get_datetime();
void ds3231_set_datetime(struct datetime);
void ds3231_set_alarm2(struct alarm_conf *, ALARM_MODE);
void ds3231_alarm2_ctrl(ALARM2_CFG);
void ds3231_ctrl_INT(uint8_t);
#ifdef DEBUG
void ds3231_dump_alarm2();
void ds3231_dump_cfg();
#endif
uint8_t dec2bcd(uint8_t);
