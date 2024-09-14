# ESP32Pogodynka

ESP32Pogodynka is interested only for Polish language speakers, so further descriptions will be in Polish

## UWAGA

Na razie instrukcja jest niekompletna, proszę o cierpliwość!

## Co potrzebne do kompilacji


* Arduino IDE z zainstalowanym core board ESP32 w wersji min. 3.0.4
* Dodatkowe biblioteki
  * [Mimbrola](https://github.com/ethanak/Mimbrola) - silnik syntezy mowy
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
  * [polski głos pl1_alaw](https://github.com/ethanak/mimbrola_voices_pl) - żeby mogła mówić (patrz informacje w [instrukcji](mimbrola)

Ponieważ standardowo w Arduino IDE nie zawiera układu partycji koniecznego
do skompilowania programu, należy przygotować je zgodnie z [instrukcją](partitions).

Ponieważ biblioteka Mimbrola może być skonfigurowana w różny sposób, uproszczona [instrukcja](mimbrola) konfiguracji.

To nie wszystko... pracuję dalej nad dokumentacją







