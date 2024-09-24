#ifndef POGODA_H
#define POGODA_H

#define firmware_version "0.2.9"

#define UNIT_USE_EVENTS 1
#define S3_ALLOW_ALL_PINS 1


// OTA tylko dla S3 z big flash

#if !defined(DISABLE_OTA) && defined(CONFIG_IDF_TARGET_ESP32S3) && BOARD_HAS_BIG_FLASH  == 1
#define ENABLE_OTA 1
#endif
// WiFi

#define pogoApSSID "pogoda"
#define pogoApPASS "wicherek"


extern void initWiFi();

//clock

extern void clockAtConnect();
extern void clockAtDisConnect();
extern void rtcLoop();
extern int _minute, _hour, _day, _month, _year, _dayoweek,_second;
extern const char *getHolly(uint8_t tomorrow);
extern const char *getCalEvent(uint8_t tomorrow);
extern void tomorrowDay(uint8_t *day, uint8_t *month, uint16_t *year = NULL);
extern bool getLocalTime();
extern bool haveClock();
extern uint8_t writeHardClock(time_t time);
extern uint32_t readHardClock();
extern uint8_t checkHardClock();
extern uint8_t clockHardStatus;
extern bool timerInitialized;
extern int TZOffset;
extern bool useDST;
extern const char *synchronizeClock();
extern uint8_t mustSyncClock;
extern const char *syncClockResponse;
extern const char *srvSynchronizeClock();
extern uint8_t properDate(uint8_t day, uint8_t month);


// pogoda

extern uint8_t parseWeather(const char *str);
extern const char * makeWeather();
extern char weabuf[];
extern bool getWeather();
extern bool startWeather();

// polecenia
enum {
    CMD_NONE=0,
    CMD_STOP,
    CMD_HOUR,
    CMD_DATE,
    CMD_WEATHER,
    CMD_VOLDOWN,
    CMD_VOLUP,
    CMD_SPDOWN,
    CMD_SPDUP,
    CMD_STORE,
    CMD_PARAMS,
    CMD_MAX
};

// hardware
#ifdef  CONFIG_IDF_TARGET_ESP32S3
#define defPIN_BUTTON 4
#define defPIN_BUTTON_GND 5
#define defPIN_SDA 43
#define defPIN_SCL 6
#define defPIN_DHT 0
#define defPIN_ONEWIRE 0
#define defPIN_IR_RX 9
#define defWCLK_PIN 8
#define defBCLK_PIN 7
#define defDOUT_PIN 44


#define PIN_BUTTON pinPrefsW.btn
#define PIN_BUTTON_GND pinPrefsW.btng
#define PIN_DHT pinPrefsW.dht
#define PIN_ONEWIRE pinPrefsW.ds
#define PIN_SDA pinPrefsW.sda
#define PIN_SCL pinPrefsW.scl
#define PIN_IR_RX pinPrefsW.ir
#define WCLK_PIN pinPrefsW.wclk
#define BCLK_PIN pinPrefsW.bclk
#define DOUT_PIN pinPrefsW.dout

#else
#define PIN_DHT 17
#define PIN_ONEWIRE 16
#define PIN_BUTTON 27
#define PIN_SDA SDA
#define PIN_SCL SCL
#define PIN_IR_RX 34
#define WCLK_PIN 13
#define BCLK_PIN 12
#define DOUT_PIN 14
#endif

// serial
extern void serLoop(Stream &Ser);

extern struct netPrefs {
    char ssid[64];
    char pass[64];
    uint32_t ip;
    uint32_t gw;
    uint32_t mask;
    uint32_t ns1;
    uint32_t ns2;
    uint8_t dhcp;
    char name[15];
} netPrefs;
extern uint8_t pfsSsid(Print &Ser, char *par);
extern uint8_t pfsPass(Print &Ser, char *par);
extern uint8_t pfsDhcp(Print &Ser, char *par);
extern uint8_t pfsMyIP(Print &Ser, char *par);
extern uint8_t pfsMyGW(Print &Ser, char *par);
extern uint8_t pfsMyMask(Print &Ser, char *par);
extern uint8_t pfsMyNS1(Print &Ser, char *par);
extern uint8_t pfsMyNS2(Print &Ser, char *par);
extern uint8_t pfsNetName(Print &Ser, char *par);
extern uint8_t pfsNet(Print &Ser, char *par);
extern uint8_t pfsTempo(Print &Ser, char *par);
extern uint8_t pfsPitch(Print &Ser, char *par);
extern uint8_t pfsVol(Print &Ser, char *par);
extern uint8_t pfsContr(Print &Ser, char *par);
extern uint8_t pfsSpk(Print &Ser, char *par);
extern uint8_t pfsLon(Print &Ser, char *par);
extern uint8_t pfsLat(Print &Ser, char *par);
extern uint8_t pfsElev(Print &Ser, char *par);
extern uint8_t pfsTiz(Print &Ser, char *par);
extern uint8_t pfsGeo(Print &Ser, char *par);
extern uint8_t pfsIterm(Print &Ser, char *par);
extern uint8_t pfsEterm(Print &Ser, char *par);
extern uint8_t pfsItadr(Print &Ser, char *par);
extern uint8_t pfsEtadr(Print &Ser, char *par);
extern uint8_t pfsEspvalid(Print &Ser, char *par);
extern uint8_t pfsHard(Print &Ser, char *par);
extern uint8_t pfsClksrc(Print &Ser, char *par);
extern uint8_t showDallas(Print &Ser, char *par);
extern uint8_t pfsWea(Print &Ser, char *par);
extern uint8_t pfsIrm(Print &Ser, char *par);
extern uint8_t pfsIrc(Print &Ser, char *par);
extern uint8_t pfsIrr(Print &Ser, char *par);
extern uint8_t pfsWeaprs(Print &Ser, char *par);
extern uint8_t pfsWeanpm(Print &Ser, char *par);
extern uint8_t pfsWeafin(Print &Ser, char *par);
extern uint8_t pfsWeafex(Print &Ser, char *par);
extern uint8_t pfsWeapre(Print &Ser, char *par);
extern uint8_t pfsWeatmr(Print &Ser, char *par);
extern uint8_t pfsWeain(Print &Ser, char *par);
extern uint8_t pfsWeaout(Print &Ser, char *par);
extern uint8_t pfsCalhly(Print &Ser, char *par);
extern uint8_t pfsCalchu(Print &Ser, char *par);
extern uint8_t pfsCaltom(Print &Ser, char *par);
extern uint8_t pfsWeaSR(Print &Ser, char *par);
extern uint8_t pfsClkSync(Print &Ser, char *par);
extern uint8_t pfsClkOff(Print &Ser, char *par);
extern uint8_t pfsClkDst(Print &Ser, char *par);
extern uint8_t pfsClkClk(Print &Ser, char *par);
extern uint8_t pfsCalDay(Print &Ser, char *par);
extern uint8_t pfsCalAll(Print &Ser, char *par);
extern uint8_t pfsCalWay(Print &Ser, char *par);
extern uint8_t pfsWeaCur(Print &Ser, char *par);
extern uint8_t pfsPinny(Print &Ser, char *par);
#if defined(BOARD_HAS_PSRAM) || defined (UNIT_USE_EVENTS)

extern uint8_t pfsEvent(Print &Ser, char *par);
extern uint8_t eventInPast(struct eEvent *ev);
extern uint8_t dateInPast(int d, int m, int y);
extern int insertEvent(int day, int mon, int year,const char *txt);
extern void storeEvts();
extern void restoreEvts();

#endif

#define EVENT_COUNT 40

struct thermoData {
    uint8_t flags;
    uint8_t accStatusIn;
    uint8_t accStatusExt;
    uint8_t kludge;
    uint16_t pressIn;
    uint16_t pressOut;
    float tempIn;
    float tempOut;
    float hygrIn;
    float hygrOut;
    uint16_t accVoltageIn;
    uint16_t accVoltageOut;
};


#define THV_TEMP 1
#define THV_PRES 2
#define THV_HGR 4
#define THV_ACU 8
#define THV_ETEMP (THV_TEMP << 4)
#define THV_EPRES (THV_PRES << 4)
#define THV_EHGR (THV_HGR << 4)
#define THV_EACU (THV_ACU << 4)

    

extern struct spkPrefs {
    uint8_t tempo;
    uint8_t pitch;
    uint8_t volume;
    uint8_t contrast;
} spkPrefs;

extern struct clkPrefs {
    int16_t offset;
    uint8_t useDST;
} clkPrefs;

extern struct geoPrefs {
    int32_t lon;
    int32_t lat;
    unsigned int autotz:1;
    int elev:15;
    int16_t autoelev;
} geoPrefs;

#define ELEV_AUTO 3200

extern struct thrPrefs {
    uint8_t trmin;
    uint8_t trmex;
    uint8_t i2cin;
    uint8_t i2cex;
    uint8_t ds3231;
    uint8_t espwait;
    uint8_t exmac[6];
    uint8_t dsain[8];
    uint8_t dsaex[8];
    uint8_t inmac[6];
} thrPrefs;

    
extern struct weaPrefs {
    uint16_t flags;
    uint8_t minpre;
    uint8_t splithour;
    char inname[24];
    char outname[24];
} weaPrefs;
    
#define SPKWEA_PRESS 1
#define SPKWEA_REPRESS 2
#define SPKWEA_HYGROUT 4
#define SPKWEA_HYGRIN 8
#define SPKWEA_HOLLY 0x10
#define SPKWEA_CHURCH 0x20
#define SPKWEA_TOMORROW 0x40
#define SPKWEA_HALWAYS 0x80

#define SPKWEAS_CURRENT 8
#define SPKWEAM_CURRENT 0x300

#define SPKWEAC_CURRENT_NONE 0
#define SPKWEAC_CURRENT_SHORT 1
#define SPKWEAC_CURRENT_FULL 2

#if defined(BOARD_HAS_PSRAM) || defined (UNIT_USE_EVENTS)
extern struct eEvent {
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t pos;
    char data[44];
} *eEvents;
#endif

    


extern struct irrPrefs {
    uint32_t keycodes[9][4];
    uint8_t enabled;
} irrPrefs;

    
extern struct calPrefs {
    struct {
        uint8_t day;
        uint8_t month;
        char text[30];
    } days[32];
} calPrefs;

extern void initPrefs();
extern bool canConnect();

extern void gadaczSayDemo();
extern void gadaczGetPrefs();

extern void storeGeo();
extern void storeGadacz();
extern void storeClk();
extern void storeWea();
extern void storeNet();
extern void storeCal();

extern void loadGeo();
extern void loadGadacz();
extern void loadClk();
extern void loadWea();
extern void loadCal();

extern uint8_t i2cdevices[16];

enum {
    DEVT_NONE=0,
    DEVT_DS,
    DEVT_DHT11,
    DEVT_DHT22,
    DEVT_BMP180,
    DEVT_BMP280,
    DEVT_ESPNOW,
    DEVT_BME280,
    //DEVT_AHT20,
    DEVT_MAX};
    
extern uint8_t getDHT();
//extern uint8_t getBMP280();
extern uint8_t getBME280();
extern uint8_t initThermo();
extern float tempIn, pressIn,tempOut, hgrOut, hgrIn, pressOut;
extern uint8_t owcount;
extern uint8_t owids[2][8];

extern uint8_t indexOneWire();
extern float daltemp[2];

extern float enoTemp[], enoHumm[];
extern uint8_t enoAcc[], enoStat[];
extern uint16_t enoVolt[],enoPres[];
extern uint32_t lastEno[];
extern struct thermoData *serverCurData;
extern uint8_t getDallas();
uint8_t getTemperatures();
extern void prepareTempsForServer(struct thermoData *td);
extern void WiFiGetMac(uint8_t *mac);
extern uint8_t WiFiGetIP(char *buf);
extern void setStationMac(uint8_t *newPeerMac);
extern void deinitEspNow();
extern uint8_t initEspNow();
extern void startServer();
extern void stopServer();
extern void initServer();
extern uint8_t imAp;
extern void initIR();
extern uint8_t IRloop();
extern portMUX_TYPE mux;
#define Procure portENTER_CRITICAL(&mux)
#define Vacate portEXIT_CRITICAL(&mux)


extern SemaphoreHandle_t prefSemaphore;

extern void LockPrefs();
extern void UnlockPrefs();

extern uint32_t lastCode, lastCodeMillis;
extern uint8_t haveIR,monitorIR,lastCodeKey;

extern const char * const monthNM[];


#define BADGEO (200 * 3600)

extern int32_t parseGeo(Print &Ser, char *par, char *dop, int mxd);

extern const char *getToken(char **line);

#define SERVER_PORT 8080

extern void startTelnet();
extern void telnetLoop();
extern void resetESP();

extern uint8_t properAvahi(const char *par, char *whereto);

struct cmdBuffer {
    uint8_t bufpos;
    uint8_t blank;
    char buffer[82];
};

extern void addCmdChar(Print &ser,struct cmdBuffer *buf, uint8_t znak, uint8_t fromTelnet = 0);
extern void printMac(Print &ser, const char *pfx, const uint8_t *mac);
extern bool haveWeather;

//pinology
#ifdef  CONFIG_IDF_TARGET_ESP32S3
extern struct pinPrefs {
    uint8_t ir, scl, sda, wclk, bclk, dout, dht, ds, btn, btng;
    }
    pinPrefs, pinPrefsW;
#endif

extern int8_t haveEE;
extern uint8_t initEvents();
extern const char *getEventName(uint8_t tomorrow);
extern const int monlens[];

extern uint8_t RTC_DATA_ATTR debugMode;

#define I2DEVOK(n) (i2cdevices[((n) >> 3) & 15] & (1 << ((n) & 7)))


extern uint8_t haveCityFinder;
#if defined(BOARD_HAS_PSRAM) && (BOARD_HAS_BIG_FLASH == 1)
#define USE_CITY_FINDER 1
extern void initCityFinder();
extern uint32_t extractCode(const char *cs);
extern uint16_t findByCode(uint32_t kod);
extern void makeCityString(char *buf,int city);
extern int findByNamePart(const uint8_t *pat,int bstart);

#endif


#ifdef ENABLE_OTA

extern uint8_t otaPerform(Print &Ser, char *par);
extern int runUpdateOta(Print &Ser);
extern int checkOta();
extern uint8_t doOtaUpdate;

#endif

extern void sayCWait(const char *c);
extern int sayCondPercent(int pc);
#endif


