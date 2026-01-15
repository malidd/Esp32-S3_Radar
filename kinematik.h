#ifndef KINEMATIK_H
#define KINEMATIK_H

#include <Arduino.h>
#include "donanim.h"
#include "global.h"
#include "telegrambot.h"

// Fonksiyon Prototipleri
void kinematikBaslat();         // Başlangıç ayarları ve EEPROM okuma
float veriGuncelle(bool obdistek);            // Loop içinde çağrılacak ana fonksiyon
void gurultuKalibrasyonu();    // değişti  her yerde 

// Yardımcı Fonksiyonlar (Dışarıdan çağrılmasına gerek yok ama burada dursun)
bool hareketVarMi();//  değişti 
void gpsOku();
int obdOku();
void hiziHesapla();             // Füzyon algoritması burada
void cezakes();
void bilgi();

#endif // KINEMATIK_H
