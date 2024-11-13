
#if defined(ESP32)


#include <USB.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#else
#error Nie ta architektura
#endif
#include "Termometr.h"

uint32_t ms;
uint8_t sentOK;
uint8_t wifiset=0;
uint32_t lastActivity,lastSent,waitSeconds;
unsigned long ARDUINO_ISR_ATTR seconds() {
  return (unsigned long)(esp_timer_get_time() / 1000000ULL);
}
uint8_t sleepy;
void setup()
{
    Serial.begin(115200);
    lastActivity = lastSent = seconds();
    initPFS();
    sentOK=0;
    ms=millis()+2000UL-(uint32_t)realPrefes.sleep;
    wifiset=0;
    waitSeconds = 3;
    sleepy = (esp_reset_reason() == ESP_RST_DEEPSLEEP);
}

void gotoSleep()
{
    if (debugOn()) {
        Serial.printf("Going to sleep\n");
        delay(200);
    }
    esp_sleep_enable_timer_wakeup((uint64_t)realPrefes.sleep * 1000000ULL);
    esp_deep_sleep_start();
}

void loop()
{
    static int i = 0;
    if (seconds() - lastSent >= waitSeconds) {
        chargeMode = getPower() > 800;
        if (!wifiset) {
            initEN();
            wifiset=1;
        }
        if (debugOn()) {
            Serial.printf("Próba wysłania po %d (sent) %d (actv)\n", seconds() - lastSent, seconds() - lastActivity);
        }
        
        //Serial.printf("I=%d, JTag %d\n",i++,usb_serial_jtag_is_connected());
        sentOK = 0;
        dataSend();
        lastSent=seconds();
        if (sleepy) waitSeconds = realPrefes.sleep;
    }
    
    sendLoop();
    if (sentOK) {
        if (!sleepy && seconds() - lastActivity < 30) waitSeconds = 5;
        else waitSeconds = realPrefes.sleep;
        if (debugOn()) {
                Serial.printf("SentOK %d waitSeconds %d chargeMode %d keepAlive %d\n", sentOK, waitSeconds, chargeMode,(realPrefes.flags & PFS_KEEPALIVE));
        }
        sentOK=0;
        if (!chargeMode && // nigdy nie śpimy w chargeMode
            !(realPrefes.flags & PFS_KEEPALIVE) && // tu nam zabraniają
            waitSeconds > 5) { // kontynuacja działania
                waitSeconds = 3; // na później
                gotoSleep();
            
        }
    }
    serLoop();
}
