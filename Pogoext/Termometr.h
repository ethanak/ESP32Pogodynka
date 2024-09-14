#ifndef TERMOMETR_H
#define TERMOMETR_H


#define MAGIC_EE 0x3279

extern struct prefes {
    uint16_t magic;
    uint16_t sleep;
    uint16_t v42;
    uint8_t pin;
    uint8_t termtyp;
    uint8_t flags;
    uint8_t kludge;
    uint8_t station[6];
} prefes, realPrefes;

#define PFS_HAVE_CHARGER 1
#define PFS_KEEPALIVE 2

#ifndef ESP32
#error Tylko dla ESP32
#endif

extern const uint8_t availPins[];

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define AVAILPINS 4,5,6,7,9,10
#define USE_USB_TEST 1
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define AVAILPINS 3,4,5,6,7,10
#define USE_USB_TEST 1
#else
#define AVAILPINS 4,5,16,17,18,19,22,23
#endif

extern uint16_t getPower();

extern bool chargeMode;


extern void printMac (uint8_t *mac);
extern void initEN();
extern void  sendLoop();
extern void  serLoop();

enum {
    THR_NONE=0,
    THR_DS,
    THR_DHT11,
    THR_DHT22,
    THR_MAX
    
};


extern float tempOut, hgrOut;
extern void pfsSleep(char *s);
extern void pfsCalib(char *s);
extern void pfsTerm(char *s);
extern void pfsPin(char *s);
extern void pfsSave(char *s);
extern void pfsPeer(char *s);
extern void pfsKeep(char *s);
extern void pfsCharger(char *s);


extern void initPFS();
extern uint8_t readTemp();
extern uint8_t sentOK;
extern void dataSend();

extern unsigned long ARDUINO_ISR_ATTR seconds();
extern uint32_t lastActivity;

extern void printMac (const char *pfx,const uint8_t *mac);

extern const char *getToken(char **line);

#endif
