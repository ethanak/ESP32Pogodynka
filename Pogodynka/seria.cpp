#include <Arduino.h>
#include <Arduino.h>
#include "Pogoda.h"
#include <ctype.h>
#include <nvs_flash.h>


const char *getToken(char **line)
{
    while (**line && isspace(**line)) (*line)++;
    if (!**line) return NULL;
    char *rc=*line;
    while (**line && !isspace(**line)) (*line)++;
    if (**line) *(*line)++=0;
    return rc;
}


static uint8_t pfsHelp(Print &Ser, char *s);

static uint8_t fmtNvm(Print &Ser, char *s)
{
    if (strcmp(s,"preferences")) {
        Ser.printf("Nieprawidłowy parametr\n");
        return 0;
    }
    nvs_flash_erase();
    nvs_flash_init();
    delay(100);
    resetESP();
}

static uint8_t pfsSetDbg(Print &Ser, char *s)
{
    if (s && *s) {
        if (!strcmp(s,"on")) debugMode = 1;
        else if (!strcmp(s,"off")) debugMode = 0;
        else {
            Ser.printf("Błędny parametr\n");
            return 0;
        }
    }
    Ser.printf("Tryb debugowania %słączony\n",debugMode ? "za":"wy");
    return 1;
}
static uint8_t showI2C(Print &Ser, char *s)
{
    int i,n;
    //for (i=0;i<16;i++) Ser.printf("%02x ",i2cdevices[i]);
    Ser.printf("Urządzenia I2C");
    for (i=1,n=0; i<0x7f; i++) if (i2cdevices[i>>3] & (1 << (i&7))) {
        Ser.printf("%c 0x%02X",n?',':':',i);
        n++;
    }
    if (!n) Ser.printf(" - brak\n");else Ser.printf("\n");
    return 1;
}

static uint8_t showLocMac(Print &Ser, char *s)
{
    uint8_t mac[6];
    WiFiGetMac(mac);
    Ser.printf("adres MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 1;
}

static uint8_t showLocIP(Print &Ser, char *s)
{
     char buf[20];
     if (!WiFiGetIP(buf)) Ser.printf("Urządzenie niepodłączone\n");
     else Ser.printf("Adres IP %s\n",buf);
     return 1;
}

static uint8_t showSrvAdr(Print &Ser, char *s)
{
    char buf[128], *c;
    c=buf;
    if (imAp) {
        strcpy(buf,"SSID: " pogoApSSID ", Hasło: " pogoApPASS ", Serwer: http://");
    }
    else {
        strcpy(buf,"Serwer: http://");
    }
    c=buf+strlen(buf);
    if (!WiFiGetIP(c)) {
        Ser.printf("Urządzenie niepodłączone\n");
        return 0;
    }
    //strcat(buf,":8080/");
    Ser.printf("%s:%d/\n", buf,SERVER_PORT);
    return 1;
}

static uint8_t showVersion(Print &Ser, char *s)
{
    Ser.printf("Wersja firmware: %s\n", firmware_version);
    return 1;
}
 
static uint8_t rstEsp(Print &Ser, char *s)
{
    resetESP();
    return 0;
}

static const struct serCommand {
    const char *name;
    const char *shdes;
    const char *ldes;
    uint8_t (*fun)(Print &,char *);
} serCmds[] ={
    {"help","Pomoc","Parametr - polecenie którego dotyczy pomoc",pfsHelp},
    {"+network","Połączenie WiFi","""UWAGA!!!\n\
    Zmiany wprowadzane w tej sekcji będą aktywne dopiero po wydaniu\n\
    polecenia 'net save'""",NULL},
    {"ssid","Nazwa sieci WiFi","Podanie parametru ustawia nazwę",pfsSsid},
    {"pass","Hasło sieci WiFi","Podanie parametru ustawia hasło",pfsPass},
    {"dhcp","Konfiguracja IP","a - automatyczna; r - ręczna",pfsDhcp},
    {"ip","Adres IP","IP urządzenia dla ręcznego ustawienia",pfsMyIP},
    {"gw","Adres IP bramy","IP bramy dla ręcznego ustawienia",pfsMyGW},
    {"mask","Maska podsieci","IP lub prefiks dla ręcznego ustawienia",pfsMyMask},
    {"name1","Pierwszy serwer nazw","IP pierwszego serwera nazw lub 0",pfsMyNS1},
    {"name2","Drugi serwer nazw","IP drugiego serwera nazw lub 0",pfsMyNS2},
    {"mdns","Nazwa dla bonjour/avahi","Bez parametru podaje aktualną nazwę\n\
    Podana nazwa może zawierać wyłącznie małe litery ASCII oraz cyfry,\n\
    mogą być przedzielone myślnikami. Maksymalna długość nazwy 14 znaków\n\
    Podanie samego myślnika oznacza pustą nazwę (wyłączenie mDNS)",pfsNetName},
    {"fakemac","Podmieniony adres MAC stacji","Bez parametru podaje aktualny stan\n\
    Z parametrem 'off' wyłącza podmianę adresu\n\
    Z parametrem 'on' włącza podmianę adresu\n\
    Z parametrem 'set' po którym następuje adres MAC ustawia fałszywy adres",
        pfsMyFakeMac},
    {"net","Wszystkie ustawienia sieci","z parametrem save zapisuje i resetuje\n\
    z parametrem restore wraca do zapisanych ustawień\n\
    bez parametru pokazuje ustawienia",pfsNet},
    {"+talk","Ustawienia mowy",NULL,NULL},
    {"tempo","Prędkość głosu","parametr z zakresu 0 do 24",pfsTempo},
    {"pitch","Wysokość głosu","parametr z zakresu 0 do 24",pfsPitch},
    {"vol","Głośność mowy","parametr z zakresu 0 do 24",pfsVol},
    {"contr","Wyrazistość mowy","parametr z zakresu 0 do 100",pfsContr},
    {"spk","Wszystkie ustawienia i testy mowy","bez parametru - pokazanie bieżących ustawień\n\
    z parametrem save - zapis ustawień\n\
    z parametrem restore - przywrócenie zapisanych.\n\
    z parametrem h lub g - powiedz godzinę\n\
    z parametrem d - powiedz godzinę i datę\n\
    z parametrem p - powiedz godzinę, datę i prognozę\n\
    z parametrem s - zatrzymaj mowę\n\
    Podanie dowolnego innego napisu spowoduje jego wypowiedzenie",pfsSpk},
    {"+geoloc","Położenie geograficzne",NULL,NULL},
    {"lon","Długość geograficzna","liczba ułamkowa (ujemna to długość zachodnia) albo\n\
    dwie lub trzy liczby oznaczające stopnie, minuty i sekundy,\n\
    ewentualnie zakończone literą E (domyślnie wschód) lub W (zachód)", pfsLon},
    {"lat","Szerokość geograficzna","liczba ułamkowa (ujemna to szerokość południowa) albo\n\
    dwie lub trzy liczby oznaczające stopnie, minuty i sekundy,\n\
    ewentualnie zakończone literą N (domyślnie północ) lub S (południe)", pfsLat},
    {"elev","Wysokość nad poziomem morza","Wysokość w metrach lub A jeśli wyliczana automatycznie",pfsElev},
    {"tz","Strefa czasowa dla prognozy pogody","p - Polska; a - automatyczna",pfsTiz},
    {"geo","Wszystkie parametry lokalizacji","z parametrem save - zapisanie\n\
    z parametrem restore - przywrócenie zapisanych\n\    bez parametru - pokazanie bieżących",pfsGeo},
    {"+pogoda","Ustawienia czytania prognozy, czujników i wydarzeń",NULL, NULL},
    {"weaprs","Odczyt ciśnienia","t: tak, n:nie",pfsWeaprs},
    {"weanpm","Przeliczenie na poziom morza","t: tak, n:nie",pfsWeanpm},
    {"weahin","Odczyt wilgotności wewnątrz","t: tak, n:nie",pfsWeafin},
    {"weahex","Odczyt wilgotności na zewnątrz","t: tak, n:nie",pfsWeafex},
    {"calday","Odczytywanie świąt państwowych","t: tak, n:nie",pfsCalhly},
    {"calall","Odczytywanie również świąt kościelnych","t: tak, n:nie",pfsCalchu},
    {"caltmr","Odczytywanie świąt i kalendarza na jutro","t: tak, n:nie",pfsCaltom},
    {"calalw","Odczytywanie świąt i kalendarza również przy prognozie",
        "t: tak, n: nie",pfsCalWay},

    {"weacur","Tryb odczytywania aktualnych warunków",
        "none - nie odczytuj; short - krótko; full - pełne", pfsWeaCur},
    {"weapre","Prawdopodobieństwo opadów liczone jako deszcz",
            "liczba od 10 do 60, domyślnie 25",pfsWeapre},
    {"weatmr","Godzina przełączenia prognozy na jutro i pojutrze",
            "liczba od 12 do 18",pfsWeatmr},
    {"weain","Nazwa odczytu wewnątrz","Napis do 23 bajtów, niedopuszczalny cudzysłów. Domyślnie: wewnątrz",pfsWeain},
    {"weaout","Nazwa odczytu na zewnątrz","Napis do 23 bajtów, niedopuszczalny cudzysłów. Domyślnie: na zewnątrz",pfsWeaout},
    {"wea","Wszystkie odczytu pogody","z parametrem save - zapisanie\n\
    z parametrem restore - przywrócenie zapisanych\n\
    bez parametru - pokazanie bieżących",pfsWeaSR},
    {"+calend","Ustawienia kalendarza", NULL, NULL},
    {"day","Dzień w kalendarzu", "Parametry to dwie liczby określające dzień i miesiąc.\n\
    Jeśli nie podano trzeciego parametru, wyświetla zdarzenie na dany dzień.\n\
    Trzecim parametrem może być nazwa zdarzenia (np. imieniny żony)\n\
    lub słowo delete oznaczające usunięcie wpisu", pfsCalDay},
    {"cal","Ustawienia kalendarza","z parametrem save - zapisanie\n\
    z parametrem restore - przywrócenie zapisanych\n\
    bez parametru - pokazanie bieżących",pfsCalAll},
#if defined(BOARD_HAS_PSRAM) || defined(UNIT_USE_EVENTS)
    {"+timetab","Edycja terminarza",NULL,NULL},
    {"event","dodanie, usunięcie lub edycja wydarzenia",
        "Bez parametrów - pokazuje wszystkie wydarzenia\n\
    Trzy liczby to dzień, miesiąc i rok\n\
    Bez czwartego parametru pokazuje wydarzenie na dany dzień\n\
    Jeśli czwarty parametr to \"del\", usuwa wydarzenie\n\
    W przeciwnym przypadku dodaje nowy wpis",pfsEvent},
        
#endif
    {"+remote","Ustawienia klawiszy pilota", NULL, NULL},
    {"irmon", "Monitor podczerwieni","on - włącz, off - wyłącz",pfsIrm},
    {"irkey", "Przyporządkowanie kodów pilota",
            "liczba 1 do 4 - numer klawisza, liczba 1 ro 4 - wariant, opcjonalnie kod (8 cyfr szesnastkowych\n\
    Brak kodu oznacza ostatnio odebrany, zero to usunięcie kodu",pfsIrc},
    {"irr", "Ustawienia pilota","z parametrem save - zapisanie\n\
    z parametrem restore - przywrócenie zapisanych\n\
    z parametrem on lub off - włączenie/wyłączenie odbiornika\n\
    bez parametru - pokazanie bieżących",pfsIrr},
    {"+clock","Ustawienia czasu",NULL,NULL},
    {"tz","Strefa czasowa","offset stefy czasowej w minutach (Polska=60)",pfsClkOff},
    {"dst","Zmiana czasu","t: uwzględniana, n: nieuwzględniana",pfsClkDst},
    {"synclk","Synchronizacja czasu z internetem",
    "Nie przyjmuje parametrów. Synchronizuje zegar urządzenia\n\
    z serwerami czasu. Jeśli zegar sprzętowy jest dostępny, ustawia\n\
    również czas na tym zegarze", pfsClkSync},
    {"clk","Wszystkie ustawienia czasu","z parametrem save - zapisanie\n\
    z parametrem restore - przywrócenie zapisanych\n\
    bez parametru - pokazanie bieżących",pfsClkClk},
    {"+hardware","Ustawienia sprzętowe",NULL,NULL},
    {"iterm","Typ termometru wewnętrznego","Dopuszczalne: bmp180, bmp280, bme280, ds, esp, none",pfsIterm},
    {"eterm","Typ termometru zewnętrznego","Dopuszczalne: dht11, dht22, ds, esp, none",pfsEterm},
    {"itermadr","Adres termometru wewnętrznego","dwie cyfry szesnastkowe dla I2C\n\
    osiem dwucyfrowych liczb szesnastkowych lub indeks dla DS18B20\n\
    Liczby mogą być oddzielane dowolnymi znakami",pfsItadr},
    {"etermadr","Adres termometru zewnętrznego","indeks dla DS18B20 z polecenia dallas\n\
    lub sześć dwucyfrowych liczb szesnastkowych dla esp-now (adres MAC)\n\
    Liczby mogą być oddzielane dowolnymi znakami",pfsEtadr},
    {"espvalid","Okres ważności danych ze zdalnego czujnika",
    "Bez poarametru podaje okres.\n\
    Parametr to liczba minut od 5 do 25 (domyślnie 15)",pfsEspvalid},
    {"rtc","Użycie sprzętowego zegara","t: tak, n:nie",pfsClksrc},
    {"hard","Ustawienia sprzętowe","z parametrem save - zapisanie"
#ifdef  CONFIG_IDF_TARGET_ESP32S3
    " (oprócz pinów)"
#endif
        "\n\
    z parametrem restore - przywrócenie zapisanych\n\
    bez parametru - pokazanie bieżących",pfsHard},
    {"i2c","Pokazuje podłączone urządzenia I2C",NULL,showI2C},
    {"dallas","Pokazuje podłączone urządzenia DS18B20",NULL,showDallas},
    {"showmac","Pokazuje adres MAC urządzenia",NULL,showLocMac},
    {"showip","Pokazuje adres IP urządzenia",NULL,showLocIP},
    {"showsrv","Pokazuje URL serwera urządzenia",NULL,showSrvAdr},
    {"erase","Formatowanie NVS","Po podaniu parametru \"preferences\" wszystkie\n\
    zapisane dane zostaną usunięte a urządzenie zrestartowane",fmtNvm},

#ifdef  CONFIG_IDF_TARGET_ESP32S3
    {"pin","Ustawienia pinów XIAO ESP32-S3","bez parametru - pokazanie pinów\n\
    Z parametrem ir, scl, sda, wclk, bclk, dout, dht, ds, btn, btng\n\
    podaje numer przydzielonego pinu lub jeśli został podany kolejny\n\
    parametr (numer) - przypisuje pin do funkcji\n\
    Z parametrem save zapisuje tablicę pinów\n\
    Z parametrem restore przywraca tablicę pinów\n\
    Z parametrem default przywraca domyślne ustawienia\n\
    Wszystkie operacje odbywają się na kopii tablicy i będą uwzględniane\n\
    dopiero po restarcie urządzenia", pfsPinny},
#else
    {"pins","Przyporządkowanie pinów DevKit",NULL, pfsPinny},
#endif
    {"+other","Pozostałe ustawienia",NULL,NULL},
#ifdef  ENABLE_OTA
    {"version","Pokazuje wersję firmware",NULL,showVersion},
    {"ota","Operacje Over-the-Air","check - sprawdzenie czy jest nowa wersja\n\
    update - aktualizacja\n\
    update force - wymuszenie aktualizacji",otaPerform},
#endif
    {"debug","Włączenie komunikatów debugowania","Parametr on/off\n\
    Nie jest zapisywany w pamięci flash",pfsSetDbg},    
    {"reset","Restart ESP",NULL, rstEsp},
    
    {NULL,NULL,NULL,NULL}};
        
static uint8_t pfsHelp(Print &Ser, char *s)
{
    int i,hp,nm=-1;
    if (!s || !*s) {
        Ser.printf("Wpisz polecenie help nazwa, gdzie nazwa to:\n");
        for (i=0;serCmds[i].name;i++) if (serCmds[i].name[0] == '+') {
            Ser.printf("%s: %s\n",serCmds[i].name+1,serCmds[i].shdes);
        }
        return 1;
    }
    for (i=hp=0,nm=-1;serCmds[i].name;i++) {
        if (serCmds[i].name[0] == '+' && !strcmp(s,serCmds[i].name+1)) {
            hp = i+1;
            break;
        }
        if (!strcmp(s,serCmds[i].name)) {
            nm = i;
            break;
        }
    }
    //Ser.printf("E %d %d\n",hp, nm);
    //return 1;
    if (hp > 0) {
        Ser.printf("Sekcja %s: %s\n",serCmds[hp-1].name+1,serCmds[hp-1].shdes);
        for (i=hp; serCmds[i].name && serCmds[i].name[0] != '+'; i++) {
            Ser.printf("%s: %s\n",serCmds[i].name,serCmds[i].shdes);
        }
        if (serCmds[hp-1].ldes) Ser.printf("%s\n",serCmds[hp-1].ldes);
        return 1;
    }
    if (nm >= 0) {
        //Ser.printf("NM=%d\n",nm);
        //return 1;
        Ser.printf("%s: %s\n    %s\n",
            serCmds[nm].name,serCmds[nm].shdes,
            serCmds[nm].ldes ? serCmds[nm].ldes : "Polecenie nie przyjmuje parametrów");
        return 1;
    }
    Ser.printf("Nieznane polecenie lub sekcja.\nWpisz polecenie help nazwa, gdzie nazwa to:\n");
    //return 0;
    for (i=0;serCmds[i].name;i++) if (serCmds[i].name[0] == '+') {
        Ser.printf("%s: %s\n",serCmds[i].name+1,serCmds[i].shdes);
    }
    return 1;
}



void doSerialCmd(Print &Ser,char *str)
{
    const char *cmd=getToken(&str);
    int i;
    for (i=0;serCmds[i].name;i++) if (serCmds[i].name[0] != '+' && !strcmp(cmd, serCmds[i].name)) break;
    if (!serCmds[i].name) {
        Ser.printf("Nieznane polecenie %s\n",cmd);
        return;
    }
    if (serCmds[i].fun) {
        serCmds[i].fun(Ser,str);
        Ser.flush();
    }
}

static struct cmdBuffer serialBuffer;

void serLoop(Stream &Ser)
{
    while (Ser.available()) {
        addCmdChar(Ser,&serialBuffer,Ser.read());
    }
}
