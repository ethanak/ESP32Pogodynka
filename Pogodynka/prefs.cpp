#include <Arduino.h>
#include <Preferences.h>
#include "Pogoda.h"
#include <ESP32Gadacz.h>
#include <ctype.h>

#define prv_htonl(x)					\
  ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8)	\
   | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))

struct netPrefs netPrefs;
struct spkPrefs spkPrefs;
struct clkPrefs clkPrefs;
struct geoPrefs geoPrefs;
struct thrPrefs thrPrefs;
struct weaPrefs weaPrefs;
struct irrPrefs irrPrefs;
struct calPrefs calPrefs;
#ifdef  CONFIG_IDF_TARGET_ESP32S3
struct pinPrefs pinPrefs,pinPrefsW;
#endif

static Preferences prefs;

void LockPrefs()
{
    xSemaphoreTake(prefSemaphore, portMAX_DELAY);
}

void UnlockPrefs()
{
    xSemaphoreGive(prefSemaphore);
}

void loadGeo()
{
    LockPrefs();
    if (!prefs.getBytes("geo",(void *)&geoPrefs,sizeof(geoPrefs))) {
        geoPrefs.elev = ELEV_AUTO;
        geoPrefs.lon= 21 * 3600;
        geoPrefs.lat= 52 * 3600 + 15*60;
        geoPrefs.autoelev = ELEV_AUTO;
    }
    haveWeather = 0;
    UnlockPrefs();
}

void loadCal()
{
    LockPrefs();
    if (!prefs.getBytes("cal",(void *)&calPrefs, sizeof(calPrefs))) {
        memset((void *)&calPrefs, 0, sizeof(calPrefs));
    }
    UnlockPrefs();
}

void loadClk()
{
    LockPrefs();
    if (!prefs.getBytes("clk",(void *)&clkPrefs,sizeof(clkPrefs))) {
        clkPrefs.offset = 60;
        clkPrefs.useDST=1;
    }
    UnlockPrefs();
}

void loadGadacz()
{
    LockPrefs();
    if (!prefs.getBytes("spk",(void *)&spkPrefs,sizeof(spkPrefs))) {
        spkPrefs.contrast=0;
        spkPrefs.pitch=12;
        spkPrefs.tempo=7;
        spkPrefs.volume=12;
    }
    UnlockPrefs();
}

void loadWea()
{
    LockPrefs();
    if (!prefs.getBytes("wea",(void *)&weaPrefs,sizeof(weaPrefs))) {
        weaPrefs.flags = 0xff;
        weaPrefs.minpre=25;
        weaPrefs.splithour = 14;
        strcpy(weaPrefs.inname,"wewnątrz");
        strcpy(weaPrefs.outname,"na zewnątrz");
    }
    UnlockPrefs();
}

void storeGeo()
{
    LockPrefs();
    prefs.putBytes("geo",(void *)&geoPrefs,sizeof(geoPrefs));
    haveWeather = 0;
    UnlockPrefs();
}

void storeClk()
{
    LockPrefs();
    prefs.putBytes("clk",(void *)&clkPrefs,sizeof(clkPrefs));
    UnlockPrefs();
}

void storeCal()
{
    LockPrefs();
    prefs.putBytes("cal",(void *)&calPrefs,sizeof(calPrefs));
    UnlockPrefs();
}

void storeGadacz()
{
    LockPrefs();
    prefs.putBytes("spk",(void *)&spkPrefs,sizeof(spkPrefs));
    UnlockPrefs();
}

void storeWea()
{
    LockPrefs();
    prefs.putBytes("wea",(void *)&weaPrefs,sizeof(weaPrefs));
    UnlockPrefs();
}

void storeNet()
{
    LockPrefs();
    prefs.putBytes("net",(void *)&netPrefs,sizeof(netPrefs));
    UnlockPrefs();
}

#ifdef  CONFIG_IDF_TARGET_ESP32S3
static void setDefaultPins()
{
    pinPrefs.scl=defPIN_SCL;
    pinPrefs.sda=defPIN_SDA;
    pinPrefs.dht=defPIN_DHT;
    pinPrefs.ds =defPIN_ONEWIRE;
    pinPrefs.ir =defPIN_IR_RX;
    pinPrefs.btn=defPIN_BUTTON;
    pinPrefs.btng=defPIN_BUTTON_GND;
    pinPrefs.wclk=defWCLK_PIN;
    pinPrefs.bclk=defBCLK_PIN;
    pinPrefs.dout=defDOUT_PIN;
}
#endif
void initPrefs()
{
    prefs.begin("pogodynka");
#ifdef  CONFIG_IDF_TARGET_ESP32S3
    if (!prefs.getBytes("pin",(void *)&pinPrefs,sizeof(pinPrefs))) {
        setDefaultPins();
        prefs.putBytes("pin",(void *)&pinPrefs,sizeof(pinPrefs));
    }
    pinPrefsW=pinPrefs;
    
        
#endif
    if (!prefs.getBytes("net",(void *)&netPrefs,sizeof(netPrefs))) {
        memset((void *)&netPrefs,0,sizeof(netPrefs));
    }
    loadGadacz();
    loadClk();
    loadGeo();
    loadWea();
    loadCal();
    if (!prefs.getBytes("hrd",(void *)&thrPrefs,sizeof(thrPrefs))) {
        memset((void *)&thrPrefs,0,sizeof(thrPrefs));
        
    }
    if (thrPrefs.espwait < 5 ||  thrPrefs.espwait > 25) thrPrefs.espwait = 15;
    if (!prefs.getBytes("irr",(void *)&irrPrefs,sizeof(irrPrefs))) {
        memset((void *)&irrPrefs,0,sizeof(irrPrefs));
    }
}

bool canConnect()
{
    if (!netPrefs.ssid[0] || !netPrefs.pass[0]) return false;
    if (netPrefs.dhcp) return true;
    if (!netPrefs.ip || !netPrefs.gw || !netPrefs.mask || !netPrefs.ns1) return false;
    return true;
}




uint8_t pfsSsid(Print &Ser, char *par)
{
    if (!par || !*par) {
        if (!netPrefs.ssid[0]) Ser.printf("Nazwa nie jest ustawiona\n");
        else Ser.printf("%s\n",netPrefs.ssid);
        return 1;
    }
    int t=strlen(par);
    if (t < 2 || t > 63) {
        Ser.printf("Nieprawidłowa nazwa\n");
        return 0;
    }
    strcpy(netPrefs.ssid,par);
    Ser.printf("Ustawiono nazwę \"%s\"\n",par);
    return 1;
}

uint8_t pfsPass(Print &Ser, char *par)
{
    if (!par || !*par) {
        if (!netPrefs.pass[0]) Ser.printf("Hasło nie jest ustawiona\n");
        else Ser.printf("%s\n",netPrefs.pass);
        return 1;
    }
    int t=strlen(par);
    if (t < 2 || t > 63 || strchr(par,' ')) {
        Ser.printf("Nieprawidłowe hasło\n");
        return 0;
    }
    strcpy(netPrefs.pass,par);
    Ser.printf("Ustawiono hasło \"%s\"\n",par);
    return 1;
}

static int check2(Print &Ser,const char *str, const char *sf, const char *st)
{
    if (str[1]) {
        Ser.printf("Parametr musi być jednym znakiem\n");
        return -1;
    }
    if (strchr(sf,*str)) return 0;
    if (strchr(st,*str)) return 1;
    Ser.printf("Błędny parametr\n");
    return -1;
}

uint8_t pfsDhcp(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Konfiguracja IP: %s\n",netPrefs.dhcp ? "automatyczna":"ręczna");
        return 1;
    }
    int t=check2(Ser,par,"rR","aA");
    if (t < 0) return 0;
    netPrefs.dhcp=t;
    Ser.printf("Ustawiona konfiguracja IP: %s\n",netPrefs.dhcp ? "automatyczna":"ręczna");
    return 1;
}

static uint8_t pfsIPADR(Print &Ser, char *par,uint32_t *whereto,
    const char *label, uint8_t prefix=0)
{
    if (!par || !*par) {
        if (!*whereto) Ser.printf("nieustawiony\n");
        else Ser.printf("%s\n",IPAddress(*whereto).toString().c_str());
        return 1;
    }
    if (prefix == 2 && !strcmp(par,"0")) {
        *whereto = 0;
        Ser.printf("nieustawiony\n");
        return 1;
    }
        
    if (prefix == 1) {
        char *c;
        int pfx;
        pfx=strtol(par,&c,10);
        if (!*c) {
            if (pfx < 8 || pfx > 32) {
                Ser.printf("Błędny prefiks\n");
                return 0;
            }
            
            //*whereto=htonl(0xffffffff << (32-pfx));
            *whereto=prv_htonl(0xffffffff << (32-pfx));
            Ser.printf("Ustawiono %s\n",IPAddress(*whereto).toString().c_str());
            return 1;
        }
    }
    IPAddress mip;
    mip.fromString(par);
    *whereto=(uint32_t)(mip);
    Ser.printf("Ustawiono %s\n",IPAddress(*whereto).toString().c_str());
    return 1;
}

uint8_t pfsMyIP(Print &Ser, char *par)
{
    return pfsIPADR(Ser,par,&netPrefs.ip,"Adres IP");
}
uint8_t pfsMyGW(Print &Ser, char *par)
{
    return pfsIPADR(Ser,par,&netPrefs.gw,"Adres bramy");
}
uint8_t pfsMyMask(Print &Ser, char *par)
{
    return pfsIPADR(Ser,par,&netPrefs.mask,"Maska podsieci",1);
}
uint8_t pfsMyNS1(Print &Ser, char *par)
{
    return pfsIPADR(Ser,par,&netPrefs.ns1,"Pierwszy serwer nazw",2);
}

uint8_t pfsMyNS2(Print &Ser, char *par)
{
    return pfsIPADR(Ser,par,&netPrefs.ns2,"Drugi serwer nazw",2);
}

uint8_t properAvahi(const char *par, char *whereto)
{
    if (!strcmp(par,"-")) {
        *whereto = 0;
        return 1;
    }
    if (strlen(par) > 14) return 0;
    if (strlen(par) < 3) return 0;
    uint8_t mode = 0;
    const char *p;
    for (p=par;*par;par++) {
        switch(mode) {
            case 0: // start
            if (!isalpha(*par)) return 0;
            mode = 1;
            break;

            case 1: // po cyfrze/literze
            if (*par == '-') {
                mode = 2;
                break;
            }
            if (!isalnum(*par)) return 0;
            break;

            case 2:
            if (!isalnum(*par)) return 0;
            mode = 1;
            break;
        }
    }
    if (mode == 2) return 0;
    strcpy(whereto, p);
    return 1;
}
            
uint8_t pfsNetName(Print &Ser, char *par)
{
    if (par && *par) {
        if (!properAvahi(par,netPrefs.name)) {
            Ser.printf("Błędna nazwa\n");
            return 0;
        }
    }
    if (!netPrefs.name[0]) Ser.printf("mDNS wyłączony\n");
    else Ser.printf("Nazwa dla mDNS: \"%s\"\n",netPrefs.name);
    return 1;
}


static void printip(Print &Ser,const char *name, uint32_t ip)
{
    Ser.printf("%s: %s\n",name,
        ip?IPAddress(ip).toString().c_str():"nieustawione");
}

uint8_t pfsNet(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("SSID: %s\nHasło: %s\nKonfiguracja: %s\n",
            netPrefs.ssid[0] ? netPrefs.ssid: "nieustawione",
            netPrefs.pass[0] ? netPrefs.pass: "nieustawione",
            netPrefs.dhcp ? "automatyczna":"ręczna");
        if (!netPrefs.dhcp) {
            printip(Ser,"Adres IP urządzenia",netPrefs.ip);
            printip(Ser,"Adres IP bramy",netPrefs.gw);
            printip(Ser,"Maska sieci",netPrefs.mask);
            printip(Ser,"Pierwszy serwer nazw",netPrefs.ns1);
            printip(Ser,"Drugi serwer nazw",netPrefs.ns2);
        }
        if (!netPrefs.name[0]) Ser.printf("mDNS wyłączony\n");
        else Ser.printf("Nazwa dla mDNS: \"%s\"\n",netPrefs.name);
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeNet();
//        LockPrefs();
//        prefs.putBytes("net",(void *)&netPrefs,sizeof(netPrefs));
//        UnlockPrefs();
        resetESP();
        return 0;
    }
    if (!strcmp(par,"restore")) {
        LockPrefs();
        if (!prefs.getBytes("net",(void *)&netPrefs,sizeof(netPrefs))) {
            memset((void *)&netPrefs,0,sizeof(netPrefs));
        }
        UnlockPrefs();
        Ser.printf("Przywrócono parametry WiFi\n");
        return 1;
    }
    Ser.printf("Nieznany parametr %s\n", par);
    return 0;
}

    
uint8_t pfsTempo(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Tempo %d\n",spkPrefs.tempo);
        return 1;
    }
    int n = strtol(par,&par,10);
    if (*par || n<0 || n > 24) {
        Ser.printf("Błędne określenie tempa\n");
        return 0;
    }
    Gadacz::setSpeed(spkPrefs.tempo=n);
    Ser.printf("Tempo %d\n",spkPrefs.tempo);
    return 1;
}

uint8_t pfsPitch(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Wysokość %d\n",spkPrefs.pitch);
        return 1;
    }
    int n = strtol(par,&par,10);
    if (*par || n<0 || n > 24) {
        Ser.printf("Błędne określenie wysokości\n");
        return 0;
    }
    Gadacz::setPitch(spkPrefs.pitch=n);
    Ser.printf("Wysokość %d\n",spkPrefs.pitch);
    return 1;
}

uint8_t pfsVol(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Głośność %d\n",spkPrefs.volume);
        return 1;
    }
    int n = strtol(par,&par,10);
    if (*par || n<0 || n > 24) {
        Ser.printf("Błędne określenie głośności\n");
        return 0;
    }
    Gadacz::setVolume(spkPrefs.volume=n);
    Ser.printf("Głośność %d\n",spkPrefs.volume);
    return 1;
}

uint8_t pfsContr(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Wyrazistość %d\n",spkPrefs.contrast);
        return 1;
    }
    int n = strtol(par,&par,10);
    if (*par || n<0 || n > 100) {
        Ser.printf("Błędne określenie wyrazistości\n");
        return 0;
    }
    Gadacz::setContrast(spkPrefs.contrast=n);
    Ser.printf("Wyrazistość %d\n",spkPrefs.contrast);
    return 1;
}
    
uint8_t pfsSpk(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Tempo %d\n",spkPrefs.tempo);
        Ser.printf("Wysokość %d\n",spkPrefs.pitch);
        Ser.printf("Głośność %d\n",spkPrefs.volume);
        Ser.printf("Wyrazistość %d\n",spkPrefs.contrast);
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeGadacz();
        Ser.printf("Zapisane parametry mowy\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        loadGadacz();
        Ser.printf("Przywrócono parametry mowy\n");
        return 1;
    }
    extern uint8_t externCmd;
    if (!strcmp(par,"h") || !strcmp(par,"g")) {externCmd = CMD_HOUR;return 1;}
    if (!strcmp(par,"d")) {externCmd = CMD_DATE;return 1;}
    if (!strcmp(par,"s")) {externCmd = CMD_STOP;return 1;}
    if (!strcmp(par,"p")) {externCmd = CMD_WEATHER;return 1;}
    gadaczGetPrefs();        
    Gadacz::say(par);
    return 1;
        
}

#if 0
int32_t parseGeo(Print &Ser, char *par, char *dop, int mxd)
{
    int minus = 0;
    char *c;
    int deg, mnt, sec;
    if (*par == '-') {
        par++;
        minus=1;
    }
    if (!*par || !isdigit(*par)) return BADGEO;
    c=par;
    deg=strtol(par,&par,10);
    mxd *= 3600;
    if (*par == '.') {
        float pdm=strtod(c,&par);
        if (*par)  return BADGEO;
        deg=3600 * pdm;
        if (deg > mxd)  return BADGEO;
        if (minus) deg = -deg;
        return deg;
    }
    if (minus)  return BADGEO;
    while (*par && !isdigit(*par) && !strchr(dop,*par)) par++;
    if (!*par || !isdigit(*par))  return BADGEO;
    mnt = strtol(par,&par,10);
    if (mnt >= 60)  return BADGEO;
    sec=0;
    while (*par && !isdigit(*par) && !strchr(dop,*par)) par++;
    if (*par && isdigit(*par)) {
        sec = strtol(par,&par,10);
        if (sec >=60)  return BADGEO;
        while (*par && !strchr(dop,*par)) par++;
    }
    if (*par) {
        if (strchr("sSwW",*par)) minus = 1;
        par++;
        if (*par)  return BADGEO;
    }
    deg = deg * 3600 + mnt * 60 + sec;
    if (minus) deg = -deg;
    return deg;
}
#else
int32_t parseGeo(Print &Ser, char *par, char *dop, int mxd)
{
    uint8_t minus = 0;
    int deg,ndit;
    if (*par == '-') {
        par++;
        minus=1;
        while (isspace(*par)) par++;
    }
    if (!isdigit(*par)) return BADGEO;
    deg = strtol(par, (char **) &par, 10) * 3600;
    if (*par == '.' && isdigit(par[1])) {
        const char *s=par+1;
        ndit = 0;
        while (isdigit(*s)) {
            ndit++;
            s++;
        }
        // jeśli nie ma nic dalej, pewnie float
        while (isspace(*s)) s++;
        if (*s && strchr(dop,*s)) {
            if (minus) return BADGEO;
            if (strchr("sSwW",*s)) minus = 1;
            s++;
            while (isspace(*s)) s++;
            if (*s) return BADGEO;
        }
        if (!*s) {
            if (ndit == 2 && par[1] <= '5') return NUNGEO;
            deg += 3600 * strtod(par, NULL);
            if (minus) deg=-deg;
            return deg;
        }
    }
    if (minus) return BADGEO;
    while (*par && !isdigit(*par)) par++;
    int m=strtol(par,(char **)&par, 10);
    if (m >= 60) return BADGEO;
    deg += m * 60;
    while (*par && !isalnum(*par)) par++;
    if (isdigit(*par)) {
        m=strtol(par,(char **)&par, 10);
        if (m >= 60) return BADGEO;
        deg += m;
        while (*par && !isalnum(*par)) par++;
    }
    if (*par) {
        if (!strchr(dop,*par)) return BADGEO;
        if (strchr("sSwW",*par++)) minus = 1;
        while (isspace(*par)) par++;
        if (*par) return BADGEO;
    }
    if (minus) return -deg;
    return deg;
}

#endif
        

static void printgeo(Print &Ser, const char *coto, int32_t ile, const char *pmin)
{
    int mc=0;
    if (ile < 0) {
        mc = 1;
        ile = -ile;
    }
    Ser.printf("%s geograficzna: %02d°%02d'%02d\"%c\n",
        coto, ile /3600, (ile / 60) % 60, ile % 60, pmin[mc]);
}

uint8_t pfsLon(Print &Ser, char *par)
{
    if (!par || !*par) {
        printgeo(Ser,"Długość",geoPrefs.lon,"EW");
        return 1;
    }
    int lon = parseGeo(Ser, par, "eEwW", 180);
    if (lon == BADGEO) {
        Ser.printf("Błędna wartość długości\n");
        return 0;
    }
    if (lon == NUNGEO) {
        Ser.printf("Niejdnoznaczny zapis długości\n");
        return 0;
    }
    geoPrefs.lon = lon;
    geoPrefs.autoelev = ELEV_AUTO;
    printgeo(Ser,"Długość",lon,"EW");
    return 1;
}

uint8_t pfsLat(Print &Ser, char *par)
{
    if (!par || !*par) {
        printgeo(Ser,"Szerokość",geoPrefs.lat,"NS");
        return 1;
    }
    int lat = parseGeo(Ser, par, "nNsS", 90);
    if (lat == BADGEO) {
        Ser.printf("Błędna wartość szerokości\n");
        return 0;
    }
    if (lat == NUNGEO) {
        Ser.printf("Niejdnoznaczny zapis szerokości\n");
        return 0;
    }
    geoPrefs.lat = lat;
    geoPrefs.autoelev = ELEV_AUTO;
    printgeo(Ser,"Szerokość",lat,"NS");
    return 1;
}

uint8_t pfsElev(Print &Ser, char *par)
{
    int lm;
    if (par && *par) {
        if (tolower(*par) == 'a') {
            lm = ELEV_AUTO;
            par++;
        }
        else {
            lm = strtol(par, &par, 10);
        }
        if (*par || (lm != ELEV_AUTO && (lm < -400 || lm > 3000))) {
            Ser.printf("Nieprawidłowa wartość wysokości\n");
            return 0;
        }
        geoPrefs.elev = lm;
        geoPrefs.autoelev = ELEV_AUTO;
        
    }
    if (geoPrefs.elev == ELEV_AUTO) Ser.printf("Wysokość ustalana automatycznie\n");
    else Ser.printf("Wysokość %d m\n",geoPrefs.elev);
    return 1;
}

uint8_t pfsTiz(Print &Ser, char *par)
{
    if (par && *par) {
        if (!strcasecmp(par,"p")) geoPrefs.autotz = 0;
        else if (!strcasecmp(par,"a")) geoPrefs.autotz = 1;
        else {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
    }
    Ser.printf("Strefa czasowa prognozy: %s\n",geoPrefs.autotz ? "automatyczna": "Polska");
    return 1;
}

uint8_t pfsGeo(Print &Ser, char *par)
{
    if (!par || !*par) {
        printgeo(Ser,"Długość",geoPrefs.lon,"EW");
        printgeo(Ser,"Szerokość",geoPrefs.lat,"NS");
        if (geoPrefs.elev == ELEV_AUTO) Ser.printf("Wysokość ustalana automatycznie\n");
        else Ser.printf("Wysokość %d m\n",geoPrefs.elev);
        Ser.printf("Strefa czasowa prognozy: %s\n",geoPrefs.autotz ? "automatyczna": "Polska");
        return 1;
    }
    if (!strcmp(par, "save")) {
        storeGeo();
        Ser.printf("Zapisano położenie geograficzne\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        loadGeo();
        Ser.printf("Przywrócono położenie geograficzne\n");
        return 0;
    }
    Ser.printf("Błędny parametr\n");
    return 0;

}

static const char * const tmprNames[]={
    "none","ds","dht11","dht22","bmp180","bmp280","esp","bme280",NULL};
static const char * const tmprNamesD[]={
    "brak","DS18B20","DHT11","DHT22","BMP180","BMP280","esp-now","BME280",NULL};


void printMac(Print &Ser, const char *pfx, const uint8_t *mac)
{
    Ser.printf("%s%02X:%02X:%02X:%02X:%02X:%02X\n", pfx,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void printTermDev(Print &Ser, uint8_t typ, uint8_t i2cadr, const uint8_t *dsadr,const uint8_t *mac=NULL,bool addrobly=false)
{
    Ser.printf("%s",tmprNamesD[typ]);
    if (typ == DEVT_BMP180 || typ == DEVT_BMP280 || typ == DEVT_BME280) {
        if (i2cadr) Ser.printf(", adres 0x%02x\n",i2cadr);
        else Ser.printf(", adres nieustawiony\n");
        return;
    }
    if (typ == DEVT_ESPNOW) {
        if (!mac || !*mac) {
            Ser.printf(", adres nieustawiony\n");
        }
        else {
            printMac(Ser, ", adres MAC ",mac);
        }
        return;
    }
    if (typ == DEVT_DS) {
        if (*dsadr)
            Ser.printf(", adres %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
                dsadr[0], dsadr[1], dsadr[2], dsadr[3],
                dsadr[4], dsadr[5], dsadr[6], dsadr[7]);
        else Ser.printf(", adres nieustawiony\n");
        return;
    }
    Ser.printf("\n");
            
}

uint8_t pfsIterm(Print &Ser, char *par)
{
    static uint16_t ohay = 1 | 2 |  0x10 | 0x20 | 0x40 | 0x80;
    
    if (!par || !*par) {
        Ser.printf("Urządzenie wewnątrz: ");
        printTermDev(Ser, thrPrefs.trmin, thrPrefs.i2cin,thrPrefs.dsain,thrPrefs.inmac);
        return 1;
    }
    int i;
    for (i=0;tmprNames[i];i++) {
        if (!strcasecmp(tmprNames[i],par)) break;
    }
    if (!tmprNames[i]) {
        Ser.printf("Nieznany typ urządzenia\n");
        return 0;
    }
    if (!(ohay & (1<<i))) {
        Ser.printf("Niedopuszczalny typ urządzenia\n");
        return 0;
    }
    thrPrefs.trmin = i;
    Ser.printf("Ustawiono urządzenie wewnątrz: %s\n",tmprNamesD[thrPrefs.trmin]);
    if (i == DEVT_BMP280 || i == DEVT_BME280) {
        if (I2DEVOK(0x76)) thrPrefs.i2cin = 0x76;
        else if (I2DEVOK(0x77)) thrPrefs.i2cin = 0x77;
        else thrPrefs.i2cin=0;
    }
    else if (i == DEVT_BMP180) {
        thrPrefs.i2cin = 0x77;
    }

    if (i == DEVT_BMP280 || i == DEVT_BMP280 || i == DEVT_BME280) {
        if (thrPrefs.i2cin) Ser.printf("Ustawiono automatycznie adres 0x%02X\n",thrPrefs.i2cin);
        else Ser.printf("Urządzenie niepodłączone, ustaw adres poleceniem itaddr\n");
    }
    return 1;
}

uint8_t pfsEspvalid(Print &Ser, char *par)
{
    if (par && *par) {
        int n=strtol(par, &par, 10);
        if (*par || n < 5 || n > 25) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        thrPrefs.espwait = n;
    }
    Ser.printf("Okres ważności %d minut\n",thrPrefs.espwait);
    return 1;
}

static uint8_t autoAdrDS(uint8_t *adr, uint8_t *othadr, uint8_t othtyp)
{
    int cnt = indexOneWire();
    int i,n;
    for (i=0,n=-1;i<cnt;i++) {
        if (othtyp == DEVT_DS && !memcmp(othadr, owids[i],8)) continue;
        if (n == -1) n=i;
        else return 0; // too many
    }
    if (n >= 0) {
        memcpy(adr, owids[n], 8);
        return 1;
    }
    return 0;
}



uint8_t pfsEterm(Print &Ser, char *par)
{
    static uint16_t ohay = 1 | 2 |  4 | 8 | 0x40;
    
    if (!par || !*par) {
        Ser.printf("Urządzenie na zewnątrz: ");
        printTermDev(Ser, thrPrefs.trmex, thrPrefs.i2cex,thrPrefs.dsaex,thrPrefs.exmac);
        return 1;
    }
    int i;
    for (i=0;tmprNames[i];i++) {
        if (!strcasecmp(tmprNames[i],par)) break;
    }
    if (!tmprNames[i]) {
        Ser.printf("Nieznany typ urządzenia\n");
        return 0;
    }
    if (!(ohay & (1<<i))) {
        Ser.printf("Niedopuszczalny typ urządzenia\n");
        return 0;
    }
    thrPrefs.trmex = i;
    Ser.printf("Ustawiono urządzenie na zewnątrz: %s\n",tmprNamesD[thrPrefs.trmex]);
    if (thrPrefs.trmex == DEVT_DS) {
        if (autoAdrDS(thrPrefs.dsaex, thrPrefs.dsain, thrPrefs.trmin)) {
            Ser.printf("Automatyczny adres: ");
            printTermDev(Ser, thrPrefs.trmex, thrPrefs.i2cex,thrPrefs.dsaex,thrPrefs.exmac,true);
        }
    }

    return 1;
}

uint8_t pfsHard(Print &Ser, char *par)
{
    if (!*par) {
        Ser.printf("Wewnątrz: "); printTermDev(Ser, thrPrefs.trmin, thrPrefs.i2cin,thrPrefs.dsain,thrPrefs.inmac);
        Ser.printf("Na zewnątrz: ");printTermDev(Ser, thrPrefs.trmex, thrPrefs.i2cex,thrPrefs.dsaex,thrPrefs.exmac);
        Ser.printf("Dane zdalnego czujnika ważne przez %d minut\n",thrPrefs.espwait);
        Ser.printf("Źródło czasu: %s\n",thrPrefs.ds3231 ? "zegar sprzętowy": "internet");
        
    
        return 1;
    }
    if (!strcmp(par,"save")) {
        LockPrefs();
        prefs.putBytes("hrd",(void *)&thrPrefs,sizeof(thrPrefs));
        UnlockPrefs();
        Ser.printf("Zapisano ustawienia sprzętowe\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        LockPrefs();
        if (!prefs.getBytes("hrd",(void *)&thrPrefs,sizeof(thrPrefs))) {
            memset((void *)&thrPrefs,0,sizeof(thrPrefs));
        }
        if (thrPrefs.espwait < 5 ||  thrPrefs.espwait > 25) thrPrefs.espwait = 15;
        UnlockPrefs();
        Ser.printf("Odtworzono ustawienia sprzętowe\n");
        return 1;
    }
    Ser.printf("Błędny parametr\n");
    return 0;
}
static const char *const adrErrors[]={
    "Błędny zapis liczby szesnastkowej",
    "Niedopuszczalna wartość",
    "To urządzenie nie ma ustawianego adresu",
    "Błędny zapis adresu MAC",
    "Błędny parametr"};

int geti2cadr(const char *c)
{
    if (*c == '0' && tolower(c[1]) == 'x') c+=2;
    if (strlen(c) != 2) return -1;
    int adr=strtol(c,(char **)&c,16);
    if (*c) return -1;
    return adr;
}

uint8_t getDSAdr(const char *par,uint8_t *adr)
{
    if (par[1]) return 5;
    int idx = par[0] - '1';
    if (idx < 0 || idx >= owcount) return 2;
    memcpy(adr,owids[idx],8);
    return 0;
}

static uint8_t getExtMac(const char *par, uint8_t *adr)
{
    int i;
    uint8_t mac[6];
    if (!strcmp(par,"unset")) {
        memset((void *)adr, 0,6);
        return 1;
    }
    for (i=0;i<6;i++) {
        if (i) {
            while (*par && !isxdigit(*par)) par++;
        }
        if (!*par || !par[1] || !isxdigit(*par) || !isxdigit(par[1])) return 4;
        if (par[2] && isxdigit(par[2])) return 4;
        mac[i]=strtol(par, (char **)&par, 16);
    }
    if (*par) return 4;
    memcpy(adr,mac,6);
    return 0;
}
        
    
uint8_t pfsEtadr(Print &Ser, char *par)
{
    if (!par || !*par) {
        printTermDev(Ser, thrPrefs.trmex, thrPrefs.i2cex,thrPrefs.dsaex,thrPrefs.exmac);
        return 1;
    }
    int err=0;
    switch(thrPrefs.trmex) {
        case DEVT_DS:
        err = getDSAdr(par,thrPrefs.dsaex);
        break;

        case DEVT_ESPNOW:
        err = getExtMac(par, thrPrefs.exmac);
        break;

        default:
        err = 3;
    }
    if (!err) {
        Ser.printf("Ustawiono: ");printTermDev(Ser, thrPrefs.trmex,
        thrPrefs.i2cex,thrPrefs.dsaex, thrPrefs.exmac);
        return 1;
    }
    Ser.printf("%s\n",adrErrors[err-1]);
    return 0;
}

uint8_t pfsItadr(Print &Ser, char *par)
{
    if (!par || !*par) {
        printTermDev(Ser, thrPrefs.trmin, thrPrefs.i2cin,thrPrefs.dsain,thrPrefs.inmac);
        return 1;
    }
    int ia,err=0;
    switch(thrPrefs.trmin) {
        case DEVT_BMP280:
        ia = geti2cadr(par);
        if (ia < 0) {
            err = 1;
            break;
        }
        if (ia != 0x76 && ia !=0x77) {
            err = 2;
            break;
        }
        thrPrefs.i2cin = ia;

        case DEVT_DS:
        err = getDSAdr(par,thrPrefs.dsain);
        break;

        case DEVT_ESPNOW:
        err = getExtMac(par, thrPrefs.inmac);
        break;

        default:
        err=3;
    }
    if (!err) {
        Ser.printf("Ustawiono: ");printTermDev(Ser, thrPrefs.trmin, thrPrefs.i2cin,thrPrefs.dsain,thrPrefs.inmac);
        return 1;
    }
    Ser.printf("%s\n",adrErrors[err-1]);
    return 0;
}

uint8_t showDallas(Print &Ser, char *par)
{
    int cnt = indexOneWire();
    if (!cnt) {
        Ser.printf("Brak urządzeń DS18B20\n");
        return 1;
    }
    Ser.printf("Urządzenia DS18B20:\n");
    int i;
    for (i=0;i<owcount;i++) {
        Ser.printf("%d: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n",
                i+1,
                owids[i][0], owids[i][1], owids[i][2], owids[i][3],
                owids[i][4], owids[i][5], owids[i][6], owids[i][7]);
    }
    return 1;
}

uint8_t pfsClksrc(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Źródło czasu: %s\n",thrPrefs.ds3231 ? "zegar sprzętowy": "internet");
        return 1;
    }
    int err=1;
    if (!par[1]) {
        if (tolower(*par) == 't') {err=0;thrPrefs.ds3231 = 1;}
        else if (tolower(*par) == 'n') {err=0;thrPrefs.ds3231 = 0;}
    }
    if (err) {
        Ser.printf("Błędny parametr\n");
        return 0;
    }
    Ser.printf("Źródło czasu: %s\n",thrPrefs.ds3231 ? "zegar sprzętowy": "internet");
    return 1;
}


uint8_t pfsWea(Print &Ser, char *par)
{
    if (!par || !*par) {
        int n=0;
        Ser.printf("Odczyty: ");
        if (weaPrefs.flags & SPKWEA_PRESS) {
            Ser.printf("ciśnienie %s", (weaPrefs.flags & SPKWEA_PRESS)? "przeliczone": "rzeczywiste");
            n=1;
        }
        if (weaPrefs.flags & SPKWEA_HYGRIN) {
            Ser.printf("%s wilgotność wewnątrz",n?", ":"");
            n=1;
        }
        if (weaPrefs.flags & SPKWEA_HYGROUT) {
            Ser.printf("%s wilgotność na zewnątrz",n?", ":"");
            n=1;
        }
        if (!n) Ser.printf("brak");
        Ser.printf("\nPrawdopodobieństwo opadów do wyliczania: %d%%\n",
            weaPrefs.minpre);
        Ser.printf("Godzina przełączania prognozy: %02d:00\n",
            weaPrefs.splithour);
            
        Ser.printf("Nazwa wewnątrz: %s\n",weaPrefs.inname);
        Ser.printf("Nazwa na zewnątrz: %s\n",weaPrefs.outname);
        return 1;
    }
    return 0;
}
            
const char * const irKeyNames[]={
    "Godzina","Data","Prognoza","Stop","Ciszej","Głośniej", "Wolniej","Szybciej","Zapisz"};
const char *const irKeyCmds[] = {
    "hour","date","wea","stop","vol-","vol+","spd-","spd+","store"};

int utfdelta(const char *c)
{
    int n=0,i;
    for (;*c;c++) if ((*c & 0xc0) == 0x80) n++;
    return n;
}

    
        
uint8_t pfsIrm(Print &Ser, char *par)
{
    if (*par) {
        if (!strcasecmp(par, "off")) {
            monitorIR = 0;
        }
        else if (!strcasecmp(par, "on")) {
            monitorIR = 1;
        }
        else {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
    }
    Ser.printf("Monitor IR %s\n",monitorIR?"aktywny":"wyłączpny");
    return 1;
}

uint8_t pfsIrc(Print &Ser, char *par)
{
    int i,j;
    const char *c=getToken(&par);
    if (!c || !*c) {
        if (!irrPrefs.enabled) {
            Ser.printf("Odniornik podczerwieni wyłączony\n");
        }
        else {
            for (i=0;i<9;i++) {
                Ser.printf("%-5s %-*s",irKeyCmds[i],8+utfdelta(irKeyNames[i]),irKeyNames[i]);
                for (j=0;j<4;j++) {
                    if (!irrPrefs.keycodes[i][j]) Ser.printf("   --    ");
                    else Ser.printf(" %08X ",irrPrefs.keycodes[i][j]);
                }
                Ser.printf("\n");
            }
        }
        return 1;
    }
    int nkey=-1;
    if (isdigit(*c) && !c[1]) {
        if (*c >= '1' && *c <='9') nkey = *c++-'1';
    }
    else {
        for (i=0;i<9;i++) if (!strcmp(c,irKeyCmds[i])) {
            nkey = i;
            break;
        }
    }
    if (nkey < 0) {
        Ser.printf("Błędny numer klawisza\n");
        return 0;
    }
    int npoz=-1;
    c=getToken(&par);
    if (c && *c && isdigit(*c) && !c[1]) {
        if (*c<'1' || *c >'4') {
            Ser.printf("Błędny numer komórki\n");
            return 0;
        }
        npoz = *c - '1';
        c=getToken(&par);
    }
    else {
        for (i=0;i<4;i++) {
            if (!irrPrefs.keycodes[nkey][i]) {
                npoz = i;
                break;
            }
        }
        if (npoz < 0) {
            Ser.printf("Wszystkie komórki są zajęte, podaj numer\n");
            return 0;
        }
    }
    uint32_t code = 0;
    if (!c || *c) {
        if (!monitorIR) {
            Ser.printf("Monitor IR musi być włączony\n");
            return 0;
        }
        if (!lastCode || millis() - lastCodeMillis > 20000) {
            Ser.printf("Nie wykryto klawisza w ciągu ostatnicj 29 sekund\n");
            return 0;
        }
        code = lastCode;
    }
    else {
        if (strlen(c) == 8) {
            code=strtol(c,(char **)&c,16);
            if (*c) code = 0;
        }
        if (!code) {
            Ser.printf("Nieprawidłowy zapis kodu\n");
        }
    }
    for (i=0;i<9;i++) for (j=0;j<4;j++) {
        if (i == nkey && j == npoz) continue;
        if (irrPrefs.keycodes[i][j] == code) {
            Ser.printf("Kod jest już przyporządkowany do funkcji %s\n",irKeyNames[i]);
            return 0;
        }
    }
    irrPrefs.keycodes[nkey][npoz] = code;
    Ser.printf("Przypisano: %s",irKeyNames[nkey]);
    for (j=0;j<4;j++) {
        if (!irrPrefs.keycodes[nkey][j]) Ser.printf("    --   ");
        else Ser.printf(" %08X",irrPrefs.keycodes[nkey][j]);
    }
    Ser.printf("\n");
    return 1;
    
}
uint8_t pfsIrr(Print &Ser, char *par)
{
    if (!par || !*par) return pfsIrc(Ser, par);
    if (!strcmp(par,"save")) {
        prefs.putBytes("irr",(void *)&irrPrefs,sizeof(irrPrefs));
        Ser.printf("Zapisano ustawienia pilota\n");
        return 1;
    }
    else if (!strcmp(par,"restore")) {
        if (!prefs.getBytes("irr",(void *)&irrPrefs,sizeof(irrPrefs))) {
            memset((void *)&irrPrefs,0,sizeof(irrPrefs));
        }
        Ser.printf("Przywróconono ustawienia pilota\n");
        return 1;
    }
    else if (!strcmp(par,"on")) {
        irrPrefs.enabled = 1;
        Ser.printf("Załączono odbiornik podczerwieni\n");
        return 1;
    }
    else if (!strcmp(par,"off")) {
        irrPrefs.enabled = 0;
        Ser.printf("Wyłączono odbiornik podczerwieni\n");
        return 1;
    }
    Ser.printf("Błędny parametr\n");
    return 0;
}
static const char *const lopins[]={
    "Odbiornik podczerwieni", "I2C SCL", "I2C SDA", "Audio WCLK/LRCLK",
        "Audio BCLK", "Audio OUT", "DHT22", "DS18B20", "Przycisk", "Przycisk GND"};
static const char *const shpins[]={
    "ir", "scl", "sda", "wclk", "bclk", "dout", "dht", "ds", "btn", "btng"};


#ifdef  CONFIG_IDF_TARGET_ESP32S3
static const uint8_t zeropins[]={1,0,0,0,0,0,1,1,0,1};
#ifdef S3_ALLOW_ALL_PINS
static uint8_t alPins[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
    43,44,45,46,47,48,0};
#else
static uint8_t alPins[]={1,2,3,4,5,6,7,8,9,10,43,44,0};
#endif
uint8_t pfsPinny(Print &Ser, char *par)
{
    int i,j,npin,vpin;uint8_t *pins=(uint8_t *)&pinPrefs;
    const char *tok;
    if (!par || !*par) {
        for (i=0;i<10;i++) if (pins[i]) {
            Ser.printf("Pin %-4s: %2d%c (%s)\n",shpins[i],pins[i],
                (pins[i] > 44 || (pins[i] >10 && pins[i]<20)) ? '*':' ',
            lopins[i]);
        }
        else {
            Ser.printf("Pin %-4s: nieaktywny (%s)\n",shpins[i],lopins[i]);
        }
        return 1;
    }
    tok=getToken(&par);
    if (!strcmp(tok,"restore")) {
        if (par && *par) Ser.printf("Zbędny parametr\n");
        else {
            pinPrefs = pinPrefsW;
            Ser.printf("Przywrócono do ostatnio odczytanych\n");
            return 1;
        }
    }
    if (!strcmp(tok,"default")) {
        if (par && *par) Ser.printf("Zbędny parametr\n");
        else {
            setDefaultPins();
            Ser.printf("Przywrócono do ustawień domyślnych\n");
            return 1;
        }
    }
    if (!strcmp(tok,"save")) {
        if (par && *par) Ser.printf("Zbędny parametr\n");
        else {
            for (i=0;i<9;i++) if (pins[i]) {
                for (j=i+1;j<10;j++) if (pins[i] == pins[j]) {
                    Ser.printf("Błąd: piny %s i %s mają ten sam numer %d\n",
                        shpins[i], shpins[j], pins[i]);
                    return 0;
                }
            }
            prefs.putBytes("pin",(void *)&pinPrefs,sizeof(pinPrefs));
            Ser.printf("Zapisane piny zostaną uwzględnione po resecie\n");
            return 1;
        }
    }
    for (i=0;i<10;i++) if (!strcmp(tok, shpins[i])) break;
    if (i >= 10) {
        Ser.printf("Nieprawidłowa nazwa pinu\n");
        return 0;
    }
    npin=i;
    if (par && *par) {
        int err=1;
        tok=getToken(&par);
        if (!strcmp(tok,"off") && zeropins[npin]) {
            vpin = 0;
        }
        else {
            if (isdigit(*tok)) {
                vpin=strtol(tok,(char **)&tok, 10);
                if (!*tok) err = 0;
            }
            if (err) {
                Ser.printf("Błędny parametr\n");
                return 0;
            }
            for (i=0;alPins[i];i++) if (vpin == alPins[i]) break;
            if (!alPins[i] || (npin == 0 && vpin > 18)) {
                Ser.printf("Nielegalny numer GPIO\n");
                return 0;
            }
        }
        pins[npin] = vpin;
    }
    if (!pins[npin])
        Ser.printf("Pin %-4s: nieaktywny (%s)\n",shpins[npin],lopins[npin]);
    else
        Ser.printf("Pin %-4s: %2d%c (%s)\n",shpins[npin],pins[npin],
        (pins[npin] > 44 || (pins[npin] >10 && pins[npin]<20)) ? '*':' ',
        lopins[npin]);
    return 0;
}
#else
static uint8_t statPins[]={PIN_IR_RX,SCL,SDA,WCLK_PIN,BCLK_PIN,DOUT_PIN,
        PIN_DHT,PIN_ONEWIRE,PIN_BUTTON};

uint8_t pfsPinny(Print &Ser, char *par)
{
    int i;
    for (i=0;i<9;i++) {
        Ser.printf("Pin %-4s: %d (%s)\n",shpins[i],statPins[i],lopins[i]);
    }
    return 1;
}

#endif
static uint8_t pwl(Print &Ser, const char *coto, int onoff)
{
    
    return 1;
}

static uint8_t pwx(Print &Ser, const char *coto, char *par, int bit)
{
    if (par && *par) {
        if (!strcasecmp(par,"t")) weaPrefs.flags |= bit;
        else if (!strcasecmp(par,"n")) weaPrefs.flags &= ~bit;
        else {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
    }
    Ser.printf("%s: %s\n", coto, (weaPrefs.flags & bit)?"tak":"nie");
    return 1;
}

uint8_t pfsWeaprs(Print &Ser, char *par)
{
    return pwx(Ser, "odczyt ciśnienia",par, SPKWEA_PRESS);
}

uint8_t pfsWeanpm(Print &Ser, char *par)
{
    return pwx(Ser, "przeliczenie ciśnienia npm",par, SPKWEA_REPRESS);
}
uint8_t pfsWeafin(Print &Ser, char *par)
{
    return pwx(Ser, "wilgotność wewnątrz",par, SPKWEA_HYGRIN);
}
uint8_t pfsWeafex(Print &Ser, char *par)
{
    return pwx(Ser, "wilgotność na zewnątrz",par, SPKWEA_HYGROUT);
}
uint8_t pfsCalhly(Print &Ser, char *par)
{
    return pwx(Ser, "Odczyt świąt państwowych",par, SPKWEA_HOLLY);
}
uint8_t pfsCalchu(Print &Ser, char *par)
{
    return pwx(Ser, "Odczyt świąt kościelnych",par, SPKWEA_CHURCH);
}
uint8_t pfsCaltom(Print &Ser, char *par)
{
    return pwx(Ser, "Odczyt wydarzeń na następny dzień",par, SPKWEA_TOMORROW);
}
uint8_t pfsCalWay(Print &Ser, char *par)
{
    return pwx(Ser, "Odczyt wydarzeń również przy prognozie",par, SPKWEA_HALWAYS);
}

uint8_t pfsWeaCur(Print &Ser, char *par)
{
    int n;
    static const char *const curves[]={"brak","krótkie","pełne"};
    if (par && *par) {
        if (!strcmp(par,"none")) n=0;
        else if (!strcmp(par,"short")) n=1;
        else if (!strcmp(par,"full")) n=2;
        else {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        weaPrefs.flags = (weaPrefs.flags & ~SPKWEAM_CURRENT) | (n << SPKWEAS_CURRENT);
    }
    n= (weaPrefs.flags & SPKWEAM_CURRENT) >> SPKWEAS_CURRENT;
    if (n > 2) n = 2;
    Ser.printf("Odczytywanie warunków bieżących: %s\n",
        curves[n]);
    return 1;
}

uint8_t pfsWeapre(Print &Ser, char *par)
{
    if (par && *par) {
        int n = strtol(par, &par,10);
        if (*par || n < 10 || n > 60) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        weaPrefs.minpre = n;
    }
    Ser.printf("Minimalne prawdopodobieństwo opadów: %d%%\n",weaPrefs.minpre);
    return 1;
}
uint8_t pfsWeatmr(Print &Ser, char *par)
{
    if (par && *par) {
        int n = strtol(par, &par,10);
        if (*par || n < 12 || n > 16) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        weaPrefs.splithour = n;
    }
    Ser.printf("Przełączanie prognozy: %02d:00\n",weaPrefs.splithour);
    return 1;
}


static uint8_t pfsw(Print &Ser, const char *coto, char *name, char *par)
{
    if (par && *par) {
        if (strchr(par, '"') || strlen(par) < 4 || strlen(par) > 23) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        strcpy(name, par);
    }
    Ser.printf("Nazwa %s: %s\n",coto,name);
    return 1;
}

uint8_t pfsWeain(Print &Ser, char *par)
{
    return pfsw(Ser,"wewnątrz",weaPrefs.inname,par);
}
uint8_t pfsWeaout(Print &Ser, char *par)
{
    return pfsw(Ser,"na zewnątrz",weaPrefs.outname,par);
}

uint8_t pfsWeaSR(Print &Ser, char *par)
{
    if (!par || !*par) {
        pwx(Ser,"Odczyt ciśnienia",NULL,SPKWEA_PRESS);
        pwx(Ser,"Przeliczanie ciśnienia npm",NULL,SPKWEA_REPRESS);
        pwx(Ser,"Odczyt wilgotności wewnątrz",NULL,SPKWEA_HYGRIN);
        pwx(Ser,"Odczyt wilgotności na zewnątrz",NULL,SPKWEA_HYGROUT);
        pwx(Ser,"Odczyt świąt państwowych",NULL,SPKWEA_HOLLY);
        pwx(Ser,"Odczyt świąt kościelnych",NULL,SPKWEA_CHURCH);
        pwx(Ser,"Odczyt wydarzeń na jutro",NULL,SPKWEA_TOMORROW);
        Ser.printf("Minimalne prawdopodobieństwo opadów: %d%%\n",weaPrefs.minpre);
        Ser.printf("Przełączanie prognozy: %02d:00\n",weaPrefs.splithour);
        Ser.printf("Nazwa wewnątrz: %s\n",weaPrefs.inname);
        Ser.printf("Nazwa na zewnątrz: %s\n",weaPrefs.outname);
        
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeWea();
        Ser.printf("Zapisano ustawienia odczytu prognozy\n");
        return 1;
    }
    else if (!strcmp(par,"restore")) {
        loadWea();
        Ser.printf("Przywrócono ustawienia odczytu prognozy\n");
        return 1;
    }
    Ser.printf("Błędny parametr\n");
    return 0;
}

uint8_t pfsClkSync(Print &Ser, char *par)
{
    const char *rc=synchronizeClock();
    if (rc) {
        Ser.printf("Błąd: %s\n",rc);
    }
    Ser.printf("Zsynchronizowano czas\n");
    return 1;
}

uint8_t pfsClkOff(Print &Ser, char *par)
{
    if (par && *par) {
        int n = strtol(par, &par,10);
        if (*par || n < -60 || n > 240 || (n %15)) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
        clkPrefs.offset = n;
    }
    Ser.printf("Strefa czasowa: +%d\n",clkPrefs.offset);
    return 1;
}

uint8_t pfsClkDst(Print &Ser, char *par)
{
    if (par && *par) {
        int err=1;
        if (!par[1]) {
            if (tolower(*par) == 't') {err=0;clkPrefs.useDST = 1;}
            else if (tolower(*par) == 'n')  {err=0;clkPrefs.useDST = 0;}
        }
        if (err) {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
    }
    Ser.printf("Uwzgęnienie czasu letniego: %s\n",clkPrefs.useDST?"tak":"nie");
    return 1;
}

uint8_t pfsClkClk(Print &Ser, char *par)
{
    if (!par || !*par) {
        Ser.printf("Strefa czasowa: +%d\n",clkPrefs.offset);
        Ser.printf("Uwzgędnienie czasu letniego: %s\n",clkPrefs.useDST?"tak":"nie");
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeClk();
        Ser.printf("Zapisano ustawienia czasu\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        loadClk();
        Ser.printf("Przywrócono ustawienia czasu\n");
        return 1;
    }
    Ser.printf("Błędny parametr\n");
    return 0;
}

const char * const monthNM[]={
    "stycznia","lutego","marca","kwietnia","maja","czerwca",
    "lipca","sierpnia","września","października",
    "listopada","grudnia"};
uint8_t pfsCalDay(Print &Ser, char *par)
{
    const char *p1=NULL, *p2=NULL;
    uint8_t d, m;
    p1=getToken(&par);
    p2=getToken(&par);
    if (!p2) {
        Ser.printf("Podaj dzień i miesiąc\n");
        return 0;
    }
    char *c;
    d=strtol(p1,&c,10);if (c && *c) {
        Ser.printf("Błędny parametr dzień\n");
        return 0;
    }
    m=strtol(p2,&c,10);if (c && *c) {
        Ser.printf("Błędny parametr miesiąc\n");
        return 0;
    }
    if (properDate(d,m)) {
        Ser.printf("Taki dzień nie istnieje\n");
        return 0;
    }
    int i;
    if (!strcmp(par,"delete")) {
        Ser.printf("Usuwam dzień %d %d\n",d,m);
        for (i=0;i<32;i++) {
            if (calPrefs.days[i].day == d && calPrefs.days[i].month == m) {
                calPrefs.days[i].day=0;
                calPrefs.days[i].month=0;
            }
        }
    }
    else if (*par) {
        if (strlen(par) > 31) {
            Ser.printf("Za długi tekst\n");
            return 0;
        }
        for (i=0;i<32;i++) {
            if (calPrefs.days[i].day == d && calPrefs.days[i].month == m) break;
        }
        if (i >= 32) {
            for (i=0;i<32;i++) if (!calPrefs.days[i].day) break;
        }
        if (i >= 32) {
            Ser.printf("Brak miejsca na kolejny wpis\n");
            return 0;
        }
        calPrefs.days[i].day =d;
        calPrefs.days[i].month = m;
        strcpy(calPrefs.days[i].text, par);
    }
    for (i=0;i<32;i++) {
        if (calPrefs.days[i].day == d && calPrefs.days[i].month == m) break;
    }
    if (i <32 )Ser.printf("%d %s: %s\n",
                calPrefs.days[i].day,
                monthNM[calPrefs.days[i].month-1],
                calPrefs.days[i].text);

    else Ser.printf("Brak wydarzenia na %d %s\n",d,monthNM[m-1]);
    return 1;
}
uint8_t pfsCalAll(Print &Ser, char *par)
{
    if (!par || !*par) {
        int i,d;
        for (i=d=0;i<32;i++) if (calPrefs.days[i].day) {
            Ser.printf("%d %s: %s\n",
                calPrefs.days[i].day,
                monthNM[calPrefs.days[i].month-1],
                calPrefs.days[i].text);
            d=1;
        }
        if (!d) Ser.printf("Kalendarz jest pusty\n");
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeCal();
        Ser.printf("Zapisano ustawienia kalendarza\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        loadCal();
        Ser.printf("Przywrócono ustawienia kalendarza\n");
        return 1;
    }
    Ser.printf("Błędny parametr\n");
    return 0;
    
}


