/*
 * (c)2026 R van Dorland
 */

#ifndef WEATHER_H
#define WEATHER_H

#include "secret.h"
#include <Arduino.h>
#include <U8g2lib.h> // Nodig voor het u8g2 object

// Functie declaratie
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol);
void drawWeather(uint8_t symbol, int degree);
void drawScrollString(int16_t offset, const char* s);
void draw(const char* s, uint8_t symbol, int degree);

// Extern variabelen (zodat ze ook elders gebruikt kunnen worden indien nodig)

// [[maybe_unused]] extern const char* ntpServer;
// [[maybe_unused]] extern const long gmtOffset_sec;
// [[maybe_unused]] extern const int daylightOffset_sec;

#endif