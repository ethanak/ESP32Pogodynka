# Pogodynka - konstrukcja

__UWAGA!__

Omawiam tu konstrukcję opartą na XIAO S3!
Zakładam, że właściwy plik zip (z Releases) został pobrany i rozpakowany.

## Przygotowanie płytki

Należy do padów ```EN``` i ```GND``` (pod spodem płytki XIAO S3) przylutować przewody
którymi będzie podłączony klawisz ```RESET```.

![Podłączenie RESET](/images/resets3.png)

Należy również podłączyć antenę.

## Wstępne zaprogramowanie

Do zaprogramowania potrzebny jest np. program [esptool.py](https://github.com/espressif/esptool)
Jeśli na kopmputerze zainstalowane jest Arduino IDE i Core Board ESP32,
można wykorzystać plik znajdujący się już w folderze tools pakietu
- w przypadku Linuksa będzie to najprawdopodobniej
```~/.arduino15/packages/esp32/tools/esptool_py/```

W ropakowanym folderze znajdują się dwa pliki:

* ```upload_linux.sh``` - skrypt do uploadu. Numer portu jest ustawiony
na ```/dev/ttyACM0``` (```/dev/ttyUSB0``` dla devkit), w razie czego
należy co zmienić na właściwy
* ```addresses.txt``` - lista adresów pod które należy wgrać właściwe pliki .bin,
jeśli używamy innego oprogramowania (np. Flash Upload Tool)

Korzystając z tych plików należy wstępnie wgrać je na płytkę.

__UWAGA!___ W przypadku płytek S3 może być konieczne przestawienie
jej w tryb upload (BOOT, RST, puść RST, puść BOOT)

