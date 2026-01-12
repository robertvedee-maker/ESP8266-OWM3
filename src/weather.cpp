/*
 * (c)2026 R van Dorland
 */

#include "weather.h"
#include "secret.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Definities van de variabelen
String weatherTempStr = "--.-°C";
long currentWeatherIcon = 104; // Standaard wolkje
String weatherAlertStr = "Test Code Geel"; // Nieuwe variabele voor de waarschuwing (standaard leeg)

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
            filter["current"]["weather"]["id"] = true;
            filter["alerts"][0]["event"] = true; // Pak alleen de titel van de eerste waarschuwing

            JsonDocument doc;
            deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

            float temp = doc["current"]["temp"];
            int weatherId = doc["current"]["weather"]["id"];

            weatherTempStr = String(temp, 1) + "°C";
            currentWeatherIcon = mapWeatherIdToIcon(weatherId);

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

long mapWeatherIdToIcon(int id)
{
    if (id == 800)
        return 73; // Helder
    if (id >= 801 && id <= 802)
        return 34; // Wolken weinig
    if (id >= 803 && id <= 804)
        return 33; // Wolken veel
    if (id >= 701 && id <= 721)
        return 60; // Mist Nevel Smog
    if (id >= 731)
        return 57; // Stof Zand
    if (id >= 741 && id <= 762)
        return 63; // Vulkanisch As Zand Dust
    if (id == 781)
        return 88; // Tornado
    if (id >= 600 && id <= 622)
        return 54; // Sneeuw
    if (id == 500)
        return 45; // ligte regen
    if (id == 501)
        return 39; // matige regen
    if (id >= 502 && id <= 504)
        return 42; // zware regen
    if (id == 511)
        return 48; // IJzel
    if (id >= 520 && id <= 531)
        return 51; // Regen buien
    if (id >= 300 && id <= 321)
        return 36; // motregen
    if (id >= 200 && id <= 232)
        return 70; // Onweer
    return 102; // Default
}
