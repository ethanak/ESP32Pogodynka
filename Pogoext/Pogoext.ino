
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
uint8_t slp;
void setup()
{
    Serial.begin(115200);
    lastActivity = lastSent = seconds();
    initPFS();
    sentOK=0;
    ms=millis()+2000UL-(uint32_t)realPrefes.sleep;
    wifiset=0;
    waitSeconds = 3;
    slp = (esp_reset_reason() == ESP_RST_DEEPSLEEP);
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
        Serial.printf("Sent after %d %d\n", seconds() - lastSent, seconds() - lastActivity);
        
        //Serial.printf("I=%d, JTag %d\n",i++,usb_serial_jtag_is_connected());
        sentOK = 0;
        dataSend();
        lastSent=seconds();
        if (waitSeconds < 5) waitSeconds = 5;
    }
    
    sendLoop();
    if (sentOK) {
        sentOK=0;
        //if (!usb_serial_jtag_is_connected() && !(realPrefes.flags & PFS_KEEPALIVE)) {
        if (waitSeconds > 5 && !chargeMode && !(realPrefes.flags & PFS_KEEPALIVE)) {
            waitSeconds = 3;
            esp_sleep_enable_timer_wakeup((uint64_t)realPrefes.sleep * 1000000ULL);
            esp_deep_sleep_start();
        }
        else {
            if (seconds() - lastActivity < 30) waitSeconds = 5;
            else waitSeconds = realPrefes.sleep;
        }
    }
    serLoop();
}
