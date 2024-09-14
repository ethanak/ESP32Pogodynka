# Konfiguracja biblioteki Mimbrola

_(dotyczy wyłącznie programu Pogodynka)_

## Dla płytki Dev Module

Ponieważ biblioteka ESP32Gadacz zawiera wszystkie potrtzebne pliki,
wystarczy postępować zgodnie z [instrukcją](https://github.com/ethanak/ESP32Gadacz/tree/master/voice).

## Dla płytki XIAO ESP32S3

W folderze, w którym zainstalowano bibliotekę znajduje się folder data,
przeznaczony do instalacji głosu. Do tego folderu należy skopiować folder
pl_alaw, zawierający nagłówki polskiego głosu skompresowanego algorytmem A-law
i przystosowanego do odczytu z przygotowanej partycji.

Następnie należy zamienić plik config.h z głównego folderu Mimbroli na załączony.

Należy następnie pobrać plik [espola_alaw.blob](https://github.com/ethanak/mimbrola_voices_pl/raw/main/Mimbrola/espola_alaw.blob),
zawierający właściwy polski głos.
Plik ten należy wgrać do płytki S3 pod adres 0x5b0000 (zgodnie z układem
partycji mbr8ota) np. programem [esptool](https://github.com/espressif/esptool)
lub Flash Download Utility. Przykładowo dla Linuksa polecenie będzie:

```
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
    --before default_reset --after hard_reset \
    write_flash  -z --flash_mode keep --flash_freq keep --flash_size keep\
    0x5b0000 "espola.blob" 
```

lub prościej

```
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
    write_flash 0x5b0000 "espola.blob" 
```

Dla MacOS i Windows polecenie będzie wyglądać podobnie.

Można również użyć załączonego w core board ESP32 programu esptool -
znajduje się on (dla Linuksa) w ```~/.arduino15/packages/esp32/tools/esptool_py/<numer_wersji>/esptool.py```.
W takim przypadku polecenie (dla Linuksa) polecenie wyglądałoby mniej więcej tak:

```
python3 ~/.arduino15/packages/esp32/tools/esptool_py/*/esptool.py \
    --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
    --before default_reset --after hard_reset \
    write_flash  -z --flash_mode keep --flash_freq keep --flash_size keep\
    0x5b0000 "espola.blob" 
```


