#include "kinematik.h"

MotionProfile profile;

int hiz_kesin ;

// Zamanlama değişkenleri
unsigned long last_obd_time = 0;
unsigned long last_gps_time = 0;
unsigned long cezatime=0;

// Filtreleme için önceki değerler
const float THRESHOLD_MULTIPLIER = 1.2; 



void bilgi () {

  Serial.print("GPS Hiz: "); Serial.print(gps_speed_kmh);
      Serial.print(" | OBD Hiz: "); Serial.print(anlik_hiz_obd);
      Serial.print(" | Sinir: "); Serial.println(yol_hiz_siniri);
      Serial.print(" Konum: Lat: "); Serial.print(new_lat, 6);
      Serial.print(" Lon: "); Serial.print(new_lon, 6);
      Serial.print(" | Satelit Sayisi: "); Serial.print(gps.satellites.value());
    Serial.print(" | Konum zamanı: ");Serial.println(gps.location.age());
      Serial.print("Hız Kesin: "); Serial.print(hiz_kesin);
     Serial.print(" | Hız var mı: "); Serial.println(hareketVarMi());
  Serial.println("---------------------------");

}
void kinematikBaslat() { 
    
    // EEPROM Başlat (512 byte yeterli)
    if (!EEPROM.begin(512)) {
        Serial.println("HATA: EEPROM başlatılamadı.");
        // Varsayılan güvenli değerleri ata
        profile.base_acc_mag = 1.0;
        profile.acc_threshold = 0.1;
        profile.is_calibrated = false;
        return;
    }

    // Veriyi Çek
    EEPROM.get(0, profile);

    // Veri geçerli mi kontrol et (Nancheck ve Mantık kontrolü)
    // Eğer kalibrasyon yapılmamışsa veya saçma sapan (NaN) değerler varsa
    if (isnan(profile.base_acc_mag) || profile.base_acc_mag < 0.5 || profile.base_acc_mag > 1.5||profile.is_calibrated){
        Serial.println("Gecerli kalibrasyon verisi YOK. Varsayilanlar kullanilacak.");
        profile.base_acc_mag = 1.0;
        profile.acc_threshold = 0.1; 
        profile.base_gyro_mag = 0.0;
        profile.gyro_threshold = 2.0;
        profile.is_calibrated = false;
    } else {
        Serial.println("Kayitli MotionProfile Yuklendi:");
        Serial.printf("Ref Acc: %.3f | Acc Threshold: %.3f\n", profile.base_acc_mag, profile.acc_threshold);
        profile.is_calibrated = true;
    }
}



bool hareketVarMi() {// anlık olarak hareket var mı hesaplayan fonksiyon 
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // 1. Anlık Büyüklükleri Hesapla (Tüm eksenler dahil)
    float acc_now = sqrt(pow(ax/16384.0, 2) + pow(ay/16384.0, 2) + pow(az/16384.0, 2));
    float gyro_now = sqrt(pow(gx/131.0, 2) + pow(gy/131.0, 2) + pow(gz/131.0, 2));

    // 2. Referans değerden farkını al (Sapma miktarı)
    float acc_delta = abs(acc_now - profile.base_acc_mag);
    float gyro_delta = abs(gyro_now - profile.base_gyro_mag);

    // 3. KARŞILAŞTIRMA (Hareket Kontrolü)
    
    // A. İVME KONTROLÜ (Gaz, Fren, Çukur)
    if (acc_delta > profile.acc_threshold) {
        return true; // Hareket Var
    }

    // B. JİROSKOP KONTROLÜ (Dönüş, Viraj, Şerit Değiştirme)
    if (gyro_delta > profile.gyro_threshold) {
        return true; // Hareket Var
    }

    // İkisi de eşiğin altındaysa, araç rölanti titreşimi içinde demektir -> DURUYOR.
    return false; 
}





float veriGuncelle(bool obdistek) {
    //  gpsOku(); 

    // 2. Eğer karar mekanizması OBD verisine ihtiyaç duyuyorsa
    if (obdistek) {
        // Sadece zamanı geldiyse fiziksel sorgu yap (Isınmayı önlemek için)
        if (millis() - last_obd_time > 200) {
            obdOku(); 
            last_obd_time = millis();
        }
        // ÖNEMLİ: Zamanı gelmese bile "anlik_hiz_obd" içindeki SON veriyi döndür.
        // Böylece GPS'e düşüp hızın sıfırlanmasını engelleriz.
        return (float)anlik_hiz_obd;
    }

    // 3. Eğer OBD gerekli değilse direkt GPS hızını döndür
    return (float)gps_speed_kmh;
}



void gpsOku() {
    while (GPS_Serial.available() > 0) {
    char c = GPS_Serial.read();
    gps.encode(c); // Sonucu if içine alma, sadece besle
    // Debug için: Gelen ham veriyi görmek istersen:
    // Serial.write(c); 
  } 
            // Konum Güncelleme Mantığı
            if (gps.location.isValid() && gps.location.isUpdated()) {
              // Serial.printf("GPS Konumu Alindi.");
                float temp_lat = gps.location.lat();
                float temp_lon = gps.location.lng();

                gps_speed_kmh = gps.speed.kmph();//bypass yap durduğu yerde 0.5 fln veriyor

                      //  last_gps_time=gps.location.age()

                // DÜZELTME: Matematiksel fark kontrolü
                if (abs(temp_lat - old_lat) > 0.00005 || abs(temp_lon - old_lon) > 0.00005) {
                    old_lat = new_lat; // Eskiyi sakla
                    old_lon = new_lon;
                    
                    new_lat = temp_lat; // Yeniyi al
                    new_lon = temp_lon;
                    Serial.printf("GPS Konum Guncellendi: Lat: %.6f, Lon: %.6f\n", new_lat, new_lon);

                    // CloudLocation nesnesini güncelle (IoT Cloud için)
                    konum = {new_lat, new_lon};
                    
                    
                }
            }

            
        }

int obdOku() {
    // OBD_Serial donanim.cpp'de tanımlı (OdbSerial değil!)
    OBD_Serial.write("010D\r"); // Hız isteği PID
    
    // Basit bir timeout mekanizması (Kodun donmaması için)
    unsigned long start = millis();
    String response = "";
    
    while (millis() - start < 50) { // Max 50ms bekle
        if (OBD_Serial.available()) {
            char c = OBD_Serial.read();
            if (c != '>' && c != '\r' && c != '\n') { // Gereksiz karakterleri at
                response += c;
            }
        }
    }

    // Hex Çözümleme: Genelde cevap "41 0D XX" şeklindedir
    if (response.indexOf("410D") != -1) { // Boşluksuz kontrol (daha güvenli)
        int index = response.indexOf("410D");
        String hexVal = response.substring(index + 4, index + 6); // XX kısmını al
        anlik_hiz_obd = strtol(hexVal.c_str(), NULL, 16); // Hex to Int
    } 
    // Alternatif (Boşluklu gelirse):
    else if (response.indexOf("41 0D") != -1) {
        int index = response.indexOf("41 0D");
        String hexVal = response.substring(index + 6, index + 8);
        anlik_hiz_obd = strtol(hexVal.c_str(), NULL, 16);
    }

return anlik_hiz_obd;

} 

void hiziHesapla() {// bu mantık düzgün çalışmıyor 
    // SENSOR FUSION (Karar Mekanizması)
    bool hareket = hareketVarMi();

    if (!hareket){

    hiz_kesin=0;
    return;
     } 
 


    else if(gps.speed.isValid()&& gps.location.age()<1500  &&gps.location.isValid() ) {

         hiz_kesin = veriGuncelle(false);
return;
    }

    else {

         hiz_kesin =  veriGuncelle(true);
         return;
    }
    
    
}

// BU FONKSİYON ARAÇ ÇALIŞIRKEN AMA DURURKEN BİR KERE ÇAĞRILMALI 
// cihaz kalibre olduktan sonra eeproma kaydeder.
void gurultuKalibrasyonu() {
    Serial.println("Kalibrasyon basladi... 2 saniye bekleyin...");
    
    float sum_acc_mag = 0;
    float sum_gyro_mag = 0;
    float max_acc_diff = 0; // Maksimum sapmayı bulacağız
    float max_gyro_diff = 0;
    
    int sample_count = 200; // 200 örnek (Yaklaşık 2-3 saniye sürer)

    // 1. ADIM: ORTALAMAYI BUL (BASELINE)
    for (int i = 0; i < sample_count; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        // Ham veriyi fiziksel birime çevir (Magnitude Hesabı)
        // Yön önemsiz, büyüklük önemli: sqrt(x^2 + y^2 + z^2)
        float acc_mag = sqrt(pow(ax/16384.0, 2) + pow(ay/16384.0, 2) + pow(az/16384.0, 2));
        float gyro_mag = sqrt(pow(gx/131.0, 2) + pow(gy/131.0, 2) + pow(gz/131.0, 2));
        
        sum_acc_mag += acc_mag;
        sum_gyro_mag += gyro_mag;
        delay(5);
    }
    
    profile.base_acc_mag = sum_acc_mag / sample_count;
    profile.base_gyro_mag = sum_gyro_mag / sample_count;

    // 2. ADIM: GÜRÜLTÜ (TİTREŞİM) ARALIĞINI BUL
    // Tekrar okuyup, ortalamadan en fazla ne kadar saptığına bakıyoruz.
    for (int i = 0; i < sample_count; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        float acc_mag = sqrt(pow(ax/16384.0, 2) + pow(ay/16384.0, 2) + pow(az/16384.0, 2));
        float gyro_mag = sqrt(pow(gx/131.0, 2) + pow(gy/131.0, 2) + pow(gz/131.0, 2));
        
        // Farkı bul (Mutlak değer)
        float acc_diff = abs(acc_mag - profile.base_acc_mag);
        float gyro_diff = abs(gyro_mag - profile.base_gyro_mag);
        
        if (acc_diff > max_acc_diff) max_acc_diff = acc_diff;
        if (gyro_diff > max_gyro_diff) max_gyro_diff = gyro_diff;
        delay(2);
    }

    // Eşik değerlerini belirle (Maksimum gürültü * Güvenlik Çarpanı)
    profile.acc_threshold = max_acc_diff * THRESHOLD_MULTIPLIER;
    profile.gyro_threshold = max_gyro_diff * THRESHOLD_MULTIPLIER;
    
    // Çok düşük eşiklere karşı koruma (Sensör çok temizse bile bir alt limit olsun)
    if (profile.acc_threshold < 0.02) profile.acc_threshold = 0.02; 
    if (profile.gyro_threshold < 1.0) profile.gyro_threshold = 1.0;

    profile.is_calibrated = true;
    
    EEPROM.put(0, profile); // RAM'deki buffer'a yaz
    
    if (EEPROM.commit()) {  // Flash belleğe yak
        Serial.println("Kalibrasyon verileri EEPROM'a kaydedildi.");
    } else {
        Serial.println("HATA: EEPROM'a kayit yapilamadi!");
    }
    Serial.printf("Kalibrasyon Tamam!\n");
    Serial.printf("Ref Acc: %.3f | Acc Threshold: %.3f\n", profile.base_acc_mag, profile.acc_threshold);
    Serial.printf("Ref Gyro: %.3f | Gyro Threshold: %.3f\n", profile.base_gyro_mag, profile.gyro_threshold);
}


  void cezakes(){
 if (hiz_kesin > yol_hiz_siniri && yol_hiz_siniri > 0) {
      // Basit bir Debounce (Gürültü önleme) eklenebilir
      Serial.print("!!! HIZ IHLALI !!! Hiz: ");
      Serial.print(gps_speed_kmh);
      Serial.print(" / Sinir: ");
      Serial.println(yol_hiz_siniri);
      mesaj(yol_hiz_siniri,hiz_kesin);
      ceza=false;
      cezatime=millis();
      // Buraya bir Buzzer kodu ekleyebilirsin:
      // digitalWrite(BUZZER_PIN, HIGH);
  }
if ((millis()-cezatime)>3000 && !ceza ){
  Serial.print("ceza gösterim süresi doldu sıfırlanıyor");
  ceza =!ceza;
  cezatime=millis();
}
 

 
  
}