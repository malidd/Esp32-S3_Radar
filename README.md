Not: Lisans bitirme tezi olup ,çalışmamın bir örneğinin burada bulunması için yüklemiş bulunmaktayım. 


Akıllı Araç Hız Takip ve İhlal Bildirim Sistemi
Bu proje, ESP32-S3 mikrodenetleyicisi üzerinde çalışan, sensör füzyonu (GPS, OBD-II ve IMU) kullanarak gerçek zamanlı araç hızı tespiti yapan ve TomTom API üzerinden yol hız sınırlarını sorgulayarak ihlal durumunda bildirim gönderen gömülü bir sistemdir.

🚀 Proje Mimarisi
Sistem, gerçek zamanlı veri işleme ve ağ gecikmelerinden etkilenmemek adına FreeRTOS tabanlı çift çekirdek (Dual-Core) mimarisi üzerine kurulmuştur:

Core 0 (İletişim Çekirdeği): WiFi yönetimi, Arduino IoT Cloud senkronizasyonu ve TomTom API sorguları.

Core 1 (Sensör Çekirdeği): MPU6050 ivmeölçer, GPS verisi okuma, OBD-II sorgulama ve hız hesaplama algoritmaları.

📂 Dosya Yapısı ve Görev Dağılımı
1. Ana Kontrol ve Yönetim
main.cpp: Sistemin giriş noktasıdır. Donanım başlatma işlemlerini yapar ve Core 0 üzerinde çalışacak olan BulutGorevi isimli FreeRTOS görevini (task) oluşturur. Core 1'deki döngüde sensör okumalarını koordine eder.

global.h: Projenin "ortak hafızasıdır". Dosyalar arası paylaşılan tüm değişkenler, nesneler (GPS, MPU6050, OBD) ve MotionProfile yapısı burada extern olarak tanımlanmıştır.

2. Donanım ve Sensör Katmanı
donanim.h/cpp: MPU6050 (I2C), GPS (UART1) ve OBD-II (UART2) donanımlarının pin yapılandırmalarını ve düşük seviyeli başlatma protokollerini içerir.

kinematik.h/cpp: Projenin matematiksel beynidir.

Sensör Füzyonu: GPS sinyali zayıf olduğunda OBD-II verisine, araç durduğunda ise IMU verisine öncelik vererek hiz_kesin değerini üretir.

Hareket Kontrolü: İvmeölçer ve Jiroskop verilerinden gürültüyü arındırarak aracın gerçekten hareket edip etmediğini (hareketVarMi) belirler.

Kalibrasyon: Aracın rölanti titreşimini EEPROM'a kaydederek dinamik eşik değerleri belirleyen gürültü kalibrasyon algoritmasını içerir.

3. Bulut ve İletişim Katmanı
bulut_servis.h/cpp:

WiFi & IoT: WiFiManager ile kolay kurulum ve Arduino IoT Cloud üzerinden uzaktan izleme sağlar.

TomTom API (SnapToRoads): Aracın mevcut koordinatlarını TomTom sunucularına göndererek aracın o an bulunduğu yolun yasal hız sınırını (yol_hiz_siniri) çeker.

telegrambot.h/cpp: Hız ihlali tespit edildiğinde, kullanıcının telefonuna ihlal hızını, yol sınırını ve aracın konumunun Google Maps linkini anlık mesaj olarak gönderir.

🛠 Kullanılan Teknolojiler
Mikrodenetleyici: ESP32-S3 (Dual Core)

Haberleşme: MQTT (Arduino Cloud), HTTPS (TomTom API), Telegram Bot API.

Protokoller: I2C, UART, SPI (EEPROM).

Kütüphaneler: TinyGPS++, ArduinoJson (v7), MPU6050_tockn, WiFiManager.

⚙️ Kurulum ve Çalıştırma
global.h ve bulut_servis.cpp dosyalarındaki API key ve cihaz kimlik bilgilerini güncelleyin.

ESP32-S3 kartınızı seçerek kodu yükleyin.

Araç çalışır haldeyken ancak dururken (rölanti) bir kez kalibrasyon işlemini tetikleyerek ivmeölçer hassasiyetini sisteme tanıtın.

Not: Bu proje akademik bir çalışma (tez) kapsamında geliştirilmiş olup, doğruluk önceliği (Accuracy > Completion) prensibiyle kodlanmıştır.
