#include <Arduino.h>

#include <WiFiClientSecure.h>
#include "bulut_servis.h"
#include "global.h"


#include "donanim.h"
#include "kinematik.h"
#include "telegrambot.h"

// TomTom ve Kinematik hesaplamalar için koordinat hafızası
float new_lat = 0.0;
float old_lat = 0.0;
float new_lon = 0.0;
float old_lon = 0.0;

static unsigned long last_print = 0;
 // Hız İhlali Kontrolü için zamanlayıcılar
unsigned long last_api_call = 0;
const long API_INTERVAL =1000; // En sık 5 saniyede bir API sorgusu at (Kotayı koru)

// --- 2. MULTICORE (Çoklu Çekirdek) GÖREV TANIMLARI ---
TaskHandle_t BulutTaskHandle;

// CORE 0: İletişim ve API Görevi
// Bu fonksiyon WiFi bağlantısını ve yavaş API işlemlerini yönetir.
void BulutGorevi(void * pvParameters) {
  
  // 1. WiFi ve Cloud Başlat (Bu işlem bloklayıcıdır, bağlanana kadar bekler)
  // Core 0'da olduğu için sensör döngüsünü durdurmaz.
  Serial.println("[CORE 0] Bulut Servisi Baslatiliyor...");
  wifiBaglan();
  bulutServisBaslat(); 
  Serial.println("[CORE 0] Bulut Baglantisi Hazir.");

  for(;;) { // Sonsuz Döngü
    // A. Bulut Senkronizasyonu (Keep Alive)
    
    bulutGuncelle();
    //hizSiniriSorgula();
    // B. Akıllı API Sorgusu (Hız Sınırı)
    // Sadece WiFi varsa, araç hareket halindeyse ve süre dolduysa sorgula
    if (WiFi.status() == WL_CONNECTED && (millis() - last_api_call > API_INTERVAL)) {
             if (abs(new_lat - old_lat) > 0.0001 || abs(new_lon - old_lon) > 0.0001) {
          Serial.println("[CORE 0] Hiz Siniri Sorgulaniyor...");
             // bulut_servis.cpp içindeki fonksiyon
             Serial.println("core 0 da api sorgulanıyor");
             hizSiniriSorgula();
            last_api_call = millis();
        }
    }

    // İşlemciyi rahatlat (Watchdog Timer'ın resetlenmesi için şart)
    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void setup() {
  // 1. Seri Haberleşme (Debug için)
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- SISTEM BASLATILIYOR ---");

  // 2. Donanım Başlatma (Core 1)
    // (MPU_SDA, MPU_SCL, GPS_RX, GPS_TX, OBD_RX, OBD_TX)
  
  donanimBaslat(8, 9, 18, 17,4,5);

  // 3. Kinematik ve Kalibrasyon Yükleme
  kinematikBaslat();

  // 4. Bulut Görevini Core 0'a Ata
    xTaskCreatePinnedToCore(
    BulutGorevi,      // Çalışacak fonksiyon
    "BulutTask",      // Görev Adı
    10000,            // Stack Boyutu (10KB genelde yeterli)
    NULL,             // Parametre
    1,                // Öncelik (1=Düşük, sensörler daha önemli)
    &BulutTaskHandle, // Handle
    0                 // Core ID (0 = Arka Plan / WiFi)
  );

  Serial.println("[CORE 1] Sensor Dongusu Basliyor...");
}










void loop() {
  // CORE 1: Sensör ve Mantık Döngüsü (Arduino varsayılan olarak Core 1'de çalışır)
  // Burası mümkün olduğunca hızlı çalışmalı (10ms - 20ms döngü)

if(kalibrasyon){ onKalibrasyonChange();}
 
if (millis() - last_print > 2000) {
 //obdOku(); 
   //hiziHesapla();
     bilgi();
     
          last_print = millis();
  }
 hiziHesapla();
 cezakes();
gpsOku();
 // Watchdog'u beslemek için çok kısa bekleme (Sensör hızını etkilemez)
  delay(1); 





}
