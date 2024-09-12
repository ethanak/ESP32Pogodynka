#include <Arduino.h>
#include "Termometr.h"
#include <DHT.h>
#include <DallasTemperature.h>

DHT *dhtsensor;
OneWire *OWire;
DallasTemperature *dallas;
uint8_t owcount=0,owid[8];

uint8_t initDallas()
{
    if (!realPrefes.pin) return 0;
    if (!OWire) OWire = new OneWire(realPrefes.pin);
    if (!dallas)  {
        dallas = new DallasTemperature(OWire);
        dallas->setResolution(12);
        owcount=0;
    }
    if (owcount == 0) {
        delay(200);
        OWire->reset_search();
        if (OWire->search(owid)) owcount=1;
    }
    return owcount;
}

uint8_t readDallas()
{
    if (!initDallas()) return 0;
    dallas->requestTemperatures();
    tempOut=dallas->getTempC(owid);
    if (tempOut < -40.0 || tempOut > 60.0) return 0;
    return 1;
    
}

float tempOut, hgrOut;

uint8_t initDHT()
{
    if (dhtsensor) return 1;
    if (!realPrefes.pin) return 0;
    if (prefes.termtyp == THR_DHT22) dhtsensor=new DHT(realPrefes.pin,DHT22);
    else if (prefes.termtyp == THR_DHT11) dhtsensor=new DHT(realPrefes.pin,DHT11);
    else return 0;
    dhtsensor->begin();
    delay(2500);
    return 1;
}

uint8_t readDHT()
{
    if (!initDHT()) return 0;
    tempOut=dhtsensor->readTemperature();
    hgrOut=dhtsensor->readHumidity();
    if (isnan(tempOut)) {
        delay(2500);
        tempOut=dhtsensor->readTemperature();
        hgrOut=dhtsensor->readHumidity();
    }
        
    if (isnan(tempOut)) {
        return 0;
    }
    if (isnan(hgrOut)) return 1;
    return 3;
}

uint8_t readTemp()
{
    int rc=0;
    switch(realPrefes.termtyp) {
        case THR_DHT11:
        case THR_DHT22:
        
        rc = readDHT();
        break;

        case THR_DS:
        rc=readDallas();
        break;

    }
    return rc;
}

