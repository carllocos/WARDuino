/*
code retried and modifed from
https://github.com/m5stack/M5StickC/blob/master/examples/Unit/ENVII_SHT30_BMP280/SHT3X.cpp
*/

#include "SensorBMP280.h"

#ifdef ARDUINO
#include "Wire.h"
#include <Adafruit_BMP280.h>
#define ADDRESS 0x44

Adafruit_BMP280 bme;
float cTemp=0;
float fTemp=0;
float humidity=0;
bool sensorinit = false;

bool sensorInitialized(){
  return sensorinit;
}

void initializeSensor(){
  Wire.begin(32, 33);
  while (!bme.begin(0x76)){  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
  }
  sensorinit = true;
}

void senseData()
{
  unsigned int data[6];
  Wire.beginTransmission(ADDRESS);
  Wire.write(0x2C);
  Wire.write(0x06);
  if (Wire.endTransmission()!=0) {
    return;   //return 1
  }
  delay(500);
  Wire.requestFrom(ADDRESS, 6);

  for (int i=0;i<6;i++) {
    data[i]=Wire.read();
  };
  
  delay(50);
  
  if (Wire.available()!=0) {
    return;//return 2
  }

  // Convert the data
  cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
  fTemp = (cTemp * 1.8) + 32;
  humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

}

float getTemperature(){
  return bme.readTemperature();
  /* return cTemp; */
};

float getHumidity(){
  return fTemp;
}

#endif
