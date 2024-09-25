# ESP32Pogodynka

ESP32Pogodynka is interested only for Polish language speakers, so further descriptions will be in Polish

## UWAGA

Na razie instrukcja jest niekompletna, proszę o cierpliwość!

## Co to jest?

__Pogodynka__ w założeniu jest urządzeniem, które może zastąpić
spotykane w handlu tzw. "mówiące termometry". Niestety - urządzenia te
są niezbyt dokładne, poza tym możliwości są niewielkie. Projekt
nie przedstawia kompletnego urządzenia - wykonanie obudowy i elektroniki
pozostawiam konstruktorowi konkretnego urządzenia. Montaż elektroniki
wymaga jedynie podstaw umiejętności lutowania, a moduły po wlutowaniu
goldpinów można umieścić nawet na małej płytce stykowej.

__Pogodynka__ wymaga stałego zasilania 5V - prąd pobierany przez urządzenie
jest na tyle mały, że wystarczy niewielki zasilacz, np. ładowarka od starego
telefonu lub - w przypadku montażu zasilacza w obudowie -
[HiLink HLK PM-01](https://botland.com.pl/zasilacze-montazowe/10929-zasilacz-hi-link-hlk-pm01-100v-240vac-5vdc-06a-5904422317102.html).
Wymaga również połączenia Wi-Fi.

### Możliwości urządzenia

* Zegar z kalendarzem i możliwością własnych wpisów
* terminarz (nie we wszystkich wersjach)
* Pomiar temperatury wewnątrz i na zewnątrz
* W zależności od zastosowanych czujników:
  * Pomiar ciśnienia
  * Pomiar wilgotności
* Podanie danych pobranych z Internetu (serwis [Open-Meteo](https://open-meteo.com/)):
  * aktualne warunki pogodowe
  * uproszczona prognoza na dwa dni

Urządzenie (poza wstępnym zaprogramowaniem przez interfejs serial) może
być częściowo programowane przez końcowego użytkownika za pomocą
przeglądarki internetowej (nie jest potrzebna żadna dodatkowa
aplikacja na smartfonie lub komputerze).

W najprostszej wersji urządzenie składa się jedynie z płytki ESP32,
wzmacniacza MAX98357, głośnika i przycisku. W takiej formie może być ono
zmontowane nawet na najmniejszej płytce stykowej (170 pól). Ta wersja
podaje jedynie czas (wraz z kalendarzem) i warunki zewnętrzne pobrane z Internetu.
Do tego można podłączyć:

* Moduł zegara DS3231 z pamięcią EEPROM (brak konieczności ciągłej synchronizacji
  z serwerami czasu, terminarz
* Czujnik temperatury wewnątrz:
  * Dallas DS18B20 - tylko temperatura
  * BMP 280 - temperatura i ciśnienie
  * BME 280 - temperatura, ciśnienie i wilgotność
* Czujnik temperatury na zewnątrz (podłączany przewodem):
  * Dallas DS18B20 - tylko temperatura
  * DHT22 - temperatura i wilgotność
* Odbiornik IR - umożliwia zdalną obsługę za pomocą pilota

Przykładowy układ zmontowany na płytce stykowej:

![XIAO-S3 na stykówce](/images/pogo170a.jpg)

Zmieściły się tam:

* Dodatkowy przycisk RESET
* Odbiornik podczerwieni TSOP31246
* Czujnik BMP280


_ciąg dalszy nastąpi_

## Kompilacja

__UWAGA!__

W większości przypadków kompilacja nie jest konieczna, wystarczą pliki
binarne umieszczone w Releases!



### Co potrzebne do kompilacji


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
  * Bme280 [ta wersja](https://github.com/malokhvii-eduard/arduino-bme280)
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

### Ustawienia menu Arduino IDE

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

_To nie wszystko... pracuję dalej nad dokumentacją_
