#include <Arduino.h>
#include "Pogoda.h"

int8_t haveEE = -1;
#if defined(BOARD_HAS_PSRAM) || defined (UNIT_USE_EVENTS)

#include <I2C_eeprom.h>

struct eEvent *eEvents;
#define EVENT_OFFSET 4
#define EVENT_ARRAY_SIZE (EVENT_COUNT * sizeof(struct eEvent))

#define EVENT_MAGIC 0xdabef012UL

static I2C_eeprom *eeprom;

static uint8_t eeAddress;

uint8_t initEvents()
{
    uint32_t magic;
    if (haveEE >= 0) return haveEE;
    for (eeAddress = 0x50; eeAddress < 0x58; eeAddress++) {
        if (I2DEVOK(eeAddress)) break;
    }
    if (eeAddress > 0x57) return haveEE=0;
    eeprom = new I2C_eeprom(eeAddress,I2C_DEVICESIZE_24LC32);
    if (!eeprom->begin()) {
        Serial.printf("Brak EEPROM\n");
        return haveEE=0;
    }
    Serial.printf("EEPROM %02X\n",eeAddress);
    eEvents=(struct eEvent *)
#ifdef BOARD_HAS_PSRAM
    ps_malloc
#else
    malloc
#endif
    (EVENT_ARRAY_SIZE);
    eeprom->readBlock(0,(uint8_t *)&magic,4);
    if (magic != EVENT_MAGIC) {
        memset((void *)eEvents,0,EVENT_ARRAY_SIZE);
        eeprom->updateBlock(EVENT_OFFSET,(uint8_t *)eEvents,EVENT_ARRAY_SIZE);
        magic = EVENT_MAGIC;
        eeprom->writeBlock(0,(uint8_t *)&magic, 4);
        Serial.printf("Nowa tabela wydarzeń\n");
    }
    else {
        eeprom->readBlock(EVENT_OFFSET,(uint8_t *)eEvents,EVENT_ARRAY_SIZE);
        Serial.printf("Istniejąca tabela wydarzeń\n");
    }
    return haveEE=1;
}

uint8_t eventInPast(struct eEvent *ev)
{
    if (ev->year < _year - 2000) return 1;
    if (ev->year > _year - 2000) return 0;
    if (ev->month < _month) return 1;
    if (ev->month > _month) return 0;
    return (ev->day < _day);
}

static int8_t findFreeEvent()
{
    uint8_t evts[EVENT_COUNT];
    int i,n;
    if (!haveEE) return -1;
    getLocalTime();
    for (i=n=0;i<EVENT_COUNT;i++) {
        if (!eEvents[i].pos || eventInPast(&eEvents[i])) {
            evts[n++] = i;
        }
    }
    if (!n) return -1;
    return evts[random(n)];
}

static int _insertEvent(int day, int mon, int year,const char *txt)
{
    int i;
    if (!haveEE) return -2;
    int y=year % 2000;
    if (strlen(txt) > 43) return -1;
    for (i=0;i<EVENT_COUNT;i++) {
        if (eEvents[i].pos &&
            eEvents[i].day == day &&
            eEvents[i].month == mon &&
            eEvents[i].year == y &&
            eEvents[i].data[0]) return -i-8; // event exisis
    }
    i=findFreeEvent();
    if (i < 0 || i >= EVENT_COUNT) return -2; // brak miejsca
    eEvents[i].pos = i+1;
    eEvents[i].day = day;
    eEvents[i].month = mon;
    eEvents[i].year = y;
    strcpy(eEvents[i].data,txt);
    //storeEventEE(i);
    return i;
}

int insertEvent(int day, int mon, int year,const char *txt)
{
    if (!haveEE) return -2;
    LockPrefs();
    int rc=_insertEvent(day,mon,year,txt);
    UnlockPrefs();
    return rc;
}

    
const char *getEventName(uint8_t tomorrow)
{
    uint8_t d, m; uint16_t y;int i;
    if (!haveEE) return NULL;
    if (tomorrow) {
        tomorrowDay(&d, &m, &y);
    }
    else {
        d=_day;
        m=_month;
        y=_year;
    }
    y -= 2000;
    for (i=0;i<EVENT_COUNT;i++) {
        if (eEvents[i].pos &&
            eEvents[i].day == d &&
            eEvents[i].month == m &&
            eEvents[i].year == y &&
            eEvents[i].data[0]) return eEvents[i].data;
        
    }
    return NULL;
}

static int getPD(char **par)
{
    if (!par || !*par) return -1;
    const char *tok = getToken(par);
    if (!isdigit(*tok)) return -1;
    int n = strtol(tok, (char **)&tok,10);
    if (*tok) return -1;
    return n;
}

static uint8_t isGoodDate(int *d, int *m, int *y, char **par)
{
    int _d, _m, _y;
    if ((_d=getPD(par)) < 0) return 0;
    if ((_m=getPD(par)) < 0) return 0;
    if ((_y=getPD(par)) < 0) return 0;
    if (_m < 1 || _m >12) return 0;
    _y = _y % 100;
    int md=monlens[_m+1];
    if (_m == 2 && !(_y & 3)) md += 1;
    if (_d<1 || _d >md) return 0;
    *d=_d;
    *m=_m;
    *y=_y;
    return 1;
}

uint8_t dateInPast(int d, int m, int y)
{
    if (y < _year - 2000) return 1;
    if (y > _year - 2000) return 0;
    if (m < _month) return 1;
    if (m> _month) return 0;
    return (d < _day);
}

static int8_t eventByDate(int d, int m, int y)
{
    int i;
    if (!haveEE) return -1;
    for (i=0;i<EVENT_COUNT;i++) {
        if (eEvents[i].pos &&
            eEvents[i].day == d &&
            eEvents[i].month == m &&
            eEvents[i].year == y &&
            eEvents[i].data[0]) return i;
    }
    return -1;
}

void storeEvts()
{
    if (!haveEE) return;
    LockPrefs();
    eeprom->updateBlock(EVENT_OFFSET,(uint8_t *)eEvents,EVENT_ARRAY_SIZE);
    UnlockPrefs();
}
void restoreEvts()
{
    if (!haveEE) return;
    LockPrefs();
    eeprom->readBlock(EVENT_OFFSET,(uint8_t *)eEvents,EVENT_ARRAY_SIZE);
    UnlockPrefs();
}

uint8_t pfsEvent(Print &Ser, char *par)
{
    if (!haveEE) {
        Ser.printf("Brak EEPROM\n");
        return 0;
    }
    if (!par || !*par) {
        int i,n;
        getLocalTime();
        for (i=n=0;i<EVENT_COUNT;i++) {
            if (eEvents[i].day && eEvents[i].pos && !eventInPast(&eEvents[i])) {
                Ser.printf("%d %s %d: %s (%d)\n",
                    eEvents[i].day,
                    monthNM[eEvents[i].month-1],
                    eEvents[i].year + 2000,
                    eEvents[i].data,eEvents[i].pos);
                n+=1;
            }
        }
        if (!n) Ser.printf("Brak wydarzeń w terminarzu\n");
        return 1;
    }
    if (!strcmp(par,"save")) {
        storeEvts();
        Ser.printf("Zapisano terminarz\n");
        return 1;
    }
    if (!strcmp(par,"restore")) {
        restoreEvts();
        Ser.printf("Przywrócono terminarz\n");
        return 1;
    }
    int d, m, y;
    if (!isGoodDate(&d, &m, &y, &par)) {
        Ser.printf("Błędna data\n");
        return 0;
    }

    if (dateInPast(d,m,y)) {
        Ser.printf("Data w przeszłości\n");
        return 0;
    }
    int fnd=eventByDate(d,m,y);
    if (!par || !*par) {
        if (fnd < 0) {
            Ser.printf("Na %d %s %d nie jest nic zaplanowane\n",
                d,monthNM[m-1],y+2000);
            
        }
        else {
            Ser.printf("%d %s %d: %s\n",
                eEvents[fnd].day,
                monthNM[eEvents[fnd].month-1],
                eEvents[fnd].year + 2000,
                eEvents[fnd].data);
            }
        return 1;
    }
    if (!strcmp(par,"del")) {
        if (fnd < 0) {
            Ser.printf("Na %d %s %d nie jest nic zaplanowane\n",
                d,monthNM[m-1],y+2000);
            
            
            
        }
        else {
            Ser.printf("Usuwam wpis: %d %s %d: %s\n",
                eEvents[fnd].day,
                monthNM[eEvents[fnd].month-1],
                eEvents[fnd].year + 2000,
                eEvents[fnd].data);
            LockPrefs();
            eEvents[fnd].day = 0;
            eEvents[fnd].pos = 0;
            UnlockPrefs();
        }
        return 1;
    }
    if (strlen(par) < 5 ) {
        Ser.printf("Za krótki tekst\n");
        return 0;
    }
    fnd=insertEvent(d, m, y, par);
    if (fnd == -1) {
        Ser.printf("Tekst jest za długi\n");return 0;
    }
    if (fnd == -2) {
        Ser.printf("Za duzo wpisów\n");return 0;
    }
    if (fnd <= -8) {
        fnd = 8-fnd;
        Ser.printf("Wpis już istnieje\n");
        return 0;
    }
    if (fnd < 0) {
        Ser.printf("Błąd wewnętrzny\n");
        return 0;
    }
    Ser.printf("Dodano wpis: %d %s %d: %s\n",
                eEvents[fnd].day,
                monthNM[eEvents[fnd].month-1],
                eEvents[fnd].year + 2000,
                eEvents[fnd].data);
    return 1;
}

#else
uint8_t initEvents()
{
    return haveEE=0;
}

const char *getEventName(uint8_t tomorrow)
{
    return NULL;
}
#endif
