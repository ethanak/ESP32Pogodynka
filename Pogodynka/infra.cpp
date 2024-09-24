#include <Arduino.h>
#include "Pogoda.h"
#include <esp32-rmt-ir.h>


uint32_t lastCode, lastCodeMillis;
uint8_t haveIR,monitorIR,lastCodeKey;

static uint8_t code2key(uint32_t code)
{
    int i,j;
    for (i=0;i<9;i++) {
        if (irrPrefs.keycodes[i][j] == code) return i+1;
    }
    return 0;
}

void irReceived(irproto brand, uint32_t code, size_t len, rmt_symbol_word_t *item){
	if( code ){
        Procure;
        lastCode = code;
        lastCodeKey = code2key(code);
        haveIR=lastCodeKey != 0;
        lastCodeMillis = millis();
        Vacate;
        if (monitorIR) 
            Serial.printf("IR %s, code: %#x, bits: %d\n",  proto[brand].name, code, len);
	}
}

void initIR()
{
    if (!irrPrefs.enabled || !PIN_IR_RX) return;
 	irRxPin = PIN_IR_RX;
	irTxPin = 0;
	xTaskCreatePinnedToCore(recvIR, "recvIR", 8192, NULL, 10, NULL, 1);
}

