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
long currentWeatherIcon = 57921; // Standaard wolkje

void fetchWeather()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client; // Verander WiFiClient naar WiFiClientSecure
    client.setInsecure(); // Dit is het "foefje": negeer SSL-certificaten
    HTTPClient http;

    // Gebruik HTTPS in de URL
    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(SECRET_LAT) + "&lon=" + String(SECRET_LON) + "&exclude=minutely,hourly,daily,alerts&units=metric&appid=" + String(SECRET_OMW3);

    if (http.begin(client, url)) {
        int httpCode = http.GET();
        Serial.print("OWM Status: ");
        Serial.println(httpCode); // DEBUG

        if (httpCode == HTTP_CODE_OK) {
            JsonDocument filter;
            filter["current"]["temp"] = true;
            filter["current"]["weather"]["id"] = true;

            JsonDocument doc;
            deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

            float temp = doc["current"]["temp"];
            int weatherId = doc["current"]["weather"]["id"];

            weatherTempStr = String(temp, 1) + "°C";
            currentWeatherIcon = mapWeatherIdToIcon(weatherId);
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
        return 57914; // Helder
    if (id >= 801 && id <= 804)
        return 57921; // Wolken
    if (id >= 500 && id <= 531)
        return 57919; // Regen
    if (id >= 200 && id <= 232)
        return 57920; // Onweer
    if (id >= 600 && id <= 622)
        return 57918; // Sneeuw
    return 57921; // Default
}
