#include <Arduino.h>
#include <WiFi.h>

#include "Pogoda.h"


const char *tiz="Europe/Warsaw";

#define URLS "https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f%s&timeformat=iso8601&forecast_days=3&timezone=%s" \
    "&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl,cloud_cover,relative_humidity_2m" \
    "&daily=wind_speed_10m_min,wind_speed_10m_max,weather_code,cloud_cover_min,cloud_cover_max,wind_gusts_10m_max" \
    "&hourly=temperature_2m,apparent_temperature,rain,showers,snowfall,precipitation_probability"
    

float geo2f(int32_t d)
{
    return d/3600.0;
}

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
static bool reqi=false;
WiFiClientSecure secClient;
bool haveWeather=false;

uint32_t lastWeather;

void initRequest()
{
    secClient.setInsecure();
}

bool getWeather()
{
    HTTPClient https;
    char url[1024];
    char ebuf[64];

    if (haveWeather && millis() - lastWeather < 600000) {
        makeWeather();
        return true;
    }


    
    memset(url,0,1024);
    if (geoPrefs.elev == ELEV_AUTO) ebuf[0] = 0;
    else sprintf(ebuf,"&elevation=%d.0", geoPrefs.elev);
    sprintf(url,URLS, geo2f(geoPrefs.lat), geo2f(geoPrefs.lon),ebuf,
        geoPrefs.autotz ? "auto" : tiz);
    if (!reqi) {
        initRequest();
        reqi=true;
    }
    if (debugMode) Serial.printf("URL=%s\n",url);
    haveWeather=false;
    if (https.begin(secClient, url)) {  // HTTPS
        https.setTimeout(750);
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = https.getString();
                if (!parseWeather(payload.c_str())) {                    
                    lastWeather = millis();
                    haveWeather = true;
                    makeWeather();
                }
            }
        }
        else {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            
        }
        
        https.end();
    }
    return haveWeather;
}
        
