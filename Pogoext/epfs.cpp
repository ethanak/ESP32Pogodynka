#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <Preferences.h>
#include "Termometr.h"

const uint8_t availPins[]={ AVAILPINS ,0};
Preferences prefs;
struct prefes prefes;           // robocze do ustawiania
struct prefes realPrefes; // rzeczywiste


//static uint8_t stationMac[6]={0xAC,0x67,0xB2,0x36,0xE2,0x58};

void printMac (const char *pfx,const uint8_t *mac)
{
    Serial.printf("%s %02X:%02X:%02X:%02X:%02X:%02X\n",pfx,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}



static uint8_t RTC_DATA_ATTR last_channel;
static uint8_t RTC_DATA_ATTR debugMode;

bool debugOn()
{
    return debugMode == 1;
}

static uint8_t dataSentSucc;

void onDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) dataSentSucc = 1;
    //Serial.printf(status == ESP_NOW_SEND_SUCCESS ? "Dane odebrane przez slave\r\n" : "Slave nie odebrał danych\r\n");
}




static uint8_t senten[9];
static uint16_t realVolt;
bool chargeMode = false;
uint16_t getPower()
{
    int i;
    uint32_t pwr;
    if (prefes.flags & PFS_HAVE_CHARGER) {
        for (i=0,pwr=0; i<16; i++) pwr += analogReadMilliVolts(A1);
        return pwr/16;
    }
#ifdef USE_USB_TEST
    return usb_serial_jtag_is_connected()?1500:0;
#endif
    return 0;
}

uint8_t getAccu()
{
    int i;uint32_t ami;
    if (!prefes.v42) realVolt = 0; // nie ma pomiaru stanu akumulatora
    else {
        for (i=0,ami=0; i<16; i++) ami += analogReadMilliVolts(A0);
        ami /= 16;
        if (ami > prefes.v42) ami = prefes.v42;
        ami = ((ami * 4200) / prefes.v42);
        realVolt = ami;
    }
    if (chargeMode) return 1;
    if (!prefes.v42) return 0;
    
    if (ami >= 3600) return 2; // akumulator naładowany
    if (ami >= 3400) return 3; // akumulator częściowo eozładowany
    return 4; // akumulator wymaga ładowania
}
static uint8_t havermac = 0;
uint8_t realMac[6];
void initEN()
{
    if (last_channel <= 0 || last_channel >= 13) {
        last_channel = 1;
    }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_now_deinit();
    
    if (!havermac) {
        esp_wifi_get_mac(WIFI_IF_STA, realMac);
        havermac=1;
    }
    if (realPrefes.flags & PFS_FAKEMAC) {
        WiFi.STA.begin();
        esp_err_t ret = esp_wifi_set_mac(WIFI_IF_STA,realPrefes.fakemac);
        if (ret) Serial.printf("Błąd ustawienia MAC: %s\n",esp_err_to_name(ret));
        WiFi.STA.begin();
    }
    esp_wifi_set_channel(last_channel,(wifi_second_chan_t)0);
    esp_now_init();
    esp_now_register_send_cb(onDataSent);
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, (void *)realPrefes.station, 6);
    peerInfo.channel =last_channel;
    peerInfo.ifidx = WIFI_IF_STA;
    int err = esp_now_add_peer(&peerInfo);
}

uint32_t sentTime;
uint8_t sentCtl;
uint8_t sentChn;
uint8_t sendStatus;
enum {
    SES_RELAX = 0,
    SES_START,
    SES_SENT,
    SES_OFF
};
    
void dataSend()
{
    if (sendStatus) return;
    senten[0] = 0xc3;
    senten[1] = getAccu();
    senten[5] = realVolt & 255;
    senten[6] = realVolt >> 8;
    int rc=readTemp();
    //Serial.printf("RV %d %f %f\n",rc,tempOut, hgrOut);
    if (rc & 1) {
        int16_t t = round(tempOut * 10);
        senten[4] = ((uint16_t) t) >> 8;
        senten[3] = ((uint16_t) t) & 255;
    }
    else {
        senten[3] = senten[4] = 0x7f;
    }
    if (rc & 2) {
        senten[2]=hgrOut;
    }
    else {
        senten[2] = 0xff;
    }
    if (rc & 4) {
        int16_t t = round(presOut);
        senten[8] = ((uint16_t) t) >> 8;
        senten[7] = ((uint16_t) t) & 255;
    }
    else {
        senten[7] = senten[8] = 0x7f;
    }
    sendStatus = SES_START;
}

static void printSent()
{
    if (!debugOn()) return;
    char buf[80];
    char *c =buf;
    c +=sprintf(c,"Wysłano: CA=%d, V=%d, H=", senten[1],
        senten[5] | (senten[6] << 8));
    if (senten[2] == 0xff) c+=sprintf(c,"None");
    else c+= sprintf(c,"%d%%",senten[2]);
    if (senten[3] == 0x7f && senten[4] == 0x7f) c+= sprintf(c,", T=None");
    else {
        int16_t t = senten[3] | (senten[4] << 8);
        c+=sprintf(c,", T=%d d°C",t);
    }
    if (senten[7] == 0x7f && senten[8] == 0x7f) c+= sprintf(c,", P=None");
    else {
        uint16_t t = senten[7] | (senten[8] << 8);
        c+=sprintf(c,", P=%d hPa",t);
    }
    Serial.printf("%s\n",buf);
}

void realSend()
{
    esp_now_send(realPrefes.station,senten,9);
}

void trySend()
{
    realSend();
    sentTime = millis();
    sentCtl++;
}



void sendLoop()
{
    if (!realPrefes.station[0]) return;
    switch(sendStatus) {
        case SES_OFF:
        sendStatus = SES_RELAX;
        break;
        
        case SES_START:
        sentCtl = 0;
        sentChn = 0;
        dataSentSucc = 0;
        trySend();
        sendStatus = SES_SENT;
        break;
        
        case SES_SENT:
        if (millis() - sentTime < 10) break;
        if (dataSentSucc) {
            printSent();
            sendStatus = 0;
            sentOK=1;
            break;
        }
        if (sentCtl < 3) {
            if (millis() - sentTime < 50) break;
            trySend();
            break;
        }
        sentChn++;
        if (sentChn >= 13) {
            if (debugOn()) Serial.printf("Nie mogę znaleźć kanału!\n");
        //    Serial.printf("Dupa\n");
            sendStatus = 0;
            sentOK=2;
            break;
        }
        delay(50);
        last_channel++;
        if (last_channel >= 13) last_channel = 1;
        //Serial.printf("Changing channel to %d\n", last_channel);
        initEN();
        sentCtl = 0;
        trySend();
        break;
    }
}

/* preferences */

void initPFS()
{
    prefs.begin("termol");
    if (!prefs.getBytes("pfs",(void *)&prefes,sizeof(struct prefes)) ||
        prefes.magic != MAGIC_EE) {
            memset((void *)&prefes,0,sizeof(struct prefes));
            prefes.magic = MAGIC_EE;
            prefes.flags = PFS_KEEPALIVE;
            prefs.clear();
            prefs.putBytes("pfs",(void *)&prefes,sizeof(struct prefes));
    }
    if (prefes.sleep < 30 || prefes.sleep > 600) prefes.sleep=300;
    if (prefes.termtyp >= THR_MAX) prefes.termtyp = 0;
    realPrefes=prefes;
}

void resetFPS()
{
    prefes = realPrefes;
}

void commitFPS()
{
    prefs.putBytes("pfs",(void *)&prefes,sizeof(struct prefes));
    realPrefes=prefes;
    
}



void pfsSleep(char *s)
{
    if (s && *s) {
        int n = strtol(s, &s, 10);
        if (*s || n<30 || n > 600) {
            Serial.printf("Błędny parametr\n");
            return;
        }
        prefes.sleep = n;
    }
    Serial.printf("Czas uśpienia %d sekund\n", prefes.sleep);
}

void pfsCalib(char *s)
{
    if (s && *s) {
        const char *c=getToken(&s);
        if (!strcmp(c,"start")) {
            uint32_t vol=0;
            uint8_t err=0;
            uint32_t v0=4200;
            if (*s) {
                c=getToken(&s);
                if (isdigit(*c)) {
                    v0=strtol(c,(char **)&c,10);
                    if (*c) err=1;
                    else if (v0 < 3000 || v0 > 6500) {
                        Serial.printf("Błędna wartość napięcia kalibracji (3000 do 6500 mV)\n");
                        return;
                    }
                }
                else err=1;
                if (*s || err) {
                    Serial.printf("Błędny parametr\n");
                    return;
                }
            }
                
            int i;
            for (i=0;i<64;i++) vol += analogReadMilliVolts(A0);
            prefes.v42 = (vol * 4200)/(64* v0);
        }
        else if (!strcmp(c,"off")) prefes.v42=0;
        else {
            Serial.printf("Błędny parametr\n");
            return;
        }
    }
    if (prefes.v42 == 0) {
        Serial.printf("Odczyt akumulatora wyłączony\n");
    }
    else {
        Serial.printf("Wartość kalibracji dla 4.2V: %d\n", prefes.v42);
    }
}

static const char *const ttyp[]={
    "none","ds","dht11","dht22","bmp"};
static const char *const ttypf[]={
    "Brak","DS28B20","DHT11","DHT22","BME/BMP280"};
    
void pfsTerm(char *s)
{
    int i;
        
    if (s && *s) {
        for (i=0;i<THR_MAX;i++) if (!strcmp(ttyp[i],s)) break;
        if (i >= THR_MAX) {
            Serial.printf("Błędny parametr\n");
            return;
        }
        prefes.termtyp = i;
    }
    Serial.printf("Typ czujnika: %s\n",ttypf[prefes.termtyp]);
    if (prefes.termtyp == THR_DS) {
        extern uint8_t owcount, owid[];
        if (!owcount) Serial.printf("Czujnik niepodłączony\n");
        else {
            Serial.printf("Adres czujnika:");
            for (i=0;i<8;i++) Serial.printf(" %02X",owid[i]);
            Serial.printf("\n");
        }
    }
    
}

uint8_t macFromPar(char *s,uint8_t *adr)
{
    int i;uint8_t mac[6];
    for (i=0;i<6;i++) {
        if (i) {
            while (*s && !isxdigit(*s)) s++;
        }
        if (!*s || !s[1] || !isxdigit(*s) || !isxdigit(s[1])) break;
        if (s[2] && isxdigit(s[2])) break;
        mac[i]=strtol(s,&s, 16);
        if (!i && !mac[i]) break;
    }
    if (i < 6 || *s || (mac[0] & 1)) {
        return 0;
    }
    memcpy((void *)adr, (void *)mac, 6);
    return 1;
}

void pfsFakeMac(char *s)
{

    if (s && *s) {
        if (!strcmp(s,"on")) prefes.flags |= PFS_FAKEMAC;
        else if (!strcmp(s,"off")) prefes.flags &= ~PFS_FAKEMAC;
        else {
            const char *c=getToken(&s);
            if (strcmp(c,"set")) {
                Serial.printf("Nieznany parametr %s\n", c);
                return;
            }
            if (!macFromPar(s, prefes.fakemac)) {
                Serial.printf("Błędny adres MAC\n");
                return;
            }
        }
    }
    printMac(
               "Oryginalny MAC        ", realMac);
    Serial.printf("Podmieniony MAC ");
    printMac(
        (prefes.flags & PFS_FAKEMAC) ? "(ON)  ": "(OFF) ",
        prefes.fakemac);
    
}

const char *const pinames[] ={
    "t","scl","sda"
};
const char *const pindisp[] ={
    "Termo","SCL","SDA"
};

void printpin(int i)
{
    int n=(i==0) ? prefes.pin: (i==1) ? prefes.sclpin : prefes.sdapin;
    if (!n) {
        Serial.printf("%s nieustawiony\n", pindisp[i]);
    }
    else {
        Serial.printf("%s: %d\n", pindisp[i],n);
    }
}

void pfsPin(char *s)
{
    int np;
    if (s && *s) {
        const char *cmd=getToken(&s);
        for (np=0;np<3;np++) if (!strcmp(pinames[np],cmd)) break;
        if (np >= 3) {
            Serial.printf("Błędny pin %s\n", cmd);
            return;
        }
        if (!*s) {
            printpin(np);
            return;
        }
        int p;
        if (!strcmp(s,"off")) p = 0;
        else {
            p = strtol(s,&s,10);
            if (p <0 || *s) {
                Serial.printf("Błędny parametr\n");
                return;
            }
        }
        int i;
        if (p) {
            for (i=0;availPins[i];i++) if (p == availPins[i]) break;
            if (!availPins[i]) {
                Serial.printf("Błąd, dopuszczalne piny to: %d",availPins[0]);
                for (i=1;availPins[i];i++) Serial.printf(", %d",availPins[i]);
                Serial.printf(".\n");
                return;
            }
        }
        if (np == 0) prefes.pin = p;
        else if (np == 1) prefes.sclpin = p;
        else prefes.sdapin = p;
        Serial.printf("Ustawiono ");printpin(np);
        return;
    }
    for (np=0; np<3;np++) printpin(np);
}


void pfsSave(char *s)
{
    if (
        (prefes.pin && ((prefes.pin == prefes.sclpin) || (prefes.pin == prefes.sdapin)))
        ||
        (prefes.sclpin && prefes.sclpin == prefes.sdapin)) {
            Serial.printf("Błędna kombinacja pinów\n");
            return;
    }
    if (prefes.termtyp == THR_BMP280 && (prefes.sclpin == 0 || prefes.sdapin == 0)) {
        Serial.printf("Ustaw piny SCL i SDA dla termometru BMP280/BME280\n");
        return;
    }
    if (prefes.termtyp && prefes.termtyp < THR_BMP280 && !prefes.pin) {
        Serial.printf("Ten typ termometru wymaga ustawienia pinu\n");
        return;
    }
    commitFPS();
    delay(100);
    ESP.restart();
}

void pfsPeer(char *s)
{
    int i; uint8_t mac[6],mbc[6];
    if (s && *s) {
        if (!macFromPar(s, mac)) {
            Serial.printf("Błędny adres MAC\n");
            return;
        }
        printMac("GVot mac", mac);
    
        esp_wifi_get_mac(WIFI_IF_STA, mbc);
        if (!memcmp((void *)mac, (void *)mbc, 6)) {
            Serial.printf("To jest adres czujnika, podaj adres stacji\n");
            return;
        }
        memcpy((void *)prefes.station,(void *)mac,6);
    }
    printMac ("Adress stacji: ",prefes.station);  
}

void pfsKeep(char *s)
{
    if (s && *s) {
        if (!strcasecmp(s,"n")) prefes.flags &= ~ PFS_KEEPALIVE;
        else if (!strcasecmp(s,"t")) prefes.flags |= PFS_KEEPALIVE;
        else {
            Serial.printf("Błędny parametr\n");
            return;
        }
    }
    Serial.printf("Urządzenie %s\n", (prefes.flags & PFS_KEEPALIVE) ? "działa cały czas" : "może być usypiane");
}

void pfsCharger(char *s)
{
    if (s && *s) {
        if (tolower(*s) == 't') prefes.flags |= PFS_HAVE_CHARGER;
        else if (tolower(*s) == 'n') prefes.flags &= ~PFS_HAVE_CHARGER;
        else {
            Serial.printf("Błędny parametr\n");
            return;
        }
    }
    Serial.printf("Urządzenie %s do ładowania\n",
        (prefes.flags & PFS_HAVE_CHARGER)? "wykrywa podłączenie" : "nie wykrywa podłączenia");
        
}

void pfsDebug(char *s)
{
    if (s && *s) {
        if (!strcmp(s,"on")) debugMode = 1;
        else if (!strcmp(s,"off")) debugMode = 0;
        else {
            Serial.printf("Błędny parametr\n");
            return;
        }
    }
    Serial.printf("Stan debug: %słączony\r\n",debugMode ?"za":"wy");
}
