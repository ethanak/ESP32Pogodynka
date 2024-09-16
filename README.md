# ESP32Pogodynka

ESP32Pogodynka is interested only for Polish language speakers, so further descriptions will be in Polish

## UWAGA

Na razie instrukcja jest niekompletna, proszę o cierpliwość!

## Co potrzebne do kompilacji


* Arduino IDE z zainstalowanym core board ESP32 w wersji min. 3.0.4
* Dodatkowe biblioteki
  * [Mimbrola](https://github.com/ethanak/Mimbrola) - silnik syntezy mowy, wersja co najmniej z 15.09.2024
  * [microlena](https://github.com/ethanak/microlena) - polski system TTS
  * [ESP32Gadacz](https://github.com/ethanak/ESP32Gadacz) - wrapper do microleny
  * [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) - biblioteka do dźwięku
  * [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) - niezbędna do działana serwera WWW
  * [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - serwer WWW we własnej osobie
* Pozostałe biblioteki do zainstalowania z managera bibliotek:
  * Bounce2
  * Time
  * I2C_EEPROM
  * ESP_Telnet
  * BMP280
  * BMP180MI
  * Dallas Temperature (wraz z OneWire)
  * DHT Sensor Library
* Dla instalacji na XIAO ESP32S3 i podobnych (min. 8 MB Flash, min. 2MB PSRAM)
  * [polski głos pl1_ulaw](https://github.com/ethanak/mimbrola_voices_pl) - żeby mogła mówić (patrz informacje w [instrukcji](mimbrola)

__UWAGA!
Arduino-IDE w wersji 2.x oraz arduino-cli nie zawsze chcą użyć układu partycji
niezbędnego do działania wersji S3, dopóki problem nie zostanie rozwiązany
należy użyć wersji 1.8.x__

Ponieważ standardowo w Arduino IDE nie zawiera układu partycji koniecznego
do skompilowania programu, należy przygotować je zgodnie z [instrukcją](partitions).

Ponieważ biblioteka Mimbrola może być skonfigurowana w różny sposób, uproszczona [instrukcja](mimbrola) konfiguracji.

## Ustawienia menu Arduino IDE

Dla płytek Dev Module wystarczy ustawić właściwy układ partycji, pozostawiając pozostałę parametry jako domyślne (w rqazie wątpliwości ustawić jak dla XIAO):
```
Partition Scheme: 4M Flash (Application Only)
```

Dla XIAO S3 ustawienia powinny być następujące:
```
Arduino Runs On: Core 1
Core Debug Level: None
CPU Frequency: 240MHz(WiFi)
Erase All Flash Before Sketch Upload: Disabled
Events Run On: Core 1
Flash Mode: QIO 80 MHz
Flash Size: 8 MB
JTag Adapter: Disabled
Partition Scheme: 8M, App=2880k, OTA, Mbrola pl1_alaw
PSRAM: OPI PSRAM
Upload Mode: UART0 / Hardware CDC
Upload Speed: 921600
USB CDC On Boot: Enabled
USB DFU On Boot: Disabled
USB Firmware MSC On Boot: Disabled
USB Mode: Hardware CDC and JTAG
```

Jeśli kompilacja i upload się powiodą, należy połączyć się z płytką
dowolnym terminalem Serial z prędkością 115200 i wpisać polecenie ```help```.

To nie wszystko... pracuję dalej nad dokumentacją



