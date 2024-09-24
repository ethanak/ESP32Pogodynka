#include <Arduino.h>
//#include <BMP280.h>
#include <BMP180TwoWire.h>
#include <Bme280.h>

#include "Pogoda.h"
#include <Wire.h>
#include <DHT.h>
#include <DallasTemperature.h>

//BMP280 *bmp280;
BMP180TwoWire *bmp180;
Bme280TwoWire *bme280;

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
    /*
    if (thrPrefs.trmin == DEVT_BMP280) {
        bmp280 = new BMP280(thrPrefs.i2cin);
        bmpready=!bmp280->begin();
    }
    else
    */
    if (thrPrefs.trmin == DEVT_BMP180) {
        bmp180 = new BMP180TwoWire(&Wire, 0x77);
        bmpready=bmp180->begin();
    }
    else if (thrPrefs.trmin == DEVT_DS) {
        indexOneWire();
    }
    else if (thrPrefs.trmin == DEVT_BME280 || thrPrefs.trmin == DEVT_BMP280) {
        bme280 = new Bme280TwoWire();
        bmpready = bme280 -> begin((Bme280TwoWireAddress) thrPrefs.i2cin);
        bme280->setSettings(Bme280Settings::indoor());

        //Serial.printf("NE %d\n", bmpready);
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

float tempIn, pressIn,pressOut, tempOut, hgrOut, hgrIn;

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
            rc = THV_TEMP;
            break;
        }
    }
    if (bmp180->measurePressure()) {
        for (i=0;i<10;i++) {
            if (i) delay(50);
            if (bmp180->hasValue()) {
                pressIn = bmp180->getPressure();
                rc = THV_PRES;
                break;
            }
        }
    }
    return bmpHasData = rc;
}

/*
uint8_t getBMP280()
{
    if (!bmpready) return 0;
    if (millis() - bmpLastRead < 2100) {
        if (!bmpHasData) return 0;
        return bmpHasData;
    }
    uint32_t x=millis();
    pressIn =bmp280->getPressure() / 100.0;
    tempIn= bmp280->getTemperature();
    bmpHasData = THV_TEMP | THV_PRES;
    bmpLastRead=millis();
    return bmpHasData;
}
*/

float daltemp[2];

uint8_t getDallas()
{
    if (!OWire) indexOneWire();
    if (!owcount) return 0;
    dallas->requestTemperatures();
    int i;
    for (i=0;i<owcount;i++) {
        daltemp[i]=dallas->getTempC(owids[i]);
    }
    return i;
}


uint8_t getBME280()
{
    if (!bmpready) return 0;
    if (millis() - bmpLastRead < 2100) {
        if (!bmpHasData) return 0;
        return bmpHasData;
    }
    pressIn =bme280->getPressure() / 100.0;
    tempIn= bme280->getTemperature();
    hgrIn=bme280->getHumidity();
    bmpHasData = THV_TEMP | THV_PRES;
    if (hgrIn > 0.1) bmpHasData |= THV_HGR;
    bmpLastRead=millis();
    return bmpHasData;
    
    return 0;
}


//#define ENO_MAXWAIT (25UL * 60000UL)
 #define ENO_MAXWAIT (thrPrefs.espwait * 60000UL) 

uint8_t getTemperatures()
{
    int rc = 0;
    int dall = 0;
    uint32_t t;
    switch(thrPrefs.trmin) {
        case DEVT_BMP180:
        rc = getBMP180();
        break;

        case DEVT_BMP280:
        //rc = getBMP280();
        //break;

        case DEVT_BME280:
        rc = getBME280();
        break;

        case DEVT_DS:
        dall = getDallas();
        if (dall) {
            tempIn = daltemp[0];
            rc = THV_TEMP;
        }
        break;
        case DEVT_ESPNOW:
        t=millis();
        Procure;
        if (enoStat[1] && t - lastEno[1] <= ENO_MAXWAIT) {
            if (enoStat[1] & THV_TEMP) tempIn = enoTemp[1];
            if (enoStat[1] & THV_HGR) hgrIn = enoHumm[1];
            if (enoStat[1] & THV_PRES) pressIn = enoPres[1];
            rc |= enoStat[1];
        }
        Vacate;
        break;
    }
    
    switch(thrPrefs.trmex)
    {
        case DEVT_DHT11:
        case DEVT_DHT22:
        rc |= getDHT() << 4;
        break;

        case DEVT_DS:
        if (thrPrefs.trmex == DEVT_DS) {
            if (dall == 2) {
                tempOut = daltemp[1];
                rc |= THV_TEMP << 4;
            }
        }
        else {
            dall = getDallas();
            if (dall) {
                tempOut = daltemp[0];
                rc |= THV_TEMP << 4;
            }
        }
        break;

        case DEVT_ESPNOW:
        t=millis();
        Procure;
        if (enoStat[0] && t - lastEno[1] <= ENO_MAXWAIT) {
            if (enoStat[0] & THV_TEMP) tempOut = enoTemp[0];
            if (enoStat[0] & THV_HGR) hgrOut = enoHumm[0];
            if (enoStat[0] & THV_PRES) pressOut = enoPres[0];
            rc |= enoStat[0] << 4;
        }
        Vacate;
        break;
    }
    return rc;
}

void prepareTempsForServer(struct thermoData *td)
{
    td->flags=getTemperatures();
    td->accStatusExt = enoAcc[0];
    td->accStatusIn = enoAcc[1];
    td->pressIn = pressIn;
    td->pressOut = pressOut;
    td->tempIn = tempIn;
    td->tempOut = tempOut;
    td->hygrIn = hgrIn;
    td->hygrOut =hgrOut;
    td->accVoltageOut = enoVolt[0];
    td->accVoltageIn = enoVolt[1];
};

