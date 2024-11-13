#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "Pogoda.h"

static int wifi_chan;
#define MAGIC_TEMP 0xc3

static struct espdata {
    uint8_t magic;
    uint8_t acc;
    uint8_t humm;
    uint8_t templ;
    uint8_t temph;
    uint8_t voltl;
    uint8_t volth;
    uint8_t presl;
    uint8_t presh;
} current_espdata;

uint8_t have_data = 0;

float enoTemp[2], enoHumm[2];
uint8_t enoAcc[2], enoStat[2];
uint16_t enoVolt[2],enoPres[2];
uint32_t lastEno[2] = {0,0};


extern uint8_t stationMac[];
void onDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
   // printf("received %02x\n",*data);
    struct espdata ce;

/*
    printMac(Serial,"SRC ",info->src_addr);
    printMac(Serial,"PFE ",thrPrefs.exmac);
    printMac(Serial,"PFI ",thrPrefs.inmac);
    printMac(Serial,"DST ",info->des_addr);
    printMac(Serial,"STA ",stationMac);
    printMac(Serial,"RLM ",realMac);
*/    
    int which;
    
    if (memcmp((void *)(info->des_addr),stationMac,6)) return;
    if (thrPrefs.trmex == DEVT_ESPNOW && !memcmp((void *)(info->src_addr),thrPrefs.exmac,6)) which=THID_EXT;
    else if (thrPrefs.trmin == DEVT_ESPNOW  && !memcmp((void *)(info->src_addr),thrPrefs.inmac,6)) which=THID_INTERNAL;
    else {
        return;
    }
    
    //Serial.printf("ENOW Got data %d/%d\n", len,sizeof(ce));
    if (len != sizeof(ce)) return;
    memcpy((void *)&ce, (void *)data,sizeof(ce));
    if (ce.magic != MAGIC_TEMP) return;
    int16_t temp = ce.templ | (ce.temph <<8);
    int16_t volt = ce.voltl | (ce.volth <<8);
    int16_t pres = ce.presl | (ce.presh <<8);
    if (debugMode) Serial.printf("%02d:%02d:%02d ENOW(%d) %02x %d %d %d %d\n",
        _hour, _minute, _second,which,ce.magic, ce.acc, ce.humm, temp,volt);
    uint32_t t=millis();
    Procure;
    lastEno[which] = t;
    enoStat[which] = 0;
    enoAcc[which] = ce.acc;
    enoVolt[which] = volt;
    if (enoAcc[which] != 1) {
        if (ce.humm != 0xff) {
            enoStat[which] |= THV_HGR;
            enoHumm[which] = ce.humm;
        }
        if (ce.temph != 0x7f) {
            enoTemp[which] = temp / 10.0;
            enoStat[which] |= THV_TEMP;
        }
        if (ce.presh != 0x7f) {
            enoPres[which] = pres;
            enoStat[which] |= THV_PRES;
        }
    }
    //if (enoAcc < 0xfe)
    enoStat[which] |= THV_ACU;
    Vacate;
        
}

static uint8_t espnowOK = 0;
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
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("ESP-NOW: brak połączenia\n");
        return;
    }
    if (!espnowOK) {
        if (!initEspNow()) return;
    }
    if (!PeerMac[0]) return;
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, (void *)PeerMac, 6);
    printMac(Serial,"ESP-NOW: peer ",peerInfo.peer_addr);

    peerInfo.channel = wifi_chan;
    peerInfo.ifidx = WIFI_IF_STA;
    int err = esp_now_add_peer(&peerInfo);
}
