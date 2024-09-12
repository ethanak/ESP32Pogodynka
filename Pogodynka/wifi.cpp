#include <Arduino.h>
#include "Pogoda.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>

struct netPrefs currentNet;
static uint8_t mdnsOK;
static void wifiConnect()
{
    WiFi.disconnect(true);
    WiFi.begin(currentNet.ssid, currentNet.pass);
    if (!currentNet.dhcp) {
        WiFi.config(currentNet.ip, currentNet.gw, currentNet.mask,
            currentNet.ns1, currentNet.ns2);
    }
    if (currentNet.name[0]) {
        mdnsOK=MDNS.begin(currentNet.name) != 0;
    }
        
}

extern volatile uint8_t wifiShowMsg;

static void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Procure;
    wifiShowMsg=1;
    Vacate;
    if (mdnsOK) {
        mdnsOK=0;
        MDNS.end();
    }
    clockAtDisConnect();
    stopServer();
    wifiConnect();
    
}

uint8_t stationMac[6];
static void WiFi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!stationMac[0]) esp_wifi_get_mac(WIFI_IF_STA, stationMac);
    Procure;
    wifiShowMsg=2;
    Vacate;
    //Serial.printf("WiFi: połączono\n");
}

static void WiFi_gotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    //Serial.printf("WiFi: uzyskano adres IP\n");
    Procure;
    wifiShowMsg=3;
    Vacate;
    clockAtConnect();
    startServer();
    startTelnet();
    if (thrPrefs.trmex == DEVT_ESPNOW && thrPrefs.exmac[0]) {
        setStationMac(thrPrefs.exmac);
    }
    else {
        deinitEspNow();
    }
}

void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    if (!canConnect()) {
        wifiShowMsg=4;
        //Serial.printf("Brak ustawień WiFi\n");
        return;
    }
    currentNet = netPrefs;
    
//    WiFi.onEvent(Wifi_connected,SYSTEM_EVENT_STA_CONNECTED);
//    WiFi.onEvent(Got_IPAddress, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent(WiFi_connected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(Wifi_disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(WiFi_gotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    wifiConnect();
}

void WiFiGetMac(uint8_t *mac)
{
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
}

uint8_t WiFiGetIP(char *buf)
{
    if (WiFi.status() != WL_CONNECTED) return 0;
    String s=WiFi.localIP().toString();
    strcpy(buf,s.c_str());
    return 1;
}
