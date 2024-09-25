#include <Arduino.h>
#include "Termometr.h"
#include <esp_wifi.h>
const char *getToken(char **line)
{
    while (**line && isspace(**line)) (*line)++;
    if (!**line) return NULL;
    char *rc=*line;
    while (**line && !isspace(**line)) (*line)++;
    if (**line) *(*line)++=0;
    return rc;
}

static char cmdbuf[80];
static int cmdbpos=0;
static int cmdblank=0;

static void pfsHelp(char *s);
static void showLocMac(char *s)
{
    uint8_t mac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    printMac("adres MAC",mac);
}

static void pfsReset(char *s)
{
    ESP.restart();
}


static const struct serCommand {
    const char *name;
    const char *shdes;
    const char *ldes;
    void (*fun)(char *);
} serCmds[] ={
    {"help","Pomoc","Parametr - polecenie którego dotyczy pomoc",pfsHelp},
    {"mymac","Pokazuje adres MAC urządzenia",NULL, showLocMac},
    {"sensor","Typ podłączonego czujnika",
        "Bez parametru pokazuje typ\n    Parametr none, ds, dht11, dht22 lub bmp ustawia typ\n",pfsTerm},
    {"pin","Ustawienie pinów termomentu i i2c",
        "Bez parametru pokazuje aktualne piny.\n\
    Z parametrem t, scl lub sda pokazuje ustawiony pin termometru lub linii i2c\n\
    Jeśli drugim parametrem będzie liczba, ustawia odpowiedni pin",pfsPin},
    {"peer","Adres głównego urządzenia",
        "Bez parametru pokazuje aktualny adres\nSześć dwucyfrowych liczb sesnastkowych (adres MAC)\n\
    Liczby mogą być oddzielane dowolnymi znakami",pfsPeer},
    {"sleep","Czas uśpienia między pomiarami","Bez parametru - pokazuje czas.\n\
    Liczba z zakresu 30 do 600 - czas między wybudzeniami w sekundach",pfsSleep},
    {"accalib","Kalibracja pomiaru napięcia akumulatora",
        "Bez parametru podaje aktualne ustawienia\n\
    \"start\" rozpoczęcie kalibracji. Drugim parametrem może być\n\
        aktualna zmierzona wartość napięcia (domyślnie 4200mV).\n\
    \"off\" - wyłączenie odczytu akumulatora",pfsCalib},
    {"charger","Czy urządzenie wykrywa podłączenie do ładowania","t - tak, n - nie\n\
    Bez parametru podahe stan",pfsCharger},
    {"keep","Urządzenie nie przechodzi w tryb uśpienia",
            "Bez parametru pokazuje aktualny stan\n\
    Parametr n: jest usypiane, t: działa non stop",pfsKeep},
    {"reset","Reset urządzenia",NULL,pfsReset},
    {"save","Zapisanie do pamięci flash i restart",NULL,pfsSave},
    
    
    {NULL,NULL,NULL,NULL}};

static void pfsHelp(char *s)
{
    int i;
    if (!s || !*s) {
        Serial.printf("Wpisz polecenie help nazwa, gdzie nazwa to:\n");
        for (i=0;serCmds[i].name;i++) {
            Serial.printf("%s: %s\n",serCmds[i].name,serCmds[i].shdes);
        }
        return;
    }
    for (i=0;serCmds[i].name;i++) if (!strcmp(s,serCmds[i].name)) {
        Serial.printf("%s: %s\n    %s\n",
        serCmds[i].name,serCmds[i].shdes,
        serCmds[i].ldes ? serCmds[i].ldes : "Polecenie nie przyjmuje parametrów");
        return; 
    }
    Serial.printf("Nieznane polecenie '%s\n'",s);  
}
static void doSerialCmd(char *str)
{
    const char *cmd=getToken(&str);
    int i;
    for (i=0;serCmds[i].name;i++) if (!strcmp(cmd, serCmds[i].name)) break;
    if (!serCmds[i].name) {
        Serial.printf("Nieznane polecenie %s\n",cmd);
        return;
    }
    if (serCmds[i].fun) serCmds[i].fun(str);
}

void serLoop()
{
    while (Serial.available()) {
        lastActivity = seconds();
        int z = Serial.read();
        if (z == '\r') continue;
        if (z == '\n') {
            if (!cmdbpos) continue;
            cmdbuf[cmdbpos]=0;
            doSerialCmd(cmdbuf);
            cmdbpos = 0;
            cmdblank=0;
            continue;
        }
        if (isspace(z)) {
            if (cmdbpos) cmdblank=1;
            continue;
        }
        if (cmdbpos < 78) {
            if (cmdblank) {
                cmdbuf[cmdbpos++]=' ';
                cmdblank = 0;
            }
            cmdbuf[cmdbpos++]=z;
        }
    }
}

