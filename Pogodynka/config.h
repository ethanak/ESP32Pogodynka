#ifndef _POGOCONFIG_H
#define _POGOCONFIG_H

// odkomentuj linię jeśli masz więcej niż 4 MB FLash
//#define BOARD_HAS_BIG_FLASH 1

// odkomentuj linię jeśli masz tylko 4 MB FLash
//#define BOARD_HAS_BIG_FLASH 0

// jeśli nie odkomentujesz, program ustawi 0 dla esp32 i 1 dla esp32s3
// co wcale nie musi być prawdą



#ifndef BOARD_HAS_BIG_FLASH
#ifdef  CONFIG_IDF_TARGET_ESP32S3
#define BOARD_HAS_BIG_FLASH 1
#else
#define BOARD_HAS_BIG_FLASH 0
#endif
#endif


// dla nietypowych płytek S3:
// #define DISABLE_OTA

// tego nie ruszaj
//#define FINDER_USE_PARTITION


#endif
