#include <Arduino.h>

#ifdef  CONFIG_IDF_TARGET_ESP32S3
#include "Pogoda.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

extern WiFiClientSecure secClient;
extern void initRequest();
static const char *otaHost = "milena.polip.com";
static const char *otaPath = "/podynka/bin/";

static void mkOtaPath(char *buf, const char *fname)
{
    sprintf(buf,"https://%s%s%s", otaHost, otaPath,fname);

}

static int otaVersion[3]={0,0,0};
static void mkVersion(const char *s, int *ota)
{
    ota[0]=ota[1]=ota[2]=0;
    while (*s && !isdigit(*s)) s++;
    if (!*s) return;
    ota[0] = strtol(s,(char **)&s, 10);
    if (*s++ != '.') return;
    ota[1] = strtol(s,(char **)&s, 10);
    if (*s++ != '.') return;
    ota[2] = strtol(s,(char **)&s, 10);
}
    
static uint8_t otaGetVersion(Print &Ser)
{
    char url[80];
    HTTPClient https;
    initRequest();
    mkOtaPath(url,"version.txt");
    //Ser.printf("URL <%s>\n",url);
    int rc=0;
    if (https.begin(secClient, url)) {
        https.setTimeout(750);
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode != 200) {
                Ser.printf("HTTP kod %d\n",httpCode);
            }
            else {
                String payload = https.getString();
                mkVersion(payload.c_str(),otaVersion);
                rc=1;
            }
        }
        else {
            Ser.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
    }
    return rc;
}

static int locver[3];
uint8_t otaPerform(Print &Ser, char *par)
{
    if (!otaGetVersion(Ser)) return 0;
    mkVersion(firmware_version,locver);
    Ser.printf("Zainstalowana wersja %d.%d.%d, dostępna %d.%d.%d\n",
        locver[0], locver[1], locver[2],
        otaVersion[0], otaVersion[1], otaVersion[2]);
    bool ohay = otaVersion[0] > locver[0] ||
        (otaVersion[0] == locver[0] && otaVersion[1] > locver[1] ||
        (otaVersion[1] == locver[1] && otaVersion[2] > locver[2]));
    if (ohay) Ser.printf("Możliwy update\n");
    if (!par || !*par || !strcmp(par,"check")) return 1;
    const char *tok = getToken(&par);
    if (strcmp(tok,"update")) {
        Ser.printf("Błędny parametr\n");
        return 0;
    }
    if (*par) {
        if (strcmp(par,"force")) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        else ohay = true;
    }
    if (!ohay) {
        Ser.printf("Nie będzie aktualizacji\n");
        return 0;
    }
    Ser.printf("Rozpoczynam aktualizację\n");
    runUpdateOta(Ser);
    return 1;
}

static void update_progress(int cur, int total) {
    sayCondPercent((cur * 100) / total);
    //Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

int checkOta()
{
    if (!otaGetVersion(Serial)) return -1;
    mkVersion(firmware_version,locver);
    bool ohay = otaVersion[0] > locver[0] ||
        (otaVersion[0] == locver[0] && otaVersion[1] > locver[1] ||
        (otaVersion[1] == locver[1] && otaVersion[2] > locver[2]));
    if (!ohay) return -2;
    return otaVersion[2] | (otaVersion[1] << 8) | (otaVersion[0] << 16);
}

int runUpdateOta(Print &Ser)
{
    sayCWait("Rozpoczynam instalację");
    char buf[80];
    mkOtaPath(buf,"smbrox3.bin");
    httpUpdate.rebootOnUpdate(false);
    sayCondPercent(-1);
    httpUpdate.onProgress(update_progress);
    t_httpUpdate_return ret = httpUpdate.update(secClient, buf);
    
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Ser.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
            httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            sayCWait("Błąd pobierania aktualizacji");
        break;
 
      case HTTP_UPDATE_NO_UPDATES:
        Ser.println("HTTP_UPDATE_NO_UPDATES");
        Ser.println("Your code is up to date!");
        sayCWait("Brak aktualizacji");
        break;
 
      case HTTP_UPDATE_OK:
      sayCWait("Instalacja zakończona");
      ESP.restart();
        break;
    }
    return 0;
}


#endif
