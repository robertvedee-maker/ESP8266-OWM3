/*
 * (c)2026 R van Dorland
 */

#include <Arduino.h>
#include <Wire.h>

#include "helpers.h"
#include "secret.h"

#include "daynight.h"
#include "fonts/climacons_24pt.h"
#include "network_logic.h" // Volgorde is hier erg belangrijk. niet aanpassen!
#include "weather.h"

// De U8G2 die je hier aanpast ook in [helpers.h] [network_logic.h], [daynight.cpp] aanpassen!

U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // OLED 1.50 128x128
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //OLED 1.54 128x64
// U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //OLED 1.30 128x64
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

String sunriseStr = "--:--";
String sunsetStr = "--:--";
String currentTimeStr = "--:--:--";
String currentDateStr = "--. --:---:----";

// De "belofte" aan de compiler dat deze functie verderop staat:
void drawDisplay(struct tm* timeInfo, time_t now);

unsigned long lastBrightnessCheck = 0;
const unsigned long brightnessInterval = 60000; // 1 minuut

/*
 *  S E T U P
 */
void setup(void)
{
    // 1. Hardware basis
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    u8g2.setContrast(10);

    // 2. Netwerk (nu lekker kort!)
    setupWiFi(SECRET_SSID, SECRET_PASS);

    fetchWeather(); // Haal direct het eerste weerbericht op

    if (WiFi.status() == WL_CONNECTED) {
        toonNetwerkInfo(); // Deze functie bevat de 'rtc_info->reason' check
    }

    setupOTA(DEVICE_MDNS_NAME);

    // 3. Tijd en Regeling
    configTzTime(SECRET_TZ_INFO, SECRET_NTP_SERVER);

    // Initialiseer eerste waarden
    manageBrightness();

    // Succes schermpje (optioneel)
    const char* Msg = "Systeem Online";
    u8g2.clearBuffer();
    u8g2.drawRFrame(0, 0, LCDWidth, LCDHeight, 5);
    u8g2.drawStr(ALIGN_CENTER(Msg), ALIGN_V_CENTER, Msg);
    // u8g2.print(WiFi.localIP().toString());
    u8g2.sendBuffer();
    delay(2000);
}

/*
 *  L O O P
 */
void loop(void)
{
    // 1. Altijd als eerste: Onderhoud voor OTA en Netwerk
    ArduinoOTA.handle();
    MDNS.update();

    unsigned long currentMillis = millis();

    // 2. Weer-update timer (elke 15 minuten = 900.000 ms)
    static unsigned long lastWeatherUpdate = 0;
    const unsigned long weatherInterval = 900000;

    if (currentMillis - lastWeatherUpdate >= weatherInterval || lastWeatherUpdate == 0) {
        lastWeatherUpdate = currentMillis;
        // Haal de data op van OpenWeatherMap 3.0
        fetchWeather();
    }

    // 3. Display en Tijd update timer (elke seconde = 1000 ms)
    static unsigned long lastDisplayUpdate = 0;
    if (currentMillis - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = currentMillis;

        time_t now = time(nullptr);
        struct tm* timeInfo = localtime(&now);

        // Alleen actie ondernemen als we een geldige tijd hebben (na 1 jan 1970)
        if (now > 100000) {
            // Update de tijds-strings (Zo. 12 Jan, etc.)
            updateDateTimeStrings(timeInfo);

            // Controleer elke minuut de helderheid/zonnestand
            static unsigned long lastBrightnessCheck = 0;
            if (currentMillis - lastBrightnessCheck >= 60000) {
                lastBrightnessCheck = currentMillis;
                manageBrightness();
            }

            // Teken alles op het scherm (Klok, Datum, Iconen en Weer)
            drawDisplay(timeInfo, now);
        }
    }
}

void drawDisplay(struct tm* timeInfo, time_t now)
{
    u8g2.clearBuffer();
    u8g2.enableUTF8Print();

    // --- 1. Bovenste balk: Iconen & Datum ---
    bool timeValid = (now > 1735689600);
    long ntpIcon = timeValid ? 57367 : 57368;
    long rssiIcon = map(WiFi.RSSI(), -90, -30, 57949, 57953);

    u8g2.setFont(u8g2_font_waffle_t_all); // Zorg dat dit font actief is
    u8g2.drawGlyph(0, 10, ntpIcon); // Y iets verlaagd naar 10 voor betere weergave
    u8g2.drawGlyph(14, 10, rssiIcon); // X iets meer ruimte gegeven

    // --- Datum rechtsboven ---
    u8g2.setFont(u8g2_font_helvR08_tf); // Switch terug naar tekst-font
    u8g2.drawStr(ALIGN_RIGHT(currentDateStr.c_str()), 10, currentDateStr.c_str());

    // --- Temperatuur naast Weericoon ---
    u8g2.setFont(u8g2_font_helvB24_tf);
    // u8g2.enableUTF8Print();
    u8g2.setCursor(40, 52);
    u8g2.print(weatherTempStr.c_str());
    u8g2.print("°C");
    // u8g2.drawStr(50, 50, weatherTempStr.c_str());

    // --- Huidige Weericoon linksboven ---
    u8g2.setFont(font_climacons_24pt); // Weer-iconen font
    u8g2.drawGlyph(1, 50, currentWeatherIcon); // HET WEER ICOON

    // --- 2. Midden: De Grote Tijd ---
    u8g2.setFont(u8g2_font_helvR18_tf);
    u8g2.drawStr(ALIGN_CENTER(currentTimeStr.c_str()), 100, currentTimeStr.c_str());

    // --- Eventuele weerwaarschuwing ---
    if (weatherAlertStr != "") {
        u8g2.setFont(u8g2_font_helvR08_tf);
        u8g2.drawBox(0, 102, 128, 13); // Een balkje onderaan
        u8g2.setDrawColor(0); // Tekst in negatief (wit op zwart)
        u8g2.drawStr(ALIGN_CENTER(weatherAlertStr.c_str()), 112, weatherAlertStr.c_str());
        u8g2.setDrawColor(1); // Terug naar normaal
    }

    // --- 3. Onderste balk: Zon-tijden & Temperatuur ---
    u8g2.setFont(u8g2_font_helvR08_tf);
    u8g2.drawStr(0, ALIGN_BOTTOM, sunriseStr.c_str());

    u8g2.drawStr(ALIGN_RIGHT(sunsetStr.c_str()), ALIGN_BOTTOM, sunsetStr.c_str());

    u8g2.sendBuffer();
}
