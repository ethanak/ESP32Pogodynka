#include <Arduino.h>
#include <BMP280.h>
#include <BMP180TwoWire.h>

#include "Pogoda.h"
#include <Wire.h>
#include <DHT.h>
#include <DallasTemperature.h>

BMP280 *bmp280;
BMP180TwoWire *bmp180;
DHT *dhtsensor;
OneWire *OWire;
DallasTemperature *dallas;

uint8_t bmpready=0;
uint8_t dhtready=0;
uint8_t bmpHasData;
uint32_t bmpLastRead;
uint8_t owcount;
uint8_t owids[2][8];

uint8_t indexOneWire()
{
    owcount=0;
    if (!OWire) OWire = new OneWire(PIN_ONEWIRE);
    if (!dallas)  {
        dallas = new DallasTemperature(OWire);
        dallas->setResolution(12);
    }
    delay(200);
    OWire->reset_search();
    while (owcount < 2) {
        if (!OWire->search(owids[owcount])) break;
        owcount++;
    }
    return owcount;
    
}
uint8_t initThermo()
{
    int rc = 0;
    bmpLastRead = millis();
    bmpHasData = 0;
    bmpready = 0;
    if (thrPrefs.trmin == DEVT_BMP280) {
        bmp280 = new BMP280(thrPrefs.i2cin);
        bmpready=!bmp280->begin();
    }
    else if (thrPrefs.trmin == DEVT_BMP180) {
        bmp180 = new BMP180TwoWire(&Wire, 0x77);
        bmpready=bmp180->begin();
    }
    else if (thrPrefs.trmin = DEVT_DS) {
        indexOneWire();
    }

    if (thrPrefs.trmex == DEVT_DHT11 || thrPrefs.trmex == DEVT_DHT22) {
        dhtsensor = new DHT(PIN_DHT, (thrPrefs.trmex == DEVT_DHT11)?DHT11:DHT22);
        dhtsensor->begin();
    }
    else if (thrPrefs.trmex == DEVT_DS && thrPrefs.trmin != DEVT_DS) {
        indexOneWire();
    }
        
    dhtsensor=new DHT(23,DHT11);
    dhtsensor->begin();
    return 1;
}

float tempIn, pressIn,tempOut, hgrOut;

    /*
     * INIT ds
     * iniT BMP280
     * READ Dht
     * WAITBOTH
     */



uint8_t getDHT()
{
    float f=dhtsensor->readTemperature();
    float g=dhtsensor->readHumidity();
    if (isnan(f)) {
        if (debugMode) Serial.printf("DHT nie chce działać\n");
        return 0;
    }
    tempOut=f;
    if (isnan(g)) return 1;
    hgrOut=g;
    return 3;
}
uint8_t getBMP180()
{
    if (!bmpready) return 0;
    if (millis() - bmpLastRead < 2100) {
        if (!bmpHasData) return 0;
        return bmpHasData;
    }

    if (!bmp180->measureTemperature()) return bmpHasData = 0;
    int i,rc=0;
    for (i=0;i<10;i++) {
        if (i) delay(50);
        if (bmp180->hasValue()) {
            tempIn = bmp180->getTemperature();
            rc = 1;
            break;
        }
    }
    if (bmp180->measurePressure()) {
        for (i=0;i<10;i++) {
            if (i) delay(50);
            if (bmp180->hasValue()) {
                pressIn = bmp180->getPressure();
                rc = 2;
                break;
            }
        }
    }
    return bmpHasData = rc;
}

uint8_t getBMP280()
{
    //Serial.printf("BMPR %d\n", bmpready);
    if (!bmpready) return 0;
    if (millis() - bmpLastRead < 2100) {
        if (!bmpHasData) return 0;
        return 3;
    }
    uint32_t x=millis();
    pressIn =bmp280->getPressure() / 100.0;
    tempIn= bmp280->getTemperature();
    bmpHasData = 1;
    bmpLastRead=millis();
    //Serial.printf("BMP got data %d\n", bmpLastRead - x);
    return 3;
}

float daltemp[2];

uint8_t getDallas()
{
    if (!OWire) indexOneWire();
    if (!owcount) return 0;
    dallas->requestTemperatures();
    int i;
    for (i=0;i<owcount;i++) {
        daltemp[i]=dallas->getTempC(owids[i]);
        //Serial.printf("DA%d %.2f\n",i+1,daltemp[i]);
    }
    return i;
}


uint8_t getBME280()
{
    return 0;
}


//#define ENO_MAXWAIT (25UL * 60000UL)
 #define ENO_MAXWAIT (thrPrefs.espwait * 60000UL) 

uint8_t getTemperatures()
{
    int rc = 0;
    int dall = 0;
    switch(thrPrefs.trmin) {
        case DEVT_BMP180:
        rc = getBMP180();
        break;

        case DEVT_BMP280:
        rc = getBMP280();
        break;

        case DEVT_DS:
        dall = getDallas();
        if (dall) {
            tempIn = daltemp[0];
            rc = 1;
        }
        break;
    }
    uint32_t t;
    
    switch(thrPrefs.trmex)
    {
        case DEVT_DHT11:
        case DEVT_DHT22:
        rc |= getDHT() <<2;
        break;

        case DEVT_DS:
        if (thrPrefs.trmin == DEVT_DS) {
            if (dall == 2) {
                tempOut = daltemp[0];
                rc |= 1;
            }
        }
        else {
            dall = getDallas();
            if (dall) {
                tempOut = daltemp[0];
                rc |= 4;
            }
        }
        break;

        case DEVT_ESPNOW:
        t=millis();
        //Serial.printf("ES %d, TI %d\n", enoStat, millis() - lastEno);
        Procure;
        if (enoStat && t - lastEno <= ENO_MAXWAIT) {
            if (enoStat & 1) tempOut = enoTemp;
            if (enoStat & 2) hgrOut = enoHumm;
            rc |= enoStat << 2;
        }
        Vacate;
        break;
    }
        
 #if 0   
    int rc=getBMP280();
    rc |= getDHT() << 2;
#endif
    return rc;
}

void prepareTempsForServer(struct thermoData *td)
{
    td->flags=getTemperatures();
    td->accStatus = enoAcc;
    td->press = pressIn;
    td->tempIn = tempIn;
    td->tempOut = tempOut;
    td->hygrIn = 0;
    td->hygrOut =hgrOut;
    td->accVoltage = enoVolt;
};

