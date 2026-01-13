/*
 * (c)2026 R van Dorland
 */

#include "weather.h"
#include "helpers.h"
#include "secret.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Definities van de variabelen
String weatherTempStr = "--.-°C";
long currentWeatherIcon = 104; // Standaard wolkje
String weatherAlertStr = "#### #### ####"; // Nieuwe variabele voor de waarschuwing (standaard leeg)

extern double sunrise_local;
extern double sunset_local;

void fetchWeather()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client; // Verander WiFiClient naar WiFiClientSecure
    client.setInsecure(); // Dit is het "foefje": negeer SSL-certificaten
    HTTPClient http;

    // Gebruik HTTPS in de URL
    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(SECRET_LAT) + "&lon=" + String(SECRET_LON) + "&exclude=" + String(SECRET_EXLUDE) + "&units=metric&appid=" + String(SECRET_OMW3);

    if (http.begin(client, url)) {
        int httpCode = http.GET();
        Serial.print("OWM Status: ");
        Serial.println(httpCode); // DEBUG

        if (httpCode == HTTP_CODE_OK) {
            JsonDocument filter;
            filter["current"]["temp"] = true;
            filter["current"]["weather"][0]["id"] = true;
            filter["alerts"][0]["event"] = true; // Pak alleen de titel van de eerste waarschuwing

            JsonDocument doc;
            deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

            float temp = doc["current"]["temp"];
            int weatherId = doc["current"]["weather"][0]["id"];

            time_t now = time(nullptr);
            struct tm* timeinfo = localtime(&now);

            weatherTempStr = String(temp, 1);
            currentWeatherIcon = mapWeatherIdToIcon(weatherId, timeinfo);

            // Waarschuwing ophalen
            if (doc["alerts"].is<JsonArray>()) {
                // Pak het eerste element [0] uit de lijst met waarschuwingen
                const char* alertEvent = doc["alerts"][0]["event"];
                if (alertEvent) {
                    weatherAlertStr = String(alertEvent);
                }
            } else {
                weatherAlertStr = "GEEN MELDINGEN"; // Geen waarschuwingen aanwezig
            }

            Serial.println("Weer succesvol bijgewerkt!");
        }
        http.end();
    }

    int httpCode = http.GET();
    Serial.print("OWM Status: ");
    Serial.println(httpCode);

    if (httpCode == 200) {
        // Succes!
    } else {
        String payload = http.getString();
        Serial.println("Foutmelding van server: " + payload);
    }
}

long mapWeatherIdToIcon(int id, struct tm* timeinfo)
{
    // Bereken het huidige uur in decimalen
    float currentHour = timeinfo->tm_hour + (timeinfo->tm_min / 60.0);

    // Is de zon momenteel op in Renkum?
    bool isDay = (currentHour > sunrise_local && currentHour < sunset_local);

    if (id >= 200 && id <= 232) { // Onweer
        return isDay ? 71 : 72; // Bijv. 64=Onweer dag, 69=Onweer nacht
    }

    if (id >= 300 && id <= 321) { // Regenachtige dag
        if (id == 300) { // Licht regem
            return isDay ? 46 : 47;
        }
        if (id == 301) { // Matige regen
            return isDay ? 40 : 41;
        }
        if (id == 302) { // Zware regen
            return isDay ? 37 : 38;
        }
        return 51; // Zwaar bewolkt (vaak hetzelfde icoon voor dag/nacht)
    }

    if (id >= 500 && id <= 531) { // Regen
        if (id == 500) { // Licht regem
            return isDay ? 46 : 47;
        }
        if (id == 501) { // Matige regen
            return isDay ? 40 : 41;
        }
        if (id == 502) { // Zware regen
            return isDay ? 37 : 38;
        }
        if (id == 503 || id == 504) { // Zeer zware regen
            return isDay ? 43 : 44;
        }
        if (id == 511) { // IJzel
            return isDay ? 49 : 50;
        }
        if (id >= 520 && id <= 522) { // Regen buien
            return isDay ? 52 : 53;
        }
        return 51; // Zwaar bewolkt (vaak hetzelfde icoon voor dag/nacht)
    }

    if (id >= 600 && id <= 622) { // Sneeuw
        if (id == 600 || id == 601) { // Lichte/moderate sneeuw
            return isDay ? 55 : 56;
        }
        return 54; // Zware sneeuw (vaak hetzelfde icoon voor dag/nacht)
    }

    if (id >= 701 && id <= 781) { // Atmosferische verschijnselen
        if (id >= 701 && id <= 721) { // Mist Nevel Smog
            return isDay ? 61 : 62;
        }
        if (id == 731) { // Stof Zand
            return isDay ? 58 : 59;
        }
        if (id >= 741 && id <= 762) { // Vulkanisch As Zand Dust
            return isDay ? 64 : 65;
        }
        if (id == 781) { // Tornado
            return 88;
        }
    }

    if (id == 800) { // Helder/Onbewolkt
        return isDay ? 73 : 78; // 0 = Zon, 1 = Maan
    }

    if (id >= 801 && id <= 804) { // Bewolkt
        if (id == 801 || id == 802) { // Licht bewolkt
            return isDay ? 34 : 35; // Bijv. 2=Zon/Wolk, 3=Maan/Wolk
        }
        return 33; // Zwaar bewolkt (vaak hetzelfde icoon voor dag/nacht)
    }

    return 104; // Default wolkje
}
