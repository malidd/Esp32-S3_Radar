#ifndef BULUT_SERVIS_H
#define BULUT_SERVIS_H

#include <Arduino.h>
#include <WiFiManager.h>

#include <ArduinoJson.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
     // Sıralama önemli
 // ESP32 built-in kütüphanesi
#include "bulut_servis.h"
#include "global.h"
#include "kinematik.h"  

extern  int hiz_kesin;
 extern int anlik_hiz_obd;
 extern int yol_hiz_siniri;
 extern CloudLocation konum;

// Fonksiyon Prototipleri
void initProperties();
void wifiBaglan();
void bulutServisBaslat();
void bulutGuncelle();

// Parametreleri kaldırdım, çünkü global.h içindeki global değişkenleri kullanacağız.
// Bu sayede fonksiyon çağırırken sürekli parametre girmek zorunda kalmazsın.
void hizSiniriSorgula(); 

// Callback Fonksiyonları
void onCezaChange();
void onKalibrasyonChange();
void onAnlikHizObdChange();
void onyol_hiz_siniriChange();
void onKonumChange();

#endif // BULUT_SERVIS_H