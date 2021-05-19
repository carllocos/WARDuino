/*
code retried and modifed from
https://github.com/m5stack/M5StickC/blob/master/examples/Unit/ENVII_SHT30_BMP280/SHT3X.cpp
*/
#ifndef __SHT3X_H
#define __HT3X_H

#if defined(ARDUINO)

void senseData(void);
float getTemperature();
float getHumidity();

#endif

#endif