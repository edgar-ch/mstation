// DS3231 I2C address
#define DS3231_ADDR 0x68

// functions headers
void ds3231_init();
uint8_t ds3231_read_reg(uint8_t);
uint8_t ds3231_is_first_run();
struct datetime ds3231_get_datetime();
void ds3231_set_datetime(struct datetime);
inline uint8_t dec2bcd(uint8_t);
