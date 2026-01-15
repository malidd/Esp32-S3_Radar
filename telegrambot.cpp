#include "telegrambot.h"
 WiFiClientSecure client;
#define BOTtoken "********************************************"  // BotFather'dan aldığınız token
#define CHAT_ID "**********"
void mesaj(int yol_hiz_siniri,int hiz_kesin){

    // BU TANIM ŞARTTIR
    client.setInsecure();    // Sertifika kontrolünü atla (Hız için)
    
    UniversalTelegramBot bot(BOTtoken, client);
// 4. MESAJI HAZIRLA
    // Telegram'da kalın yazı için Markdown formatı kullanılır (*Bold*)
    String mesajMetni = "⚠️ *HIZ SINIRI İHLALİ!* ⚠️\n\n";
    mesajMetni += "🚗 *Hızınız:* " + String(hiz_kesin) + " km/h\n";
    mesajMetni += "🛑 *Sınır:* " + String(yol_hiz_siniri) + " km/h\n";
    
    // Google Maps Linki Oluşturma
    mesajMetni += "\n📍 *Konum:* \n";
    mesajMetni += "https://www.google.com/maps/search/?api=1&query=" + String(new_lat, 6) + "," + String(new_lon, 6);

    // 5. GÖNDER
    // "Markdown" parametresi, kalın/eğik yazıları etkinleştirir.
    if (bot.sendMessage(CHAT_ID, mesajMetni, "Markdown")) {
        Serial.println("[TELEGRAM] Uyarı mesajı başarıyla gönderildi!");
       
    } else {
        Serial.println("[TELEGRAM] HATA: Mesaj gönderilemedi.");
    }
}



