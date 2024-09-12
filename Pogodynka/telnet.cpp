#include <Arduino.h>
#include "Pogoda.h"
#include <ESPTelnetStream.h>

ESPTelnetStream telnet;
const int telnetPort = 23;
static uint8_t telnetStarted = 0;

extern void doSerialCmd(Print &Ser,char *str);
void onTelnetConnect(String ip) {
    (void) ip;
    telnet.println("\nUstawienia pogodynki - wpisz \"quit\" aby wyjść");
}
 
void startTelnet()
{
    if (telnetStarted) return;
    telnet.onConnect(onTelnetConnect);
    telnetStarted = telnet.begin(telnetPort);
}

void resetESP()
{
    if (telnetStarted && telnet.isConnected()) {
        telnet.disconnectClient();
        delay(100);
    }
    ESP.restart();
}

static struct cmdBuffer telnetBuffer;

void addCmdChar(Print &ser,struct cmdBuffer *buf, uint8_t znak, uint8_t fromTelnet)
{
    if (znak == '\r' || znak == '\n') {
        //Serial.printf("ZNAK %d\n",znak);
        if (!buf->bufpos) return;
        buf->buffer[buf->bufpos]=0;
        if (fromTelnet && !strcmp(buf->buffer,"quit")) {
            telnet.disconnectClient();
        }
        else {
            doSerialCmd(ser,buf->buffer);
        }
        buf->bufpos = 0;
        buf->blank=0;
        return;
    }
    if (isspace(znak)) {
        if (buf->bufpos) buf->blank=1;
        return;
    }
    if (buf->bufpos < 80) {
        if (buf->blank) {
            buf->buffer[buf->bufpos++]=' ';
            buf->blank = 0;
        }
        buf->buffer[buf->bufpos++]=znak;
    }
}

void telnetLoop()
{
    if (!telnetStarted) return;
    telnet.loop();
    while (telnet.available() >0) {
        addCmdChar(telnet,&telnetBuffer,telnet.read(),1);
    }
}
