#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Pogoda.h"

static int wifi_chan;
#define MAGIC_TEMP 0xc2

static struct espdata {
    uint8_t magic;
    uint8_t acc;
    uint8_t humm;
    uint8_t templ;
    uint8_t temph;
    uint8_t voltl;
    uint8_t volth;
} current_espdata;

uint8_t have_data = 0;

float enoTemp, enoHumm;
uint8_t enoAcc, enoStat;
uint16_t enoVolt;
uint32_t lastEno = 0;


extern uint8_t stationMac[];
void onDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
   // printf("received %02x\n",*data);
    struct espdata ce;
    //printMac(Serial,"SRC ",info->src_addr);
    //printMac(Serial,"PFS ",thrPrefs.exmac);
    //printMac(Serial,"DST ",info->des_addr);
    //printMac(Serial,"STA ",stationMac);
    
    if (memcmp((void *)(info->src_addr),thrPrefs.exmac,6)) return;
    if (memcmp((void *)(info->des_addr),stationMac,6)) return;
    
    //Serial.printf("ENOW Got data %d/%d\n", len,sizeof(ce));
    if (len != sizeof(ce)) return;
    memcpy((void *)&ce, (void *)data,sizeof(ce));
    if (ce.magic != MAGIC_TEMP) return;
    int16_t temp = ce.templ | (ce.temph <<8);
    int16_t volt = ce.voltl | (ce.volth <<8);
    if (debugMode) Serial.printf("ENOW %02x %d %d %d %d\n",ce.magic, ce.acc, ce.humm, temp,volt);
    uint32_t t=millis();
    Procure;
    lastEno = t;
    enoStat = 0;
    enoAcc = ce.acc;
    enoVolt = volt;
    if (enoAcc != 1) {
        if (ce.humm != 0xff) {
            enoStat |= 2;
            enoHumm = ce.humm;
        }
        if (ce.temph != 0x7f) {
            enoTemp = temp / 10.0;
            enoStat |= 1;
        }
    }
    //if (enoAcc < 0xfe)
    enoStat |= 4;
    Vacate;
        
}

static uint8_t espnowOK = 0, havePeerMac = 0;
static uint8_t PeerMac[6];
//64:E8:33:8B:E5:5C


void deinitEspNow()
{
    if (!espnowOK) return;
    espnowOK=0;
    esp_now_unregister_recv_cb();
    esp_now_deinit();
}

uint8_t initEspNow()
{
    if (WiFi.status() != WL_CONNECTED) return 0;
    deinitEspNow();
    wifi_chan=WiFi.channel();
    if (debugMode) Serial.printf("Current wifi channel %d\n",wifi_chan);
    esp_now_init();
    esp_now_register_recv_cb(onDataReceive);
    espnowOK=1;
    return 1;
}

void setStationMac(uint8_t *newPeerMac)
{
    //Serial.printf("ENOW start\n");
    memcpy((void *)PeerMac, (void *)newPeerMac,6);
    havePeerMac=1;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("ESP-NOW: brak połączenia\n");
        return;
    }
    if (!initEspNow()) return;
    if (!PeerMac[0]) return;
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, (void *)PeerMac, 6);
    printMac(Serial,"ESP-NOW: peer ",peerInfo.peer_addr);

    peerInfo.channel = wifi_chan;
    peerInfo.ifidx = WIFI_IF_STA;
    int err = esp_now_add_peer(&peerInfo);
}
