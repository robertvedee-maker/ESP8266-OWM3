/*
 * (c)2026 R van Dorland
 */

#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

// Declaraties van de variabelen (worden gevuld door fetchWeather)
extern String weatherTempStr;
extern long currentWeatherIcon;

// Functies
void fetchWeather();
long mapWeatherIdToIcon(int id);

#endif
