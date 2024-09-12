#include <Arduino.h>
#include "Pogoda.h"

#ifdef USE_CITY_FINDER

#include <rom/miniz.h>

struct cf_miasto {
    uint32_t name;
    unsigned int woj:6;
    unsigned int pwt:9;
    unsigned int wies:1;
    uint16_t lon;
    uint16_t lat;
    uint16_t alt;
};

struct cf_zip {
    unsigned int start:24;
    unsigned int final:24;
    uint16_t city;
};

struct cf_trans {
    uint16_t wznak;
    uint8_t znak;
    uint8_t lznak;
    uint8_t uznak;
    uint8_t lasc;
};
#ifdef FINDER_USE_PARTITION
uint8_t *cf_blob;
#endif
#include "citysdat.h"
//#include "citycomp.h"

uint8_t *cf_mainMemo;

static int cf_OutLen;

static int cf_Deflator(const void *pBuf, int len, void *pUser)
{
    if (cf_OutLen + len > datasize) return 0;
    memcpy(cf_mainMemo + cf_OutLen, pBuf, len);
    cf_OutLen += len;
    return 1;
}


static int mytinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  int result = 0;
  tinfl_decompressor decomp;
  mz_uint8 *pDict = (mz_uint8*)ps_malloc(TINFL_LZ_DICT_SIZE);
  size_t in_buf_ofs = 0, dict_ofs = 0;
  if (!pDict)
    return TINFL_STATUS_FAILED;
  tinfl_init(&decomp);
  for ( ; ; )
  {
    size_t in_buf_size = *pIn_buf_size - in_buf_ofs, dst_buf_size = TINFL_LZ_DICT_SIZE - dict_ofs;
    tinfl_status status = tinfl_decompress(&decomp, (const mz_uint8*)pIn_buf + in_buf_ofs, &in_buf_size, pDict, pDict + dict_ofs, &dst_buf_size,
      (flags & ~(TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
    in_buf_ofs += in_buf_size;
    if ((dst_buf_size) && (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size, pPut_buf_user)))
      break;
    if (status != TINFL_STATUS_HAS_MORE_OUTPUT)
    {
      result = (status == TINFL_STATUS_DONE);
      break;
    }
    dict_ofs = (dict_ofs + dst_buf_size) & (TINFL_LZ_DICT_SIZE - 1);
  }
  free(pDict);
  *pIn_buf_size = in_buf_ofs;
  return result;
}

static int extrc;
static TaskHandle_t decompressTaskHandle = NULL;
static EventGroupHandle_t compev;
static StaticEventGroup_t compEventGroupBuffer;
static void *cpresData;
static void decompressTask(void *unused)
{
    cf_OutLen=0;
    size_t bins;
#ifdef FINDER_USE_PARTITION
    bins = origsize;
    extrc = mytinfl_decompress_mem_to_callback((void *)cf_blob, &bins,
        cf_Deflator, NULL, TINFL_FLAG_PARSE_ZLIB_HEADER);
#else    
    bins = sizeof(cf_compre);
    extrc = mytinfl_decompress_mem_to_callback((void *)cf_compre, &bins,
        cf_Deflator, NULL, TINFL_FLAG_PARSE_ZLIB_HEADER);
#endif
    xEventGroupSetBits(compev, 1);
    vTaskDelete(NULL);
}

static bool decompressExt()
{
    if (!compev) compev=xEventGroupCreateStatic(&compEventGroupBuffer);
    xTaskCreatePinnedToCore(decompressTask, "decompressor", 32768, NULL, 1, &decompressTaskHandle, 1);
    xEventGroupWaitBits(compev, 1, pdTRUE, pdFALSE, portMAX_DELAY);
    return extrc >= 0;
}


#ifdef FINDER_USE_PARTITION
extern "C" {
    extern void *readFlashContent(uint32_t start, uint32_t size);
    extern uint32_t cf_partAdr(int *datasize, int *origsize);
}
#endif

void initCityFinder()
{
#ifdef FINDER_USE_PARTITION
    uint32_t blob=cf_partAdr(&datasize, &origsize);
    if (!blob) return;
#endif
    cf_mainMemo = (uint8_t *)ps_malloc(datasize);
    if (!cf_mainMemo) return;
#ifdef FINDER_USE_PARTITION
    cf_blob= (uint8_t *)readFlashContent(blob, origsize);
    if (!cf_blob) {
        free(cf_mainMemo);
        return;
    }
#endif

    
    haveCityFinder = decompressExt();
#ifdef FINDER_USE_PARTITION
    free(cf_blob);
#endif
    if (!haveCityFinder) {
        free(cf_mainMemo);
        return;
    }

   // haveCityFinder=0;
    /*
     * minlon
minlat
nwoj
npow
nmia
nzip*/

    uint8_t *mmemo = cf_mainMemo;
    minlon = *(uint32_t *)mmemo; mmemo+= 4;
    minlat = *(uint32_t *)mmemo; mmemo+= 4;
    mmemo += 4;
    cf_plen = *(uint32_t *)mmemo; mmemo+= 4;
    cf_mlen = *(uint32_t *)mmemo; mmemo+= 4;
    cf_nzip = *(uint32_t *)mmemo; mmemo+= 4;
    uint32_t n=*(uint32_t *)mmemo; mmemo+= 4;
    //Serial.printf("WOJ %d %d %d\n", n,cf_plen,n/sizeof(*cf_woj));
    cf_woj = (int *) mmemo;
    mmemo += n;

    n=*(uint32_t *)mmemo; mmemo+= 4;
    //Serial.printf("PWT %d %d %d\n", n,cf_plen,n/sizeof(*cf_powiat));
    cf_powiat = (int *) mmemo;
    mmemo += n;

    n=*(uint32_t *)mmemo; mmemo+= 4;
    //Serial.printf("MIA %d %d %d\n", n,cf_mlen,n/sizeof(*cf_miasta));
    cf_miasta = (struct cf_miasto *) mmemo;
    mmemo += n;
    
    n=*(uint32_t *)mmemo; mmemo+= 4;
    //Serial.printf("ZIP %d %d %d\n", n,cf_zips,n/sizeof(*cf_zips));
    cf_zips = (struct cf_zip *) mmemo;
    mmemo += n;

    n=*(uint32_t *)mmemo; mmemo+= 4;
    //Serial.printf("STR %d\n",n);
    cf_strings = mmemo;
    mmemo += n;
    //printf("SIZ %d %d\n",datasize, mmemo -cf_mainMemo);
}

// interface

uint32_t extractCode(const char *cs)
{
    int n = 0;
    uint32_t r=0;
    for (;*cs;cs++) {
        if (!isdigit(*cs)) continue;
        n+=1;
        r= (r * 10) + (*cs - '0');
    }
    if (n != 5) return 0;
    return r;
}

uint16_t findByCode(uint32_t kod)
{
    int low = 0;
    int hig = cf_nzip - 1;
    int mid;
    if (!haveCityFinder) return 0;
    while (low <= hig) {
        mid = low + (hig - low) / 2;
        if (cf_zips[mid].start > kod) hig = mid-1;
        else if (cf_zips[mid].final < kod) low = mid+1;
        else return cf_zips[mid].city;
    }
    return 0;
}


static char *mkutf(char *str, const uint8_t *fname)
{
    uint8_t znak;
    while (znak = *fname++) {
        if (!(znak & 0x80)) {
            *str++ = znak;
            continue;
        }
        int i;
        for (i=0;i<18;i++) if (cf_trans[i].znak == znak) break;
        if (i<18) {
            *str++ = 0xc0 | (cf_trans[i].wznak >> 6);
            *str++ = 0x80 | (cf_trans[i].wznak & 63);
        }
    }
    *str=0;
    return str;
}

void makeCityString(char *buf,int city)
{
    int lon=cf_miasta[city-1].lon + minlon;
    int lat=cf_miasta[city-1].lat + minlat;
    buf=mkutf(buf,cf_strings + cf_miasta[city-1].name);
    if (cf_miasta[city-1].wies) buf+= sprintf(buf," wieś");
    buf += sprintf(buf,", woj. ");
    int woj = cf_miasta[city-1].woj-1;
    buf=mkutf(buf,cf_strings + cf_woj[woj]);
    int pwt = cf_miasta[city-1].pwt;
    if (pwt) {
        buf += sprintf(buf,", p. ");
        buf=mkutf(buf,cf_strings + cf_powiat[pwt-1]);
    }
    buf += sprintf(buf,", %d°%02d'%02d\"N %d°%02d'%02d\"E",
        lat / 3600, (lat % 3600) / 60,lat %60,
        lon / 3600, (lon % 3600) / 60,lon %60
        );
    int alt;
    if (alt=cf_miasta[city-1].alt) {
        buf += sprintf(buf," (");
        buf=mkutf(buf,cf_strings + cf_miasta[alt-1].name);
        buf += sprintf(buf,")");
    }
}   


static int pasuje(const uint8_t *name, const uint8_t *pat)
{
    uint16_t wch; int i;
    while (*pat && *name) {
        if (!(*name & 0x80) && !isalpha(*name)) {
            name++;
            continue;
        }
        if (!(*pat & 0x80) && !isalpha(*pat)) {
            pat++;
            continue;
        }
        if (!(*pat & 0x80)) {
            int fnd=0;
            uint8_t p = tolower(*pat);
            if (p == tolower(*name)) fnd=1;
            else {                
                for (i=0;i<18;i++) {
                    if (cf_trans[i].lasc == p && (
                        cf_trans[i].lznak == *name || cf_trans[i].uznak == *name)) {
                            fnd = 1;
                            break;
                        }
                }
            }
            if (!fnd) return 0;
            pat++;
            name++;
            continue;
        }
        if (!(*name & 0x80)) return 0;
        wch = *pat++;
        if ((wch & 0xe0) != 0xc0) return 0;
        if (!*pat) return 0;
        wch = (wch & 0x1f) << 6;
        wch |= (*pat++) & 63;
        for (i=0;i<18;i++) if (wch == cf_trans[i].wznak) break;
        if (i >= 18) return 0;
        if (*name != cf_trans[i].lznak && *name != cf_trans[i].uznak) return 0;
        name++;
    }
    while (*pat && !(*pat & 0x80) && !isalnum(*pat)) pat++;
    if (*pat) return 0;
    return 1;
}

int findByNamePart(const uint8_t *pat,int bstart)
{
    int i;
    if (!haveCityFinder) return 0;
    for (i=bstart;i<cf_mlen;i++) {
        if (pasuje(cf_strings+cf_miasta[i].name, pat)) return i+1;
    }
    return 0;
}

#endif
