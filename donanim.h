#ifndef DONANIM_H
#define DONANIM_H

#include "global.h" // extern tanımları buradan gelir
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <EEPROM.h>

// --- Sadece Fonksiyon Prototipleri ---
// Değişken tanımları (float ax_g vb.) BURADAN SİLİNDİ.
// Çünkü asıl tanımları donanim.cpp'de yapılacak, 
// bildirimleri ise global.h'de zaten var.

void donanimBaslat(int mpuSdaPin, int mpuSclPin, int gpsRxPin, int gpsTxPin, int obdRxPin, int obdTxPin);
void mpuSetup(int sdaPin, int sclPin);
void gpsSetup(int RXPin, int TXPin);
void obdSetup(int RXPin, int TXPin);

#endif // DONANIM_H