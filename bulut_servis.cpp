// 1. Önce Temel Framework ve Ağ Kütüphaneleri (Sıralama Kritik!)
#include <Arduino.h>
//#include <HTTPClient.h> // ESP32 Native HTTPClient buraya gelmeli
#include <ArduinoHttpClient.h>
// 2. Sonra Proje Dosyaları ve Diğer Kütüphaneler
#include "bulut_servis.h"
#include "global.h"
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// ... Kodun geri kalanı ...

// Değişken tanımları main.cpp'de yapıldığı için buraya yazmıyoruz.

const char DEVICE_LOGIN_NAME[] = "************************";
const char DEVICE_KEY[]        = "********************************";
const char tomtomApiKey[] = "*************************************"; 

bool ceza=true ;
bool kalibrasyon;
 int anlik_hiz_obd;
 int yol_hiz_siniri;
 CloudLocation konum;
WiFiManager wifiManager;
WiFiConnectionHandler ArduinoIoTPreferredConnection("", "");

void wifiBaglan() {
  wifiManager.autoConnect("V2nErisim", "12345678");
}

void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  
  // DÜZELTME 1: Değişken isimleri camelCase yapıldı (anlik_hiz -> anlikHizObd)
  // DÜZELTME 2: Yeni Permission yapısı kullanıldı (Hata giderildi)
  // Syntax: addProperty(değişken, İzin).onUpdate(Fonksiyon);

  ArduinoCloud.addProperty(ceza, Permission::ReadWrite).onUpdate(onCezaChange);
ArduinoCloud.addProperty(kalibrasyon, Permission::ReadWrite).onUpdate(onKalibrasyonChange);
ArduinoCloud.addProperty(hiz_kesin, Permission::ReadWrite).onUpdate(onAnlikHizObdChange);
ArduinoCloud.addProperty(yol_hiz_siniri, Permission::ReadWrite).onUpdate(onyol_hiz_siniriChange);
ArduinoCloud.addProperty(konum, Permission::ReadWrite).onUpdate(onKonumChange);

}
void bulutServisBaslat() {
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();    
} 

void bulutGuncelle() {
  ArduinoCloud.update();
}  


void onKalibrasyonChange(){

gurultuKalibrasyonu(); 
Serial.println("kalibarsyon yapıldı");


}
// Callback Fonksiyonları
void onAnlikHizObdChange() {
  // Log
}
void onyol_hiz_siniriChange() {
  // Log
}
void onKonumChange() {
  // Log
}
void onCezaChange(){

}

void hizSiniriSorgula() {
  Serial.println("\n[API] SnapToRoads Sorgusu (Gercek GPS)...");

  // 1. ÖN KONTROLLER
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API HATA] WiFi Bagli Degil!");
    return;
  }

  if (new_lat == 0.0 || new_lon == 0.0) {
    Serial.println("[API HATA] GPS Fix Yok.");
    return;
  }

  // İlk açılış kontrolü
  if (old_lat == 0.0) old_lat = new_lat;
  if (old_lon == 0.0) old_lon = new_lon;

  // 2. SSL/TLS İSTEMCİSİ
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); // Sertifika kontrolünü atla (Hız için)

  const char* server = "api.tomtom.com";
  const int port = 443;
  
  HttpClient http(wifiClient, server, port);

  // 3. KOORDİNAT HAZIRLIĞI
  // DİKKAT: SnapToRoads [Boylam, Enlem] sırasını ister!
  String point1 = String(old_lon, 6) + "," + String(old_lat, 6);
  String point2 = String(new_lon, 6) + "," + String(new_lat, 6);
  
  // DURAĞAN KONUM KONTROLÜ (OFFSET MANTIĞI)
  // Araç duruyorsa veya çok az hareket ettiyse API hata verebilir.
  // Bu durumda eski noktayı (point1) yapay olarak çok az kaydırıyoruz.
  /*if (abs(new_lat - old_lat) < 0.00005 && abs(new_lon - old_lon) < 0.00005) {
      Serial.println("[API] Duragan mod: Jitter (Offset) uygulaniyor.");
      // Boylamı 0.0001 derece (~10m) kaydır
      point1 = String(old_lon - 0.0001, 6) + "," + String(old_lat, 6);
  }*/

  // URL Parametreleri
  // fields: {route{properties{speedLimits{value}}}}
  String fields = "%7Broute%7Bproperties%7BspeedLimits%7Bvalue,unit,type%7D%7D%7D%7D";

  String urlPath = "/snapToRoads/1?key=" + String(tomtomApiKey) +
                   "&points=" + point1 + ";" + point2 +
                   "&fields=" + fields +
                   "&vehicleType=PassengerCar&measurementSystem=auto";

  // 4. İSTEĞİ GÖNDER
  http.get(urlPath);

  // 5. CEVABI İŞLE
  int statusCode = http.responseStatusCode();
  String responseBody = http.responseBody();

  Serial.print("Durum Kodu: ");
  Serial.println(statusCode);

  if (statusCode == 200) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (!error) {
        // Logdan öğrendiğimiz kesin yapı:
        // route -> [0] -> properties -> speedLimits -> value
        
        if (doc["route"].size() > 0) {
            JsonVariant props = doc["route"][0]["properties"];
            
            // speedLimits nesnesi var mı ve değeri sayısal mı?
            if (props["speedLimits"]["value"].is<int>()) {
                
                yol_hiz_siniri = props["speedLimits"]["value"];
                
                Serial.print(">>> [SNAP] GUNCEL YOL HIZI: ");
                Serial.print(yol_hiz_siniri);
                Serial.println(" km/h <<<");
            } 
            else {
                // Ara sokaklarda veya verisi olmayan yollarda buraya düşer.
                Serial.println("[API BILGI] Bu yol segmentinde hız sınırı verisi yok.");
                
                // Mühendislik Kararı: Ara sokakta hız sınırı yoksa ne olsun?
                // Seçenek A: 0 yap (Güvenli)
                yol_hiz_siniri = 0; 
                
                // Seçenek B: Şehir içi varsayılan 50 yap (Opsiyonel)
                // yol_hiz_siniri = 50; 
            }
        } else {
            Serial.println("[API UYARI] Rota eslesmedi (Off-road).");
        }
    } else {
      Serial.print("[JSON HATA] ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("[API HATA] Sunucu Yaniti: ");
    Serial.println(responseBody);
  }

  http.stop();
}





//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*void hizSiniriSorgula() {
  Serial.println("\n[API] SnapToRoads TEST SORGUSU (Izmir Bulvari)...");

  if (WiFi.status() != WL_CONNECTED) return;

  // 1. SSL/TLS AYARLARI
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); 

  const char* server = "api.tomtom.com";
  const int port = 443;
  
  HttpClient http(wifiClient, server, port);

  // 2. TEST KOORDİNATLARI (Denizli İzmir Asfaltı - Hız Verisi Var)
  // SnapToRoads sıralaması: BOYLAM (Lon), ENLEM (Lat)
  String point1 = "29.074310,37.785640"; // Eski nokta
  String point2 = "29.074350,37.785680"; // Yeni nokta (Biraz ilerisi)

  // URL Encode fields: {route{properties{speedLimits{value}}}}
  String fields = "%7Broute%7Bproperties%7BspeedLimits%7Bvalue,unit,type%7D%7D%7D%7D";

  String urlPath = "/snapToRoads/1?key=" + String(tomtomApiKey) +
                   "&points=" + point1 + ";" + point2 +
                   "&fields=" + fields +
                   "&vehicleType=PassengerCar&measurementSystem=auto";

  http.get(urlPath);

  // 3. SONUCU GÖR
  int statusCode = http.responseStatusCode();
  String responseBody = http.responseBody();

  Serial.print("Durum Kodu: ");
  Serial.println(statusCode);

  // JSON YAPISINI GÖRMEK İÇİN BUNU MUTLAKA AÇIYORUZ:
  Serial.println("--- GELEN JSON ---");
  Serial.println(responseBody);
  Serial.println("------------------");

  if (statusCode == 200) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (!error) {
        // TomTom Bazen speedLimits'i dizi [], bazen obje {} dönebilir.
        // Gelen JSON'a göre burayı netleştireceğiz.
        if (doc["route"].size() > 0) {
            
            // Debug: İçeride ne var bakalım
            JsonVariant props = doc["route"][0]["properties"];
            
            // İhtimal 1: Direkt Obje ise
            if (props["speedLimits"].is<JsonObject>()) {
                 yol_hiz_siniri = props["speedLimits"]["value"];
                 Serial.print(">>> [TEST-OBJE] HIZ: ");
            }
            // İhtimal 2: Dizi ise (Bazı yollarda birden fazla limit döner)
            else if (props["speedLimits"].is<JsonArray>()) {
                 yol_hiz_siniri = props["speedLimits"][0]["value"];
                 Serial.print(">>> [TEST-DIZI] HIZ: ");
            }
            else {
                 Serial.println("[UYARI] speedLimits formati farkli veya yok.");
                 yol_hiz_siniri = 0;
            }

            Serial.println(yol_hiz_siniri);

        } else {
            Serial.println("[API UYARI] Rota bulunamadı.");
        }
    } else {
      Serial.print("[JSON HATA] ");
      Serial.println(error.c_str());
    }
  } 
  
  http.stop();
}*/


/*void hizSiniriSorgula() {
  Serial.println("\n[API] SnapToRoads Sorgusu Baslatiliyor...");

  // 1. ÖN KOŞUL KONTROLLERİ
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API HATA] WiFi Bagli Degil!");
    return;
  }

  if (new_lat == 0.0 || new_lon == 0.0) {
    Serial.println("[API HATA] GPS verisi yok.");
    return;
  }

  // İlk açılışta eski koordinat yoksa, yeniyi kopyala
  if (old_lat == 0.0) old_lat = new_lat;
  if (old_lon == 0.0) old_lon = new_lon;

  // 2. SSL/TLS AYARLARI
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); // Sertifika kontrolünü atla (Hız için)

  const char* server = "api.tomtom.com";
  const int port = 443;
  
  HttpClient http(wifiClient, server, port);

  // 3. PARAMETRE HAZIRLIĞI (Snap To Roads için)
  // DİKKAT: SnapToRoads [Boylam,Enlem] (Lon,Lat) sırasını sever!
  String point1 = String(old_lon, 6) + "," + String(old_lat, 6);
  String point2 = String(new_lon, 6) + "," + String(new_lat, 6);
  
  // Eğer araç duruyorsa (Koordinatlar birebir aynıysa) API hata verebilir.
  // Küçük bir sapma (jitter) ekleyerek API'yi tetikliyoruz.
  if (point1 == point2) {
      // Boylamı çok az kaydır
      point1 = String(old_lon - 0.00005, 6) + "," + String(old_lat, 6);
      Serial.println("[API] Duragan mod: Offset eklendi.");
  }

  // URL Encode edilmiş fields parametresi:
  // {route{properties{speedLimits{value,unit,type}}}}
  String fields = "%7Broute%7Bproperties%7BspeedLimits%7Bvalue,unit,type%7D%7D%7D%7D";

  // 4. URL OLUŞTURMA
  // Önceki kodundaki mantık aynen korundu.
  String urlPath = "/snapToRoads/1?key=" + String(tomtomApiKey) +
                   "&points=" + point1 + ";" + point2 +
                   "&fields=" + fields +
                   "&vehicleType=PassengerCar&measurementSystem=auto";

  Serial.print("[API] GET: ");
  // Serial.println(urlPath); 

  // 5. İSTEĞİ GÖNDER
  http.get(urlPath);

  // 6. CEVAP İŞLEME
  int statusCode = http.responseStatusCode();
  String responseBody = http.responseBody();

  Serial.print("Durum Kodu: ");
  Serial.println(statusCode);

  if (statusCode == 200) {
    // JSON PARSE
    JsonDocument doc; // ArduinoJson v7
    DeserializationError error = deserializeJson(doc, responseBody);

    if (!error) {
        // Senin kodundaki JSON yolu: route -> 0 -> properties -> speedLimits -> value
        // SnapToRoads bu fields ile bu yapıyı döner.
        
        // Önce 'route' dizisinin varlığını ve doluluğunu kontrol et
        if (doc["route"].size() > 0) {
            JsonVariant limits = doc["route"][0]["properties"]["speedLimits"];
            
            if (limits["value"].is<int>()) {
                yol_hiz_siniri = limits["value"];
                Serial.print(">>> [SNAP] YOL HIZ SINIRI: ");
                Serial.print(yol_hiz_siniri);
                Serial.println(" km/h <<<");
            } else {
                Serial.println("[API UYARI] Hız sınırı verisi 'null' veya bulunamadı.");
                // Ara sokaklarda veri olmayabilir, eski veriyi koruyabilir 
                // veya varsayılan 50 yapabilirsin.
            }
        } else {
            Serial.println("[API UYARI] Rota bulunamadı (Off-road?).");
        }
    } else {
      Serial.print("[API HATA] JSON Parse: ");
      Serial.println(error.c_str());
      Serial.println(responseBody); // Hata durumunda gövdeyi gör
    }

  } else {
    Serial.print("[API HATA] Sunucu Hatasi. Detay: ");
    Serial.println(responseBody);
  }

  http.stop();
}*/

/*void hizSiniriSorgula() {
  Serial.println("\n[API] Sorgu Fonksiyonuna Girildi."); // 1. Adım: Çağrılıyor mu?

  // Ön Koşul Kontrolleri
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API HATA] WiFi Bagli Degil!");
    return;
  
  if (new_lat == 0.0 || new_lon == 0.0) {
    Serial.println("[API HATA] GPS Koordinatlari Gecersiz (0.0)!");
    return;
  }
  
  // Eski koordinat yoksa (ilk açılış), yeniyi eski gibi kullan (API hatasını önler)
  if (old_lat == 0.0) old_lat = new_lat;
  if (old_lon == 0.0) old_lon = new_lon;

  // 1. GÜVENLİ İSTEMCİ OLUŞTUR
  WiFiClientSecure client;
  client.setInsecure(); // Sertifika kontrolünü atla

  const char* server = "api.tomtom.com";
  const int port = 443;

  // Koordinatları String'e çevir
  String s_old_lat = String(old_lat, 6);
  String s_old_lon = String(old_lon, 6);
  String s_new_lat = String(new_lat, 6);
  String s_new_lon = String(new_lon, 6);

  // URL Oluşturma
  // DİKKAT: TomTom routing API'si iki nokta arasını hesaplar.
  // Noktaları "Lat,Lon" formatında string olarak birleştiriyoruz.
  String urlPath = "/routing/1/calculateRoute/" + 
                   s_old_lat + "," + s_old_lon + ":" + s_new_lat + "," + s_new_lon + 
                   "/json?key=" + String(tomtomApiKey) + 
                   "&routeType=fastest&traffic=false&computeBestOrder=false&vehicleCommercial=false&maxAlternatives=0&sectionType=none&avoid=unpavedRoads";

  Serial.print("[API] İstek Yapiliyor: ");
  // Serial.println(urlPath); // İstersen URL'i tam görüp tarayıcıda test edebilirsin

  // 2. SUNUCUYA BAĞLAN
  if (client.connect(server, port)) {
    Serial.println("[API] Sunucuya Baglanildi. GET gonderiliyor...");

    // 3. HTTP İSTEĞİ
    client.println("GET " + urlPath + " HTTP/1.1");
    client.println("Host: " + String(server));
    client.println("Connection: close");
    client.println(); // Header bitişi

    // 4. CEVABI BEKLE VE OKU
    // Headerları oku ve durum kodunu yakala
    unsigned long timeout = millis();
    while (client.connected() && !client.available()) {
      if (millis() - timeout > 5000) {
        Serial.println("[API HATA] Sunucu cevap vermedi (Timeout)!");
        client.stop();
        return;
      }
    }

    // İlk satırı (HTTP/1.1 200 OK vb.) oku
    String statusLine = client.readStringUntil('\n');
    Serial.println("[API] Sunucu Yaniti: " + statusLine);

    // HTTP 200 OK değilse dur
    if (statusLine.indexOf("200 OK") == -1) {
        Serial.println("[API HATA] Basarili yanit donmedi!");
        // Hata detayını görmek istersen body'i oku:
        // Serial.println(client.readString()); 
        client.stop();
        return;
    }

    // Headerları atla (Boş satıra kadar oku)
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") { 
        break; 
      }
    }

    // JSON Gövdesini Oku
    String payload = client.readString();
    Serial.println("[API] JSON Geldi (" + String(payload.length()) + " byte)");
    // Serial.println(payload); // Çok uzunsa yorumda kalsın, hata ararken açarsın.

    // 5. JSON PARSE İŞLEMİ
    JsonDocument doc; // ArduinoJson v7 (Eğer v6 ise DynamicJsonDocument doc(4096); yap)
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // TomTom yapısının doğruluğunu kontrol et
      if(doc["routes"][0]["legs"][0]["points"][0]["speedLimits"].size() > 0) {
           yol_hiz_siniri = doc["routes"][0]["legs"][0]["points"][0]["speedLimits"][0]["speedLimit"];
           
           Serial.print(">>> [BASARILI] YOL HIZ SINIRI GUNCELLENDI: ");
           Serial.print(yol_hiz_siniri);
           Serial.println(" km/h <<<");
      } else {
           Serial.println("[API UYARI] JSON duzgun ama 'speedLimit' verisi bulunamadi (Dizi bos).");
      }
    } else {
      Serial.print("[API HATA] JSON Ayristirma Hatasi: ");
      Serial.println(error.c_str());
    }
    
  } else {
    Serial.println("[API HATA] Sunucu baglantisi REDDEDILDI (Connection Failed).");
  }
  
  client.stop();
}}*/


// revize/bulut_servis.cpp içine
// Mevcut hizSiniriSorgula fonksiyonunu silip bunu yapıştırın:



/*void hizSiniriSorgula() {
  // Global değişkenleri ve bağlantıyı kontrol et
  if ((new_lat != 0.0) && (WiFi.status() == WL_CONNECTED)) {
    
    // 1. GÜVENLİ İSTEMCİ OLUŞTUR (HTTPClient sınıfı KULLANMIYORUZ)
    WiFiClientSecure client;
    client.setInsecure(); // TomTom sertifikasıyla uğraşmamak için (Hız odaklı)

    // Sunucu ve Port Ayarları
    const char* server = "api.tomtom.com";
    const int port = 443;

    // Koordinatları String'e çevir
    String s_old_lat = String(old_lat, 6);
    String s_old_lon = String(old_lon, 6);
    String s_new_lat = String(new_lat, 6);
    String s_new_lon = String(new_lon, 6);

    // URL Yolunu (Path) Oluştur
    // Not: URL'nin "https://api.tomtom.com" kısmını buraya yazmıyoruz, sadece devamını yazıyoruz.
    String urlPath = "/routing/1/calculateRoute/" + 
                     s_old_lat + "," + s_old_lon + ":" + s_new_lat + "," + s_new_lon + 
                     "/json?key=" + String(tomtomApiKey) + 
                     "&routeType=fastest&traffic=false&computeBestOrder=false&vehicleCommercial=false&maxAlternatives=0&sectionType=none&avoid=unpavedRoads";

    // 2. SUNUCUYA BAĞLAN
    if (client.connect(server, port)) {
      // 3. HTTP GET İSTEĞİNİ MANUEL GÖNDER
      client.println("GET " + urlPath + " HTTP/1.1");
      client.println("Host: " + String(server));
      client.println("Connection: close");
      client.println(); // Header bitişi

      // 4. CEVABI OKU
      // Headerları atla (Basit yöntem)
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") { // Header bitti, gövde başlıyor
          break;
        }
      }

      // JSON Verisini Oku
      String payload = client.readString(); // Kalan her şeyi oku
      
      // 5. JSON PARSE İŞLEMİ (Senin kodunla aynı)
      JsonDocument doc; 
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if(doc["routes"][0]["legs"][0]["points"][0]["speedLimits"].size() > 0) {
             yol_hiz_siniri = doc["routes"][0]["legs"][0]["points"][0]["speedLimits"][0]["speedLimit"];
        }
      }
    }
    client.stop(); // Bağlantıyı kapat
  }
}*/

/*void hizSiniriSorgula() {


  
  // Global değişkenleri kontrol et
  if ((new_lat != 0.0) && (WiFi.status() == WL_CONNECTED)) {
    
    if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // **** DEĞİŞİKLİK BURADA (Artık 2 nokta gönderiyoruz) ****
    // TomTom enlem/boylamı 'lon,lat' sırasıyla ister
    String point1 = String(new_lat, 6) + "," + String(new_lon, 6); // Önceki Konum
    String point2 = String(old_lat, 6) + "," + String(old_lon, 6); // Şimdiki Konum
    String pointsString = point1 + ";" + point2; // İki noktayı birleştir
    
    // fields: {route{properties{speedLimits{value,unit,type}}}}
    String encodedFields = "%7Broute%7Bproperties%7BspeedLimits%7Bvalue,unit,type%7D%7D%7D%7D";
    
    String url = "https://api.tomtom.com/snapToRoads/1?key=" + String(tomtomApiKey) +
                 "&points=" + pointsString +
                 "&fields=" + encodedFields +
                 "&vehicleType=PassengerCar&measurementSystem=auto";

    Serial.println("TomTom API'ye 2 nokta ile istek gönderiliyor...");
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      Serial.println("TomTom Yanıtı (200 OK):");
      Serial.println(payload);

      StaticJsonDocument<1024> doc; 
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        // Şimdiki konumumuzun (ikinci noktanın) hız sınırını alalım
        // `route` dizisi birden fazla segment içerebilir,
        // son segmenti almak en doğrusu olabilir, ancak genelde ilk segment de aynı hızı verir.
        // Güvenli olması için ilk segmenti alıyoruz.
        int speed = doc["route"][0]["properties"]["speedLimits"]["value"] | -1; 
        
        if (speed != -1) {
          Serial.print(">>> YOL HIZ SINIRI: "); Serial.println(speed);
          yol_hiz_siniri = speed; // Bulut değişkenini güncelle        
        } else {
          Serial.println("TomTom: Hız limiti bulunamadı.");
            yol_hiz_siniri = 0;
        }
      } else {
        Serial.print("TomTom JSON ayrıştırma hatası: "); Serial.println(error.c_str());
      } 
    } else {
      Serial.printf("TomTom HTTP Hata Kodu: %d\n", httpResponseCode);
      Serial.println(http.getString()); 
    }
    http.end();
    } else {
    Serial.println("TomTom: WiFi bağlantısı yok.");
    }
}



}*/
