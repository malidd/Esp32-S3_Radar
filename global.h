#ifndef GLOBAL_H
#define GLOBAL_H

// Gerekli Kütüphaneler (Tiplerin tanınması için şart)
#include <Arduino.h>
#include <ArduinoIoTCloud.h> // CloudLocation için
#include <Wire.h>
#include <MPU6050.h>         // MPU6050 nesnesi için
#include <TinyGPSPlus.h>     // TinyGPSPlus nesnesi için
#include <HardwareSerial.h>
#include <UniversalTelegramBot.h>

struct MotionProfile {
    float base_acc_mag;   // Dururken okunan ortalama ivme büyüklüğü (Referans 1g)
    float acc_threshold;  // İvme için gürültü eşiği (Rölanti titreşimi)
    
    float base_gyro_mag;  // Dururken okunan ortalama gyro büyüklüğü (Referans 0)
    float gyro_threshold; // Gyro için gürültü eşiği
    
    bool is_calibrated;   // Kalibrasyon yapıldı mı?
};

extern WiFiClientSecure client;
// --- EXTERN DEĞİŞKENLER (Paylaşılan Veriler) ---
// Not: Burada sadece "bildirim" yapıyoruz. 
// Bu değişkenlerin asıl tanımlarını (definitions) main.cpp içine yazmalısın.

// Bulut ve İletişim Değişkenleri (bulut_servis.cpp ile uyumlu isimler)
extern int hiz_kesin ;
extern int anlik_hiz_obd;    // Eski: anlik_hiz_obd -> Yeni: anlikHizObd
extern int yol_hiz_siniri;   // Eski: yol_hiz_siniri -> Yeni: yolHizSiniri
extern CloudLocation konum;
extern bool ceza ;
extern bool kalibrasyon;
// Kinematik ve Konum Hesaplama Değişkenleri (kinematik.cpp için eklendi)
extern float new_lat;
extern float old_lat;
extern float new_lon;
extern float old_lon;

// Sensör ve Fizik Değişkenleri
extern volatile float gps_speed_kmh;
extern volatile float ax_g, ay_g, az_g; 
extern volatile float acc_forward;

// --- EXTERN NESNELER ---
extern MotionProfile profile;
extern MPU6050 mpu; 
extern TinyGPSPlus gps;
extern HardwareSerial GPS_Serial;
extern HardwareSerial OBD_Serial;

extern const char tomtomApiKey[];
extern const char DEVICE_LOGIN_NAME[];
extern const char DEVICE_KEY[];

// Zamanlayıcı Değişkenleri
extern unsigned long last_gps_time;
extern unsigned long last_obd_time;
extern unsigned long last_kinematik_time;
extern unsigned long last_calibration_time;
extern const unsigned long gps_timeout_ms;
extern const unsigned long calibration_interval_ms;

// Fonksiyon Prototipleri
#endif // GLOBAL_H