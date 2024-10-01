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

## Pierwsze uruchomienie

### konfiguracja sieci

Urządzenie (bez podłączania peryferiów) powinno zgłosić się na odpowiednim
porcie serial (115200, 8N1). Jako pierwsze należy skonfigurować połączenie
WiFi podając kolejno polecenia:

```
ssid nazwa sieci
pass hasło
dhcp a
net save
```
dla korzystania z DHCP, lub

```
ssid nazwa sieci
pass hasło
dhcp r
ip adres_ip_urządzenia
gw adres_bramy
mask maska_podsieci
name1 adres_ip_serwera_nazw
name2 adres_ip_serwera_nazw
net save
```

Przykładowo:

```
ssid Maszt 5G Siemiatycze 500% mocy
pass NieZgadnieszHasla
dhcp a
net save
```

dla konfiguracji automatycznej, lub

```
ssid Maszt 5G Murzasichle 500% mocy
pass NieZgadnieszHasla
dhcp r
ip 192.168.1.41
gw 191.168.1.1
mask 24
name1 8.8.8.8
name2 8.8.4.4
net save
```

dla konfiguracji ręcznej.

Po wydaniu polecenia ```net save``` urządzenie powinno się zrestartować.
Na syjściu serial będą pokazywać się komunikaty diagnostyczne, najważniejsze
są komunikaty dotyczące WiFi. Jeśli komunikatem będzie ```WiFi: uzyskano IP```
oznaczać to będzie, że płytka prawidłowo połączyła się z routerem
i jest gorowa do dalszej konfiguracji.

### konfiguracja pinów

