/*
 * (c)2026 R van Dorland
 */

#include "weather.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "secret.h"

// Definities van de variabelen
String weatherTempStr = "--.-°C";
long currentWeatherIcon = 57921; // Standaard wolkje

void fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClient client;
    HTTPClient http;

    // URL voor One Call 3.0 API
    String url = "api.openweathermap.org" + String(SECRET_LAT) + 
                 "&lon=" + String(SECRET_LON) + "&exclude=minutely,hourly,daily,alerts&units=metric&appid=" + String(SECRET_OMW3);

    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            // Filter om RAM te besparen op de ESP-01S
            JsonDocument filter;
            filter["current"]["temp"] = true;
            filter["current"]["weather"][0]["id"] = true;

            JsonDocument doc;
            deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

            float temp = doc["current"]["temp"];
            int weatherId = doc["current"]["weather"][0]["id"];

            weatherTempStr = String(temp, 1) + "°C";
            currentWeatherIcon = mapWeatherIdToIcon(weatherId);
        }
        http.end();
    }
}

long mapWeatherIdToIcon(int id) {
    if (id == 800) return 57914; // Helder
    if (id >= 801 && id <= 804) return 57921; // Wolken
    if (id >= 500 && id <= 531) return 57919; // Regen
    if (id >= 200 && id <= 232) return 57920; // Onweer
    if (id >= 600 && id <= 622) return 57918; // Sneeuw
    return 57921; // Default
}
