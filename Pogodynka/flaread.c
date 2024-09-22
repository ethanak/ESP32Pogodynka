#include "config.h"
#ifdef FINDER_USE_PARTITION
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <esp_partition.h>

uint32_t cf_partAdr(int *datasize, int *origsize)
{
    esp_partition_iterator_t pat;
    static uint32_t blob_start = 0;
    pat = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (pat) {
        const esp_partition_t *t= esp_partition_get(pat);
  //      printf("%x %x %08x %08x %s\n",t->type, t->subtype, t->address, t->size, t->label);
        if (!strcmp(t->label, "cities")) {
            blob_start = t->address;
            break;
        }
        pat = esp_partition_next(pat);
    }
    esp_partition_iterator_release(pat);
    if (!blob_start) {
        printf("Brak partycji cities\n");
        return 0;
    }
    uint32_t idata[3];
    int et = spi_flash_read(blob_start,idata,12);
    if (et) {
        printf("Flash read error\n");
        return 0;
    }
    if (idata[0] != 0xDEAF0ABD) {
        printf("Błędna partycja cities\n");
        return 0;
    }
    *datasize = idata[1];
    *origsize = idata[2];
    return blob_start+12;
}    

void *readFlashContent(uint32_t start, uint32_t size)
{
    void *whereto=ps_malloc(size);
    if (!whereto) return NULL;
    if (spi_flash_read(start, whereto, size)) {
        printf("Nie dało się wczytać tabel cities\n");
        free(whereto);
        return NULL;
    }
    return whereto;
}
#endif
