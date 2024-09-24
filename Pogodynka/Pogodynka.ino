#include <WiFi.h>
#include <stdio.h>
#include "Pogoda.h"
#include <ESP32Gadacz.h>
#include <Bounce2.h>
#include <Wire.h>
#include <math.h>

uint8_t RTC_DATA_ATTR debugMode=0;

#ifdef  CONFIG_IDF_TARGET_ESP32S3
int serial_puts(void *cookie,const char *c, int size)
{
    (void)cookie;
    Serial.write(c,size);
    return size;
}

char stdout_buf[128];
#endif

uint8_t imAp = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE prefMux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t prefSemaphore = NULL;

char **scannedWiFi;

void scannet(uint8_t store=0)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);
    delay(500);
    Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    printf("Scan result %d\n",n);
    if (store) scannedWiFi=(char **)calloc((n+1), sizeof(char *));
    int i;
    for (i=0;i<n;i++) {
        if (store) {
            char buf[128];
            sprintf(buf,"%03d:%s", abs(WiFi.RSSI(i)),WiFi.SSID(i).c_str());
            scannedWiFi[i]=strdup(buf);
        }
            
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i).c_str());
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
    }
   
}

uint8_t i2cdevices[16];
void i2cscan() {
  int nDevices = 0;
  memset(i2cdevices,0,sizeof(i2cdevices));
  Serial.println("Skanuję I2C...");
  
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      i2cdevices[(address >> 3) & 15] |= 1<<(address & 7);
      Serial.printf("I2C: znaleziono 0x%02X\n", address);
      

      ++nDevices;
    } else if (error == 4) {
      Serial.printf("I2C: błąd 0x%02X\n", address);
    }
  }
  if (nDevices == 0) {
    Serial.println("Brak podłączonych urządzeń I2C\n");
  } else {
    Serial.println("Skanowanie zakończone\n");
  }
}


int I2C_ClearBus(uint8_t slow) {

    Wire.end();
  pinMode(PIN_SDA, INPUT_PULLUP); // Make PIN_SDA (data) and PIN_SCL (clock) pins Inputs with pullup.
  pinMode(PIN_SCL, INPUT_PULLUP);

  delay(slow ? 2500 : 100);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(PIN_SCL) == LOW); // Check is PIN_SCL is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master. 
    return 1; //I2C bus error. Could not clear PIN_SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(PIN_SDA) == LOW);  // vi. Check PIN_SDA input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If PIN_SDA is Low,
    clockCount--;
  // Note: I2C bus is open collector so do NOT drive PIN_SCL or PIN_SDA high.
    pinMode(PIN_SCL, INPUT); // release PIN_SCL pullup so that when made output it will be LOW
    pinMode(PIN_SCL, OUTPUT); // then clock PIN_SCL Low
    delayMicroseconds(10); //  for >5us
    pinMode(PIN_SCL, INPUT); // release PIN_SCL LOW
    pinMode(PIN_SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5us
    // The >5us is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(PIN_SCL) == LOW); // Check if PIN_SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for PIN_SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(PIN_SCL) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. PIN_SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(PIN_SDA) == LOW); //   and check PIN_SDA input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. PIN_SDA data line held low
  }

  // else pull PIN_SDA line low for Start or Repeated Start
  pinMode(PIN_SDA, INPUT); // remove pullup.
  pinMode(PIN_SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5us
  pinMode(PIN_SDA, INPUT); // remove output low
  pinMode(PIN_SDA, INPUT_PULLUP); // and make PIN_SDA high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5us
  pinMode(PIN_SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(PIN_SCL, INPUT);
  return 0; // all ok
}

uint8_t conSpoken = 0;

Bounce bncBut=Bounce();
//Bounce bncCfg=Bounce();

void gadaczGetPrefs()
{
    Gadacz::setSpeed(spkPrefs.tempo);
    Gadacz::setVolume(spkPrefs.volume);
    Gadacz::setContrast(spkPrefs.contrast);
    Gadacz::setPitch(spkPrefs.pitch);
}

void gadaczSayDemo()
{
    gadaczGetPrefs();
    Gadacz::saycst("To jest tekst wypowiedziany przy zadanych parametrach");
}

uint8_t clockHardStatus;

void setup_ap()
{
    Gadacz::saycst("Skanuję sieci");
    scannet(1);
    Gadacz::waitAudio();
    Gadacz::saycst("Uruchamiam serwer");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(pogoApSSID, pogoApPASS);
    initServer();
    startServer();
    Serial.printf("Aby uzyskać pomoc wpisz help\n");
}

uint8_t haveCityFinder;
static const char * const wifimsg[] = {
    "rozłączono",
    "połączono",
    "uzyskano IP",
    "brak ustawień"
};

volatile uint8_t wifiShowMsg=0;

void printWiFiMsg()
{
    int wsmsg;
    Procure;
    wsmsg=wifiShowMsg;
    wifiShowMsg=0;
    Vacate;
    if (wsmsg) {
        Serial.printf("WiFi: %s\n", wifimsg[wsmsg-1]);
    }
}

static void initI2C()
{
    Wire.begin(PIN_SDA, PIN_SCL);
    i2cscan();
    int rclr = 0;
    if (!I2DEVOK(0x68) && thrPrefs.ds3231) rclr = 2;
    else {
        if ((thrPrefs.trmin == DEVT_BMP180 || thrPrefs.trmin == DEVT_BMP280) &&
            thrPrefs.i2cin &&  !I2DEVOK(thrPrefs.i2cin)) rclr |= 1;
        else if ((thrPrefs.trmex == DEVT_BMP180 || thrPrefs.trmex == DEVT_BMP280) &&
            thrPrefs.i2cex &&  !I2DEVOK(thrPrefs.i2cex)) rclr |= 1;
        else if (thrPrefs.ds3231) {
            int i;
            for (i=0x50; i<0x57;i++) if (I2DEVOK(i)) break;
            if (i > 0x57) rclr |= 1;
        }
    }
    if (!rclr) {
        return;
    }
    Serial.printf("Czyszczę magistralę I2C (%s)\n", (rclr &2) ? "powoli" : "szybko");
    I2C_ClearBus(rclr & 2);
    Wire.begin(PIN_SDA, PIN_SCL);
    i2cscan();
}

void setup()
{
    Serial.begin(115300);
#ifdef BOARD_HAS_PSRAM
    if (psramInit()) {
        printf("PSRAM OK, size %d\n", ESP.getFreePsram());
    }
#endif
#ifdef  CONFIG_IDF_TARGET_ESP32S3
// te trzy linijki zeby printf wiedział gdzie ma pisać
    fclose(stdout);
    stdout = fwopen(NULL, serial_puts);
    setvbuf(stdout, stdout_buf, _IOLBF, 128);
    delay(1500);
    //scannet();
#endif
    prefSemaphore = xSemaphoreCreateBinary();
    UnlockPrefs();
    initPrefs();
#ifdef PIN_BUTTON_GND
    if (PIN_BUTTON_GND) {
        pinMode(PIN_BUTTON_GND, OUTPUT);
        digitalWrite(PIN_BUTTON_GND,0);
    }
#endif
    bncBut.attach(PIN_BUTTON, INPUT_PULLUP);
    bncBut.interval(25);
    bncBut.update();
    Gadacz::begin(WCLK_PIN, BCLK_PIN, DOUT_PIN);
    gadaczGetPrefs();
    if (!netPrefs.ssid[0]) imAp=1;
    else if (!bncBut.read()) {
        delay(100);
        bncBut.update();
        if (!bncBut.read()) imAp=1;
    }
    if (imAp) {
        setup_ap();
        return;
    }
    
    Gadacz::saycst("Cześć");
    //scannet(1); // nierpotrzebne
    initI2C();
    clockHardStatus=checkHardClock();
    if (clockHardStatus == 2) timerInitialized = 1;
    Serial.printf("RTC status: %d\n",clockHardStatus);
#ifdef USE_CITY_FINDER
    initCityFinder();
#endif
    initEvents();
    //Serial.printf("Events %d\n",);
    //Serial.printf("City Finder %d\n",haveCityFinder);
    initWiFi();
    initServer();
    printWiFiMsg();
    initIR();
    printWiFiMsg();
    initThermo();
    printWiFiMsg();
    Serial.printf("Gotowy do pracy - aby uzyskać pomoc wpisz help\n");
    //Serial.flush();
    //Serial.printf("Init thermo %d\n",initThermo());
    
}

static const char *const weekdays[]={
    "sobota","niedziela","poniedziałek","wtorek",
    "środa", "czwartek","piątek"};
static const char *const monsy[]={
    "I","II","III","IV","V","VI","VII","VIII","IX","X","XI","XII"};
char *timeString(char *buf, bool lng)
{
    if (!lng) {
        return buf+sprintf(buf,"Jest %02d:%02d",_hour,_minute);
    }
    return buf+sprintf(buf,"jest %02d:%02d, %s, %d.%s",
        _hour,_minute,weekdays[_dayoweek],_day,monsy[_month-1]);
        
}

uint8_t buttonLock=0;
uint8_t pendingDbl=0;
uint8_t externCmd;
//uint32_t pendingStart;
uint8_t getCommand()
{
    if (bncBut.fell()) {
        if (Gadacz::isSpeaking()) {
            buttonLock =1;
            return CMD_STOP;
        }
        if (pendingDbl) {
            buttonLock = 1;
            pendingDbl = 0;
            return CMD_PARAMS;
        }
        buttonLock=0;
        return CMD_NONE;
    }
    
    if (!buttonLock && !bncBut.read() && bncBut.duration() >= 1500) {
        buttonLock = 1;
        return CMD_WEATHER;
    }
    if (bncBut.rose()) {
        if (buttonLock) {
            buttonLock = 0;
            return CMD_NONE;
        }
        if (bncBut.previousDuration() < 500) {
            pendingDbl = 1;
            return CMD_NONE;
        }
        return CMD_DATE;
    }
    int ecmd=externCmd;
    if (ecmd) {
        pendingDbl=0;
        if (Gadacz::isSpeaking()) return CMD_STOP;
        externCmd=0;
        return ecmd;
    }
    
    uint8_t _lc, _lch;
    static uint32_t _lcm=0;
    static uint32_t _pcm=0;
    Procure;
    _lc = lastCodeKey;
    _pcm=_lcm;
    _lcm= lastCodeMillis;
    _lch= haveIR;
    haveIR=0;
    Vacate;
    if (monitorIR || !_lc || !_lch) {
        if (pendingDbl) {
            if (bncBut.duration() <300) {
                return CMD_NONE;
            }
            pendingDbl = 0;
            return CMD_HOUR;
        }
        return CMD_NONE;
    }
    if (_lc >4 && _lc < 9) return _lc;
    if (Gadacz::isSpeaking() && _lc == 4) return CMD_STOP;
#if 0
    //Serial.printf("PCM %d\n",millis() - _pcm);
    if (millis() - _pcm >= 2000) {
        if (!Gadacz::isSpeaking() && _lc < 4) return _lc+1;
        //tu wymaga trochę więcej roboty...
        //błędnie reaguje na repeat z pilota
        //if (_lc < 4) return CMD_STOP;
        if (_lc > 4) return _lc;
    }
#else
    // na razie bez zatrzymania tym samym klawiszem
    // potem się pomyśli
    if (!Gadacz::isSpeaking() && _lc < 4) return _lc+1;
#endif
    return CMD_NONE;
}


char timebuf[1024];
uint8_t ctlSpeakWeather = 0;
uint8_t ctlSpeakTemp = 0;
uint8_t ctlSpeakAdr = 0;
void rmtCmd(int cmd)
{
    bool done = false;
    int v = Gadacz::getVolume();
    int s = Gadacz::getSpeed();
    switch(cmd) {
        case CMD_VOLDOWN:
        if (v > 0) {
            Gadacz::setVolume(spkPrefs.volume = v-1);
            Gadacz::sayfmt("Głośność %d",spkPrefs.volume);
        }
        break;

        case CMD_VOLUP:
        if (v < 24) {
            Gadacz::setVolume(spkPrefs.volume = v+1);
            Gadacz::sayfmt("Głośność %d",spkPrefs.volume);
        }
        break;

        case CMD_SPDOWN:
        if (s > 0) {
            Gadacz::setSpeed(spkPrefs.tempo = s-1);
            Gadacz::sayfmt("Prędkość %d",spkPrefs.tempo);
        }
        break;

        case CMD_SPDUP:
        if (s < 24) {
            Gadacz::setSpeed(spkPrefs.tempo = s+1);
            Gadacz::sayfmt("Prędkość %d",spkPrefs.tempo);
        }
        break;

        case CMD_STORE:
        storeGadacz();
        Gadacz::saycst("Zapisano ustawienia mowy\n");
        break;
    }
}

static uint8_t akuSaid = 0, akuSaidCntr=0;
static uint32_t akuSaidTime;

char *fmtServerPort(char *whereto, uint32_t adr, uint16_t port)
{
    whereto += sprintf(whereto,"Adres serwera: http dwukropek ukośnik ukośnik, %d kropka, %d kropka, %d kropka, %d, dwukropek ",
                adr & 255, (adr >> 8) & 255, (adr >> 16) & 255, (adr >>24) & 255);
    if (port < 1000) 
            whereto+=sprintf(whereto,"%d",port);
    else
            whereto+=sprintf(whereto,"%d %02d",port / 100, port % 100);
    return whereto;
}

void loop_ap()
{
    extern volatile uint8_t restartAPServer;
    bncBut.update();
    if (bncBut.fell()) {
        strcpy(timebuf,"nazwa sieci: " pogoApSSID ", hasło: " pogoApPASS ". ");
        fmtServerPort(timebuf+strlen(timebuf),WiFi.softAPIP(),SERVER_PORT);
        Gadacz::say(timebuf);
    }
    if (restartAPServer) {
        delay(200);
        ESP.restart();
    }
    serLoop(Serial);
    delay(10);
}



void loop()
{
    if (imAp) {
        loop_ap();
        return;
    }
    printWiFiMsg();
    int glt = 0;
#ifdef ENABLE_OTA
    if (doOtaUpdate) {
        doOtaUpdate = 0;
        runUpdateOta(Serial);
        bncBut.update();
        return;
    }
#endif
    
    bncBut.update();
    if (!conSpoken && haveClock()) {
        extern time_t unixTime;
        getLocalTime();
        randomSeed((uint32_t)unixTime);
        conSpoken=1;
        Gadacz::waitAudio();
        Gadacz::say("Gotowy do pracy");
    }
    if (akuSaid && (millis() - akuSaidTime > 3600000UL || akuSaidCntr > 5))  akuSaid = akuSaidCntr=0;
    int cmd=getCommand();
    if (cmd && debugMode) Serial.printf("CMD %d\n",cmd);
    switch(cmd) {
        case CMD_STOP:
        Gadacz::stop();
        ctlSpeakWeather = 0;
        ctlSpeakTemp = 0;
        ctlSpeakAdr = 0;
        break;
        case CMD_HOUR:
        ctlSpeakWeather = 0;
        glt = 1;
        break;

        case CMD_DATE:
        ctlSpeakWeather = 0;
        glt = 2;
        break;

        case CMD_WEATHER:
        glt = 3;
        ctlSpeakWeather = 1;
        break;

        case CMD_VOLDOWN:
        case CMD_VOLUP:
        case CMD_SPDOWN:
        case CMD_SPDUP:
        case CMD_STORE:
        glt=0;
        ctlSpeakWeather=0;
        ctlSpeakTemp=0;
        Gadacz::stop();
        rmtCmd(cmd);
        break;

        case CMD_PARAMS:
        Gadacz::stop();
        ctlSpeakWeather = 0;
        ctlSpeakTemp = 0;
        ctlSpeakAdr = 1;
        
    }
        
    if (glt) {
        
        int tin;
        int trmpo, trmpi,presi;
        akuSaidCntr++;

        if (getLocalTime()) {
            timeString(timebuf,glt > 1);
            Gadacz::say(timebuf);
        }
        timebuf[0]=0;
        tin = getTemperatures();
        if (debugMode) Serial.printf("Temp flags = %02x\n",tin);
        if (glt < 2) tin &= THV_TEMP | THV_ETEMP;//5 | 16;
        else {
            if (!(weaPrefs.flags & SPKWEA_PRESS))
                tin &= ~(THV_PRES | THV_EPRES);
            if (!weaPrefs.flags & SPKWEA_HYGROUT) tin &= ~THV_EHGR;
            if (!weaPrefs.flags & SPKWEA_HYGRIN) tin &= ~THV_HGR;
        }
        
        if (tin & THV_ETEMP) trmpo = (int)round(tempOut);
        if (tin & THV_TEMP) trmpi = (int)round(tempIn);
        
        if ((tin & THV_TEMP) && (tin & (THV_PRES | THV_EPRES))) {
            int what = 0;
            if ((tin & (THV_TEMP | THV_PRES)) == (THV_TEMP | THV_PRES)) what = 1;
            else if ((tin & (THV_ETEMP | THV_EPRES)) == (THV_ETEMP | THV_EPRES)) what = 1;
            float f;
            if (!what) tin &= ~ (THV_PRES | THV_EPRES);
            else {
                f= (what == 1) ? pressIn : pressOut;
                if (weaPrefs.flags & SPKWEA_REPRESS) {
                    int h = geoPrefs.elev;
                    if (h == ELEV_AUTO) h=geoPrefs.autoelev;
                    if (h == ELEV_AUTO) tin &= ~(THV_PRES | THV_EPRES);
                    else {
                        f=f * exp((0.03415 * h) /
                            (273.16 + ((what == 1) ? tempIn : tempOut)));
                    }
                }
                if (tin & (THV_PRES | THV_EPRES)) presi = (int)round(f);
            }
        }
        else tin &= ~(THV_PRES | THV_EPRES);
        if (tin & (THV_TEMP | THV_ETEMP | THV_ACU | THV_EACU)) {
            char *c=timebuf;
            if (tin & (THV_TEMP | THV_ETEMP)) {
                strcpy(c,"Temperatura ");c+=strlen(c);
                if (tin & THV_TEMP) {
                    c += sprintf(c,"%s %d°", weaPrefs.inname, trmpi);
                }
                if (tin & THV_ETEMP) {
                    if (tin & THV_TEMP) {
                        strcpy(c,", ");
                        c+=2;
                    }
                    c += sprintf(c,"%s %d°", weaPrefs.outname, trmpo);
                }
                strcpy(c,". "); c+=2;
                if (tin & (THV_PRES | THV_EPRES)) {
                    c+=sprintf(c,"ciśnienie %d hPa%c ",presi,(tin & (THV_HGR | THV_EHGR)) ? ',': '.');
                }
                if (tin & (THV_HGR | THV_EHGR)) {
                    strcpy(c,"Wilgotność ");c+=strlen(c);
                    if (tin & THV_HGR) {
                        if (tin & THV_EHGR) 
                            c += sprintf(c,"%s %d%%",weaPrefs.inname,(int)(round(hgrIn)));
                        else
                            c += sprintf(c,"%d%%",(int)(round(hgrIn)));
                    }
                    if (tin & THV_EHGR) {
                        if (tin & THV_HGR) {
                            c += sprintf(c,", %s %d%%",weaPrefs.outname,(int)(round(hgrOut)));
                        }
                        else {
                            c += sprintf(c,"%d%%",(int)(round(hgrOut)));
                        }

                    }
                }
            }
            if (tin & THV_EACU) {
                uint8_t aena = 0;
                if (enoAcc[0] >= 3) {
                    if (!akuSaid || (enoAcc[0] > 3 && glt > 1)) {
                        aena = 1;
                        akuSaid = 1;
                        akuSaidTime = millis();
                    }
                }
                if (aena) {
                    sprintf(c, (enoAcc[0] == 3) ? "Niski poziom akumulatora %s. " : "Uwaga, akumulator %s rozładowany. ", weaPrefs.outname);
                }
            }
            if (tin & THV_ACU) {
                uint8_t aena = 0;
                if (enoAcc[0] >= 3) {
                    if (!akuSaid || (enoAcc[0] > 3 && glt > 1)) {
                        aena = 1;
                        akuSaid = 1;
                        akuSaidTime = millis();
                    }
                }
                if (aena) {
                    sprintf(c, (enoAcc[0] == 3) ? "Niski poziom akumulatora %s. " : "Uwaga, akumulator %s rozładowany. ", weaPrefs.inname);
                }
            }
            
            ctlSpeakTemp = 1;
            
        }
        if (glt == 2 || (weaPrefs.flags & SPKWEA_HALWAYS)) {
            const char *tdh=NULL, *tmh=NULL, *tdc=NULL, *tmc=NULL,
                *tde=NULL, *tme=NULL;
            if (glt > 1) {
                if (weaPrefs.flags & SPKWEA_HOLLY) {
                    tdh=getHolly(0);
                    if (weaPrefs.flags & SPKWEA_TOMORROW) tmh=getHolly(1);
                }
            }
            tdc = getCalEvent(0);
            if (weaPrefs.flags & SPKWEA_TOMORROW) tmc=getCalEvent(1);
            tde=getEventName(0);
            tme=getEventName(1);
            if (tdh || tdc || tmh || tmc || tde || tme) {
                uint8_t fst=0;
                char *c=timebuf+strlen(timebuf);
                if (tdh || tdc || tde) {
                    strcpy(c,"dzisiaj ");c+=8;
                    if (tdh) {
                        strcpy(c,tdh);c+=strlen(tdh);
                        fst=1;
                    }
                    if (tdc) {
                        if (fst) {strcpy(c,", ");c+=2;}
                        strcpy(c,tdc);
                        c+=strlen(tdc);
                        fst = 1;
                    }
                    if (tde) {
                        if (fst) {strcpy(c,", ");c+=2;}
                        strcpy(c,tde);
                        c+=strlen(tde);
                        fst = 1;
                    }
                    strcpy(c,". "); c+=2;
                }
                if (tmh || tmc || tme) {
                    strcpy(c,"jutro ");c+=6;
                    fst=0;
                    if (tmh) {
                        strcpy(c,tmh);c+=strlen(tmh);
                        fst=1;
                    }
                    if (tmc) {
                        if (fst) {strcpy(c,", ");c+=2;}
                        strcpy(c,tmc);
                        c+=strlen(tmc);
                        fst=1;
                    }
                    if (tme) {
                        if (fst) {strcpy(c,", ");c+=2;}
                        strcpy(c,tme);
                        c+=strlen(tme);
                        fst=1;
                    }
                    
                    strcpy(c,". "); c+=2;
                }
                ctlSpeakTemp = 1;
            }
        }
        if(ctlSpeakTemp && debugMode) Serial.printf("%s\n",timebuf);
    }

    if (ctlSpeakTemp) {
        if (!Gadacz::isSpeaking()) {
            Gadacz::say(timebuf);
            ctlSpeakTemp = 0;
        }
    }
    else if (ctlSpeakWeather) {
        if (ctlSpeakWeather == 1) { // trzeba powiedzieć ale jeszcze nie ma
            if (WiFi.status() == WL_CONNECTED && getWeather()) ctlSpeakWeather =2;
            else ctlSpeakWeather = 3;
        }
        if (ctlSpeakWeather == 2 && !Gadacz::isSpeaking()) {
            ctlSpeakWeather = 0;
            Gadacz::say(weabuf);
        }
        if (ctlSpeakWeather == 3 && !Gadacz::isSpeaking()) {
            ctlSpeakWeather = 0;
            Gadacz::saycst("Brak połączenia z serwerem pogody.");
        }
    }

    if (ctlSpeakAdr) {
        ctlSpeakAdr=0;
        if (WiFi.status() != WL_CONNECTED) {
            Gadacz::saycst("Brak połączenia w siecią");
        }
        else {
            uint32_t adr = WiFi.localIP();
            fmtServerPort(timebuf,adr,SERVER_PORT);
            Gadacz::say(timebuf);
        }
    }
    if (mustSyncClock) {
        syncClockResponse = synchronizeClock();
        mustSyncClock = 0;
    }
    if (serverCurData) {
        prepareTempsForServer(serverCurData);
        serverCurData = NULL;
    }
    rtcLoop();
    serLoop(Serial);
    telnetLoop();
}

void sayCWait(const char *c)
{
    Gadacz::saycst(c);
    Gadacz::waitAudio();
}

int sayCondPercent(int pc)
{
    static int cpc=0;
    if (pc < 0) {
        cpc = pc;
        return -1;
    }
    if (Gadacz::isSpeaking()) return -1;
    if (cpc >= 0 && pc < cpc+5) return pc;
    Gadacz::sayfmt("%d%%.", pc);
    cpc=pc;
    Gadacz::waitAudio();
    return pc;
}
    
