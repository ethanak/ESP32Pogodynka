# Przygotowanie środowiska Arduino IDE

_(dotyczy wyłącznie programu Pogodynka)_

Niezależnie od wersji IDE (1.x lub 2.x) pakiet nie zawiera układu
partycji potrzebnego do zainstalowania Pogodynki. Konieczne jest
dodanie plików csv  opisujących partycje oraz dodanie do boards.txt
linii umożliwiających wybór ukłau partycji z menu.

__UWAGA! Wszystkie zmiany należy wprowadzać przy wyłączonym Arduino IDE!__

## Dla płytki Dev Module i podobnych

__Płytka z 4 MB Flash, bez PSRAM__

Przede wszystkim należy zlokalizować folder, w którym znajduje się katalog
_hardware_ pakietu ESP32. W przypadku Linuksa będzie to najprawdopodobniej:

```
~/.arduino15/packages/esp32/hardware/esp32/<wersja>/
```

gdzie <wersja> to numer wersji core board (powinien być tlko jeden).

Teraz do folderu tools/partitions należy skopiować zamieszczony plik
```apponly.csv``` - powinien wyglądać następująco:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x3F0000,
``` 

Taki układ partycji rezerwuje maksymalny obszar pamięci flash na aplikację.

Następnie należy wprowadzić poprawki do ```boards.txt```.

W pliku należy zlokalizować ostatnią linię rozpoczynającą się od:

```
esp32.menu.PartitionScheme.
```

Za tą linią należy dodać:

```
esp32.menu.PartitionScheme.apponly=4M Flash (Application only)
esp32.menu.PartitionScheme.apponly.build.partitions=apponly
esp32.menu.PartitionScheme.apponly.upload.maximum_size=4128768

```

(oczywiście jeśli używamy innej płytki należy użyć
własciwego prefiksu zamiast ```esp32```).

Po uruchomieniu Arduino IDE powinna być możliwość wyboru ```4M Flash (Application only)``` z menu Partition Scheme.

## Dla płytki XIAO ESP32S3 i podobnych

__Płytka z min. 8 MB Flash i min. 2 MB PSRAM__

Podobnie jak wyżej, należy zlokalizować folder docelowy.

Teraz do folderu tools/partitions należy skopiować zamieszczony plik
```mbr8ota.csv``` - powinien wyglądać następująco:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x2d0000,
app1,     app,  ota_1,   0x2e0000,0x2d0000,
mbrola,   data, 0x40,    0x5b0000,0x250000,
```
Taki układ partycji pozwala na aktualizację OTA.

Podobnie jak w przypadku Dev Module, należy wprowadzić poprawki do ```boards.txt```:

Należy znaleźć ostatnią linię rozpoczynającą się od:
```
XIAO_ESP32S3.menu.PartitionScheme.
```

i dopisać za nią:

```
XIAO_ESP32S3.menu.PartitionScheme.mbr8ota=8M App=2880k OTA Mbrola pl1_ulaw
XIAO_ESP32S3.menu.PartitionScheme.mbr8ota.build.partitions=mbr8ota
XIAO_ESP32S3.menu.PartitionScheme.mbr8ota.upload.maximum_size=2949120
```

(oczywiście jeśli używamy innej płytki należy użyć
własciwego prefiksu zamiast ```XIAO_ESP32S3```).

Po uruchomieniu Arduino IDE powinna być możliwość
wyboru ```8M, App=2880k, OTA, Mbrola pl1_alaw``` z menu Partition Scheme.


