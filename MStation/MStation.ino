//
//    FILE: MStation.ino
//  AUTHOR: Edgar Cherkasov
// VERSION: 0.1.1
// PURPOSE: Code for DIY meteostation
//     URL:
//
// Released to the public domain
//

#include <dht.h>
#include <LiquidCrystal.h>
#include <OneWire.h>

#define DS18B20_ID 0x28
#define DHT11_PIN 2
#define LED 13
#define DS18B20_TRYOUT 5
#define DS18B20_PIN 3

enum DS18_ERR{NOT_FOUND = 1, CRC_ADDR_FAIL, NOT_DS18B20_DEV, CRC_SCRATCH_FAIL};

dht DHT;
LiquidCrystal lcd(4, 5, 9, 10, 11, 12);
OneWire ds(DS18B20_PIN);
char tempS[16], humS[16], ds18_tempS[16];
float ds18_temp;
byte ds18_err;
byte addr[8];
unsigned long sensors_interval = 30000; // pause between readings of sensors
unsigned long lcd_interval = 15000; // pause between shows temperature from DS18 and DHT
unsigned long sending_interval = (unsigned long) 5*60*1000; // pause between sending data to serial (Min*Sec*Millis)
unsigned long prevMillis_sensors = sensors_interval, prevMillis_lcd = lcd_interval, prevMillis_sending = sending_interval;
byte temp12 = 0; // which temperature will show, 0 - from DHT, 1 - from DS18B20
float ds18_temp_mid = 0, DHT_temp_mid = 0, humid_mid = 0; // middle value calculation
byte ds18_data_read_count = 0;
byte dht_data_read_count = 0;

void setup()
{
  lcd.begin(16, 2);
  lcd.print("MStation v0.1.1");
  lcd.setCursor(0, 1);
  lcd.print("Init...");
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  Serial.println("MStationV0.1_Init");
  if (!getAddrDS18B20()) {
    Serial.print("ERR:DS18B20_FATAL_ERROR_");
    Serial.println(ds18_err);
  }
}

void loop()
{
  int chk;
  unsigned long currMillis = millis();
  
  // READ DATA FROM SENSORS
  if (currMillis - prevMillis_sensors > sensors_interval) {
    prevMillis_sensors = currMillis;
    // TURN ON LED
    digitalWrite(LED, HIGH);
    // READ DATA FROM DS18B20+ SENSORS
    if (getTempDS18B20()) {
      // sum values
      ds18_temp_mid += ds18_temp;
      // inc count
      ds18_data_read_count++;
    } else {
      Serial.print("ERR:DS18B20_ERROR_");
      Serial.println(ds18_err);
    }
    // READ DATA FROM DHT11 SENSORS
    chk = DHT.read11(DHT11_PIN);
    switch (chk)
    {
      case DHTLIB_OK:
        // inc count and sum values
        dht_data_read_count++;
        DHT_temp_mid += DHT.temperature;
        humid_mid += DHT.humidity;
        break;
      case DHTLIB_ERROR_CHECKSUM: 
        Serial.println("Checksum_error"); 
        break;
      case DHTLIB_ERROR_TIMEOUT: 
        Serial.println("Time_out_error"); 
        break;
      default: 
        Serial.println("Unknown_error"); 
        break;
    }
    // TURN OFF LED
    digitalWrite(LED, LOW);
  }
  
  // SEND DATA
  if (currMillis - prevMillis_sending > sending_interval) {
    prevMillis_sending = currMillis;
    
    /* Uncomment if you don't wont send mid values
    Serial.print(ds18_temp, 2);
    Serial.print(',');
    Serial.print((int) DHT.temperature);
    Serial.print(',');
    Serial.println((int) DHT.humidity); */
    
    // send
    Serial.print((float) ds18_temp_mid/ds18_data_read_count, 2);
    Serial.print(',');
    Serial.print((float) DHT_temp_mid/dht_data_read_count, 2);
    Serial.print(',');
    Serial.println((float) humid_mid/dht_data_read_count, 2);
    
    // zero values and counter
    ds18_temp_mid = 0; DHT_temp_mid = 0; humid_mid = 0;
    ds18_data_read_count = 0; dht_data_read_count = 0;
  }
  
  // SHOW DATA
  if (currMillis - prevMillis_lcd > lcd_interval) {
    prevMillis_lcd = currMillis;
    // Convert double to char
    dtostrf(ds18_temp, 3, 2, ds18_tempS);
    // Print temp on LCD
    lcd.setCursor(0, 0);
    if (!temp12) {
      lcd.print("Temp1: ");
      lcd.print((int) DHT.temperature);
      lcd.print(" \x99""C    ");
      temp12 = 1;
    } else {
      lcd.print("Temp2: ");
      lcd.print(ds18_tempS);
      lcd.print(" \x99""C");
      temp12 = 0;
    }
    // Print humidity on LCD
    lcd.setCursor(0, 1);
    lcd.print("Humid: ");
    lcd.print((int) DHT.humidity);
    lcd.print(" %");
  }
}

boolean getTempDS18B20()
{
  byte i;
  byte data[12];
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 0); // start conversion, without parasite power
  delay(1000); // delay must be >750ms
  
  ds.reset();
  ds.select(addr);
  ds.write(0xBE); //read scratchpad
  
  for (i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
  // check CRC
  if (OneWire::crc8(data, 8) != data[8]) {
    ds18_err = CRC_SCRATCH_FAIL;
    return false;
  }
    
  ds18_temp = ((data[1] << 8) + data[0])*0.0625; // convert temperaturen
  
  return true;
}

boolean getAddrDS18B20()
{
  byte i;
  i = 0;
  while(!ds.search(addr)) {
    ds.reset_search();
    i++;
    if (i == DS18B20_TRYOUT) {
      ds18_err = NOT_FOUND;
      return false;
    }
    delay(250);
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
    ds18_err = CRC_ADDR_FAIL;
    return false;
  }
  
  if (addr[0] != DS18B20_ID) {
    ds18_err = NOT_DS18B20_DEV;
    return false;
  }
  
  return true;
}
//
// END OF FILE
//

