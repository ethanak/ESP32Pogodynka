#include <Arduino.h>
#include "Termometr.h"
#include <DHT.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Bme280.h>

DHT *dhtsensor;
OneWire *OWire;
DallasTemperature *dallas;
uint8_t owcount=0,owid[8];

uint8_t bmpAdr=0;
Bme280TwoWire *bme280;


void i2cscan() {
    int nDevices = 0;
    Serial.println("Skanuję I2C...");
  
    for (byte address = 0x76; address < 0x78; ++address) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("I2C: znaleziono 0x%02X\n", address);
            bmpAdr=address;
            break;
        }
        else if (error == 4) {
            Serial.printf("I2C: błąd 0x%02X\n", address);
        }
        

    }
    Serial.printf("Skanowanie zakończone, adres %02x\n", bmpAdr);
}


int I2C_ClearBus() {

  pinMode(prefes.sdapin, INPUT_PULLUP); // Make prefes.sdapin (data) and prefes.sclpin (clock) pins Inputs with pullup.
  pinMode(prefes.sclpin, INPUT_PULLUP);

  delay(100);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(prefes.sclpin) == LOW); // Check is prefes.sclpin is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master. 
    return 1; //I2C bus error. Could not clear prefes.sclpin clock line held low
  }

  boolean SDA_LOW = (digitalRead(prefes.sdapin) == LOW);  // vi. Check prefes.sdapin input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If prefes.sdapin is Low,
    clockCount--;
  // Note: I2C bus is open collector so do NOT drive prefes.sclpin or prefes.sdapin high.
    pinMode(prefes.sclpin, INPUT); // release prefes.sclpin pullup so that when made output it will be LOW
    pinMode(prefes.sclpin, OUTPUT); // then clock prefes.sclpin Low
    delayMicroseconds(10); //  for >5us
    pinMode(prefes.sclpin, INPUT); // release prefes.sclpin LOW
    pinMode(prefes.sclpin, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5us
    // The >5us is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(prefes.sclpin) == LOW); // Check if prefes.sclpin is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for prefes.sclpin to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(prefes.sclpin) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. prefes.sclpin clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(prefes.sdapin) == LOW); //   and check prefes.sdapin input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. prefes.sdapin data line held low
  }

  // else pull prefes.sdapin line low for Start or Repeated Start
  pinMode(prefes.sdapin, INPUT); // remove pullup.
  pinMode(prefes.sdapin, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5us
  pinMode(prefes.sdapin, INPUT); // remove output low
  pinMode(prefes.sdapin, INPUT_PULLUP); // and make prefes.sdapin high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5us
  pinMode(prefes.sdapin, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(prefes.sclpin, INPUT);
  return 0; // all ok
}



void initI2C()
{
    if (!prefes.sclpin || !prefes.sdapin) return;
    I2C_ClearBus();
    Wire.begin(prefes.sdapin, prefes.sclpin);
    i2cscan();
}

uint8_t bmpready=0;

uint8_t initBME()
{
    static uint8_t initialized = 0;
    if (initialized) return bmpAdr;
    initialized = 1;
    initI2C();
    Serial.printf("WIRE %02x\n", bmpAdr);
    if (!bmpAdr) return 0;
    bme280 = new Bme280TwoWire();
    bmpready = bme280 -> begin((Bme280TwoWireAddress) bmpAdr);
    if (!bmpready) {
        bmpAdr=0;
        return 0;
    }
    bme280->setSettings(Bme280Settings::indoor());
    return bmpAdr;
}


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

float tempOut, hgrOut, presOut;

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

uint8_t readBMP()
{
    if (!initBME()) return 0;
    bme280->wakeUp();
    delay(2100);
    presOut = bme280->getPressure() / 100.0;
    tempOut = bme280->getTemperature();
    hgrOut=bme280->getHumidity();
    bme280->sleep();
    Serial.printf("TPH %f %f %f\n", tempOut, presOut, hgrOut);
    if (isnan(tempOut) || isnan(presOut) || presOut < 1.0) return 0;
    uint8_t rc = 5;
    if (hgrOut > 0.1) rc |= 2;
    return rc;
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

        case THR_BMP280:
        rc = readBMP();
        break;

    }
    return rc;
}

