// BH1750 i2c bus address
#define BH1750_ADDR 0x23
// BH1750 commands
#define BH1750_PWR_DOWN 0x00
#define BH1750_PWR_ON 0x01
#define BH1750_RESET 0x07
#define BH1750_CONT_HR 0x10
#define BH1750_CONT_HR2 0x11
#define BH1750_CINT_LR 0x13
#define BH1750_1TIME_HR 0x20
#define BH1750_1TIME_HR2 0x21
#define BH1750_1TIME_LR 0x23
// BH1750 resolution mods
enum BH1750_MOD{LR = 0, HR, HR2};

// functions headers
void bh1750_write_cmd(uint8_t);
uint16_t bh1750_read_data();
uint16_t bh1750_meas_Lmode();
uint16_t bh1750_meas_Hmode();
float bh1750_meas_H2mode();