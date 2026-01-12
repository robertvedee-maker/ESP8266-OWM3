/*
 * (c)2026 R van Dorland
 */

#include <Arduino.h>
#include <Wire.h>

#include "helpers.h"
#include "secret.h"

#include "daynight.h"
#include "network_logic.h" // Volgorde is hier erg belangrijk. niet aanpassen!
#include "weather.h"

// De U8G2 die je hier aanpast ook in [helpers.h] [network_logic.h], [daynight.cpp] aanpassen!

U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // OLED 1.50 128x128
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //OLED 1.54 128x64
// U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //OLED 1.30 128x64
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

/*
class U8G2_SH1107_SEEED_128X128_F_HW_I2C : public U8G2 {
  public: U8G2_SH1107_SEEED_128X128_F_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE) : U8G2() {
    u8g2_Setup_sh1107_i2c_seeed_128x128_f(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
    u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
  }
};
*/

String sunriseStr = "--:--";
String sunsetStr = "--:--";
String currentTimeStr = "--:--:--";
String currentDateStr = "--. --:---:----";

#define SUN 0
#define SUN_CLOUD 1
#define CLOUD 2
#define RAIN 3
#define THUNDER 4

unsigned long lastBrightnessCheck = 0;
const unsigned long brightnessInterval = 60000; // 1 minuut

void setup(void)
{
    // 1. Hardware basis
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    u8g2.setContrast(10);

    // 2. Netwerk (nu lekker kort!)
    setupWiFi(SECRET_SSID, SECRET_PASS);

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
    ArduinoOTA.handle(); // Cruciaal: altijd als eerste
    MDNS.update();

    unsigned long currentMillis = millis();

    // 1. Update elke seconde voor de klok
    static unsigned long lastSecond = 0;
    if (currentMillis - lastSecond >= 1000) {
        lastSecond = currentMillis;

        time_t now = time(nullptr);
        struct tm* timeInfo = localtime(&now);

        if (now > 100000) { // Check of we geldige NTP tijd hebben
            updateDateTimeStrings(timeInfo); // Geef timeInfo door!
            fetchWeather();
        }

        // 2. Elke minuut de zon/helderheid checken
        if (currentMillis - lastBrightnessCheck >= brightnessInterval) {
            lastBrightnessCheck = currentMillis;
            manageBrightness();
        }

        // 3. Teken het scherm (alles gebeurt nu in één keer)
        u8g2.clearBuffer();
        u8g2.enableUTF8Print();

        // Iconen
        long ntpIcon = (now > 1735689600) ? 57367 : 57368;
        long rssiIcon = map(WiFi.RSSI(), -90, -30, 57949, 57953);
        u8g2.setFont(u8g2_font_waffle_t_all);
        u8g2.drawGlyph(0, 8, ntpIcon);
        u8g2.drawGlyph(12, 8, rssiIcon);

        // Tijd (Rechtsboven)
        u8g2.setFont(u8g2_font_spleen6x12_mr);
        u8g2.drawStr(ALIGN_RIGHT(currentTimeStr.c_str()), 8, currentTimeStr.c_str());

        // Weather
        draw("What a beautiful day!", SUN, 27);
        draw("The sun's come out!", SUN_CLOUD, 19);
        draw("It's raining cats and dogs.", RAIN, 8);
        draw("That sounds like thunder.", THUNDER, 12);
        draw("It's stopped raining", CLOUD, 15);

        u8g2.sendBuffer();
    }
}
