#include "donanim.h"
#include "global.h"
// --- NESNE TANIMLAMALARI (ASIL YER) ---
// Bu nesneler global.h'de "extern" olarak bildirildiği için
// diğer dosyalar (main, kinematik) bunları tanıyacaktır.

TinyGPSPlus gps;
MPU6050 mpu;

// ESP32-S3 UART Yapılandırması
HardwareSerial GPS_Serial(1); // UART 1
HardwareSerial OBD_Serial(2); // UART 2

// Değişken Tanımları
volatile float gps_speed_kmh = 0;

void mpuSetup(int sdaPin, int sclPin) {
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(400000); // 400kHz I2C hızı
    mpu.initialize();
    
    // MPU bağlantı testi
    if (mpu.testConnection()) {
        Serial.println("MPU6050 baglantisi basarili.");
          profile.base_acc_mag = 1.0; 
    profile.acc_threshold = 0.1; // Güvenli, yüksek bir değer
    profile.is_calibrated = false;
    
    Serial.println("MPU Hazir. Lutfen aracı calistirip ROLANTIDE kalibrasyonu  çagirin.");
    } else {
        Serial.println("MPU6050 baglantisi BASARISIZ.");
    }
}

void gpsSetup(int RXPin, int TXPin) {
    GPS_Serial.begin(9600, SERIAL_8N1, RXPin, TXPin);
    Serial.println("GPS seri baglantisi baslatildi.");
}

void obdSetup(int RXPin, int TXPin) {
    // DÜZELTME: OdbSerial -> OBD_Serial (Yazım hatası giderildi)
    OBD_Serial.begin(9600, SERIAL_8N1, RXPin, TXPin);
    Serial.println("OBD-II seri baglantisi baslatildi.");
    
    // OBD Başlatma Komutları
    OBD_Serial.write("ATZ\r");    delay(100); // Reset
    OBD_Serial.write("ATE0\r");   delay(100); // Echo Off
    OBD_Serial.write("ATL0\r");   delay(100); // Linefeed Off
    OBD_Serial.write("ATS0\r");   delay(100); // Spaces Off
    OBD_Serial.write("ATH1\r");   delay(100); // Headers On
    OBD_Serial.write("ATSP0\r");  delay(100); // Auto Protocol
}

void donanimBaslat(int mpuSdaPin, int mpuSclPin, int gpsRxPin, int gpsTxPin, int obdRxPin, int obdTxPin) {
    // Tüm donanımları sırayla başlat
    mpuSetup(mpuSdaPin, mpuSclPin);
    gpsSetup(gpsRxPin, gpsTxPin);
    obdSetup(obdRxPin, obdTxPin);
}