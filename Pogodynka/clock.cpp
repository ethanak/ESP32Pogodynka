#include <Arduino.h>
#include "Pogoda.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Wire.h>



static WiFiUDP Udp;
#define LOCUPORT 8765
#define NTPDELAY (60UL*60UL*1000UL)



const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

static uint32_t pckTime;
static uint8_t ntpState;
static time_t newUdpTime;
bool timerInitialized = false;
static uint32_t timerControl=0;

static bool UdpOK=false;
void clockAtConnect()
{
    if (UdpOK) return;
    Udp.begin(LOCUPORT);
    ntpState=0;
    //printf("UDP start\n");
    UdpOK=true;

}
void clockAtDisConnect()
{
    if (!UdpOK) return;
    UdpOK=false;
    Udp.stop();
    ntpState=0;
    //printf("UDP stop\n");
}

static bool sendNTPpacket()
{
    IPAddress address;
    bool x=WiFi.hostByName("pl.pool.ntp.org", address);
    //if (x) return false;
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
    return true;
    
}

static bool readNtp()
{
    int size = Udp.parsePacket();
    if (size < NTP_PACKET_SIZE) {
        //if (size) printf("Bad size %d %d\n",size,NTP_PACKET_SIZE);
        return false;
    }
    Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
    unsigned long secsSince1900;
    // convert four bytes starting at location 40 to a long integer
    secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
    secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
    secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
    secsSince1900 |= (unsigned long)packetBuffer[43];
    newUdpTime=secsSince1900 - 2208988800UL;
    return true;
}

bool haveClock()
{
    return timerInitialized;
}

void rtcLoop()
{
    if (clockHardStatus == 2) return;
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    switch(ntpState) {
        case 0:
        if (UdpOK && !timerInitialized || millis() - timerControl > NTPDELAY) ntpState = 1;
        break;

        case 1:
        //printf("Sending packet\n");
        if (!sendNTPpacket()) ntpState = 0;
        else {
            pckTime = millis();
            ntpState = 2;
        }
        break;

        case 2:
        if (millis() - pckTime >= 3000UL) {
            printf("Packet timeout\n");
            ntpState=0;
            break;
        }
        if (readNtp()) {
            //printf("Received packet\n");
            if (clockHardStatus == 1) {
                writeHardClock(newUdpTime);
                clockHardStatus = 2;
            }
            timerInitialized=1;
            timerControl = millis();
            ntpState=0;
            //printf("Packet OK\n");
        }
        break;
    }
    if (newUdpTime) {
        setTime(newUdpTime);
        newUdpTime=0;
    }
}

static int rtcStatus=-1;
static time_t oldUnixTime=0;
time_t unixTime;

static int isDST(int month, int day, int hour, int dow)
{
    if (month < 3 || month > 10)  return 0;
    if (month > 3 && month < 10)  return 1;
    int previousSunday = day - dow;
    if (month == 3) {
        return (previousSunday >= 25 && (dow || hour >= 2));
    }
    if (month == 10) {
        return (previousSunday < 25 || (!dow && day >=25 && hour < 2));

    }
    return 0;
}


int _minute, _hour, _day, _month, _year, _dayoweek, _second,_yday;
const int monlens[]={0,0,31,28,31,30,31,30,31,31,30,31,30,31};

uint8_t properDate(uint8_t day, uint8_t month)
{
    if (month < 1 || month > 12) return 1;
    if (month == 2 && day == 29) return 0;
    if (day < 1 || day > monlens[month+1]) return 2;
    return 0;
}

int getDOY(int day, int mon, int year)
{
    int i, localDOY = day + ((mon > 2 && !(year & 3)) ? 1 : 0);
    for (int i=1; i< mon; i++) localDOY += monlens[i+1]; 
    return localDOY;
}

static int wielkanoc(int year)
{
    if (year == 2049) return 108;
    if (year == 2076) return 118;
    static const int rA=24;
    static const int rB=5;
    int a = year % 19;
    int b = year % 4;
    int c = year % 7;
    int d = (a * 19 + rA) % 30;
    int e = (2 * b + 4 * c + 6 * d + rB) % 7;
    return ((year % 4) ? 81:82) + d + e;
}

#define HOLLY_FREE 1
#define HOLLY_MOV 2
#define HOLLY_CHURCH 4
const struct hollyDay {
    uint8_t day;
    uint8_t mon;
    int8_t coffset;
    uint8_t flags;
    const char *name;
} holydays[] ={
{1,1,0,HOLLY_FREE,"Nowy rok"},
{6,1,0,HOLLY_FREE,"Trzech Króli"},
{1,5,0,HOLLY_FREE,"Święto Pracy"},
{2,5,0,0,"Dzień Flagi"},
{3,5,0,HOLLY_FREE,"Święto Trzeciego Maja"},
{15,8,0,HOLLY_FREE | HOLLY_CHURCH,"Święto Wniebowzięcia Marii Panny"},
{15,8,0,HOLLY_FREE,"Dzień Wojska Polskiego"},
{1,11,0,HOLLY_FREE, "Wszystkich Świętych"},
{11,11,0,HOLLY_FREE,"Święto Niepodległości"},
{24,12,0,HOLLY_FREE,"Wigilia Bożego Narodzenia"},
{25,12,0,HOLLY_FREE,"Boże Narodzenie"},
{26,12,0,HOLLY_FREE,"Drugi dzień Bożego Narodzenia"},
{0,0,-52,HOLLY_MOV,"Tłusty Czwartek"},
{0,0,-46,HOLLY_MOV | HOLLY_CHURCH,"Popielec"},
{0,0,-7,HOLLY_MOV | HOLLY_CHURCH,"Niedziela Palmowa"},
{0,0,-2,HOLLY_MOV | HOLLY_CHURCH,"Wielki Piątek"},
{0,0,-1,HOLLY_MOV,"Wielka Sobota"},
{0,0,0,HOLLY_MOV | HOLLY_FREE,"Wielkanoc"},
{0,0,1,HOLLY_MOV | HOLLY_FREE,"Poniedziałek Wielkanocny"},
{0,0,42,HOLLY_MOV | HOLLY_CHURCH,"Święto Wniebowstąpienia"},
{0,0,49,HOLLY_MOV | HOLLY_CHURCH,"Zesłanie Ducha Świętego"},
{0,0,60,HOLLY_MOV | HOLLY_FREE, "Boże Ciało"}};

#define HOLLYCNT (sizeof(holydays) / sizeof(holydays[0]))

static const char *getHollyDay(int day, int mon, int year, int doy)
{
    int i;
    for (i=0;i<HOLLYCNT;i++) {
        if (day != holydays[i].day ||
            mon != holydays[i].mon) continue;
        if (!(weaPrefs.flags & SPKWEA_CHURCH) && (holydays[i].flags & HOLLY_CHURCH)) continue;
        return holydays[i].name;
    }
    int w = wielkanoc(year);
    for (i=0;i<HOLLYCNT;i++) if (holydays[i].flags & HOLLY_MOV) {
        if (!(weaPrefs.flags & SPKWEA_CHURCH) && (holydays[i].flags & HOLLY_CHURCH)) continue;
        if (doy == w + holydays[i].coffset) return holydays[i].name;
    }
    return NULL;
    
}

void tomorrowDay(uint8_t *day, uint8_t *month, uint16_t *year)
{
    struct tm ltm;
    memset(&ltm, 0, sizeof(ltm));
    ltm.tm_year=_year - 1900;
    ltm.tm_mon=_month-1;
    ltm.tm_mday=_day+1;
    ltm.tm_hour=12;
    ltm.tm_min=0;
    ltm.tm_sec=0;
    mktime(&ltm);
    *day = ltm.tm_mday;
    *month = ltm.tm_mon+1;
    if (year) *year = ltm.tm_year+1900;
    
}


const char *getHolly(uint8_t tomorrow)
{
    //getLocalTime();
    if (!tomorrow) {
        return getHollyDay(_day, _month, _year, _yday);
    }
    uint8_t day, mon;uint16_t year;
    tomorrowDay(&day, &mon, &year);
    return getHollyDay(day, mon, year,
        getDOY(day, mon, year));
}

const char *getCalEvent(uint8_t tomorrow)
{
    //getLocalTime();
    uint8_t d=_day;
    uint8_t m=_month;
    if (tomorrow) tomorrowDay(&d, &m);
    int i;
    for (i=0;i<32;i++) if (calPrefs.days[i].day == d && calPrefs.days[i].month == m) {
        if (calPrefs.days[i].text[0]) return calPrefs.days[i].text;
    }
    return NULL; 
    
}



bool getLocalTime()
{
    if (!timerInitialized) return false;
    if (clockHardStatus == 2) {
        unixTime = readHardClock();
        if (!unixTime) {
            clockHardStatus = 0;
            unixTime = now();
        }
        else {
            setTime(unixTime);
        }
    }
    else unixTime = now();
    
    uint32_t loctime = unixTime + 60 * clkPrefs.offset;
    if (clkPrefs.useDST && isDST(month(loctime), day(loctime), hour(loctime),dayOfWeek(loctime))) {
        loctime += 3600;
    }
    _second = second(loctime);
    _minute = minute(loctime);
    _hour = hour(loctime);
    _day = day(loctime);
    _month = month(loctime);
    _year = year(loctime);
    _yday = getDOY(_day, _month, _year);
    _dayoweek = dayOfWeek(loctime) % 7;
    return true;
}


/* DS3231 code */

static uint8_t decToBcd(uint8_t val) {
// Convert normal decimal numbers to binary coded decimal
    return ( (val/10*16) + (val%10) );
}

static uint8_t bcdToDec(uint8_t val) {
// Convert binary coded decimal to normal decimal numbers
    return ( (val/16*10) + (val%16) );
}

#define CLOCK_ADR 0x68

uint8_t writeHardClock(time_t time)
{
    struct tm ltm;
    gmtime_r(&time, &ltm);
    uint8_t abuf[7];
    abuf[0] = decToBcd(ltm.tm_sec);
    abuf[1] = decToBcd(ltm.tm_min);
    abuf[2] = decToBcd(ltm.tm_hour);
    abuf[3] = decToBcd(ltm.tm_wday+1);
    abuf[4] = decToBcd(ltm.tm_mday);
    abuf[5] = decToBcd(ltm.tm_mon+1);
    abuf[6] = decToBcd(ltm.tm_year % 100);
    Wire.beginTransmission(CLOCK_ADR);
    Wire.write(0);
    Wire.write(abuf,7);
    Wire.endTransmission();
    return readHardClock() != 0;
    
    
}

uint32_t readHardClock()
{
    uint8_t dsbuf[7];
    uint8_t i;
    Wire.beginTransmission(CLOCK_ADR);
    Wire.write(0);
    if (Wire.endTransmission()) return 0;
    Wire.requestFrom(CLOCK_ADR,7);
    for (i=0;i<7;i++) dsbuf[i]=bcdToDec(Wire.read());
    struct tm ltm;
    memset(&ltm, 0, sizeof(ltm));
    ltm.tm_year=100+dsbuf[6];
    ltm.tm_mon=dsbuf[5]-1;
    ltm.tm_mday=dsbuf[4];
    ltm.tm_hour=dsbuf[2];
    ltm.tm_min=dsbuf[1];
    ltm.tm_sec=dsbuf[0];
    uint32_t uy=mktime(&ltm);
    return uy;
}

uint8_t checkHardClock()
{
    if (!thrPrefs.ds3231) return 0;
    Wire.beginTransmission(CLOCK_ADR);
    Wire.write(6);
    if (Wire.endTransmission()) return 0;
    Wire.requestFrom(CLOCK_ADR,1);
    uint8_t yr=bcdToDec(Wire.read());
    if (yr < 20) return 1;
    return 2;
}

const char *synchronizeClock()
{
    if (!UdpOK) return "Brak połączenia z serwerem czasu";
    if (ntpState) return "Trwa automatyczna synchronizacja";
    sendNTPpacket();
    uint32_t m0 = millis();
    while(!readNtp()) {
        if (millis() - m0 > 3000) {
            return "Przekroczono czas oczekiwania na odpowiedź";
        }
        delay(20);
    }
    if (newUdpTime) {
        setTime(newUdpTime);
        if (thrPrefs.ds3231 > 0) {
            if (writeHardClock(newUdpTime)) clockHardStatus = 2;
        }
        newUdpTime=0;
    }
    return NULL;
}
uint8_t mustSyncClock=0;
const char *syncClockResponse;
const char *srvSynchronizeClock()
{
    uint32_t syncClockStart=millis();
    mustSyncClock = 1;
    while(mustSyncClock) {
        if (millis() - syncClockStart > 4000) {
            return "Błąd synchronizacji";
        }
        delay(50);
    }
    if (syncClockResponse) return syncClockResponse;
    return "Zegar zsynchronizowany";
}
