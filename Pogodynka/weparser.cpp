#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "Pogoda.h"



struct curweather {
    uint8_t control;
    uint8_t humidity;
    uint8_t windCode;
    uint8_t windir;
    uint8_t weaCode;
    uint8_t cloud;
    uint16_t realtemp;
    uint16_t appatemp;
    uint16_t presmsl;
    uint32_t curtime;
} curweather;


#define CH_REALTEMP 1
#define CH_APPATEMP 2
#define CH_WINDCODE 4
#define CH_WINDIR 8
#define CH_WEACODE 0x10
#define CH_CLOUD 0x20
#define CH_MSL 0x40
#define CH_HUM 0x80

struct dlyweather {
    uint16_t control;
    uint8_t windmin;
    uint8_t windmax;
    uint8_t windguts;
    uint8_t weaCode;
    uint8_t cloudmin;
    uint8_t cloudmax;
    uint32_t time;
} dlywea[3];


struct hrlyweather {
    uint16_t control;
    int16_t realtemp;
    int16_t appatemp;
    int16_t rain;
    int16_t showers;
    int16_t snowfall;
    int16_t prepro;
    int32_t time;
} hrlwea[72];
    
    
#define DH_WIMIN 1
#define DH_WIMAX 2
#define DH_WIGUS 4
#define DH_WEACO 8
#define DH_CLMIN 0x10
#define DH_CLMAX 0x20



#define OC(a) offsetof(struct curweather, a)
#define OD(a) offsetof(struct dlyweather, a)
#define OH(a) offsetof(struct hrlyweather, a)

#define TYP_I10 1
#define TYP_WINDC 2
#define TYP_WINDIR 3
#define TYP_UINT 4
#define TYP_DATETIME 6


struct wepars {
    const char *name;
    uint8_t cbit;
    uint8_t ofset;
    uint8_t typ;
    uint8_t width;
} curwepars[] = {
    {"time",0,OC(curtime),TYP_DATETIME,2},
    {"temperature_2m",CH_REALTEMP, OC(realtemp), TYP_I10,1},
    {"apparent_temperature",CH_APPATEMP, OC(appatemp), TYP_I10,1},
    {"weather_code",CH_WEACODE,OC(weaCode),TYP_UINT,0},
    {"wind_speed_10m",CH_WINDCODE,OC(windCode), TYP_WINDC,0},
    {"wind_direction_10m",CH_WINDIR,OC(windir),TYP_WINDIR,0},
    {"pressure_msl",CH_MSL,OC(presmsl),TYP_UINT,1},
    {"cloud_cover",CH_CLOUD,OC(cloud),TYP_UINT,0},
    {"relative_humidity_2m",CH_HUM,OC(humidity),TYP_UINT,0},
    {NULL,0,0,0}};

struct wepars dlypars[]={
    {"wind_speed_10m_min",DH_WIMIN,OD(windmin),TYP_WINDC,0},
    {"wind_speed_10m_max",DH_WIMAX,OD(windmax),TYP_WINDC,0},
    {"wind_gusts_10m_max",DH_WIGUS,OD(windmax),TYP_WINDC,0},
    {"weather_code",DH_WEACO,OD(weaCode),TYP_UINT,0},
    {"cloud_cover_min",DH_CLMIN,OD(cloudmin),TYP_UINT,0},
    {"cloud_cover_max",DH_CLMIN,OD(cloudmax),TYP_UINT,0},
    {"time",0,OD(time),TYP_DATETIME,2},
    {NULL,0,0,0}};


#define HA_REALTEMP 1
#define HA_APPATEMP 2
#define HA_RAIN 4
#define HA_SHOWERS 8
#define HA_SNOW 16
#define HA_PREPRO 32

struct wepars hrlpars[]={
    {"temperature_2m",HA_REALTEMP,OH(realtemp),TYP_I10,1},
    {"apparent_temperature",HA_APPATEMP,OH(appatemp),TYP_I10,1},
    {"rain",HA_RAIN,OH(rain),TYP_UINT,1},
    {"showers",HA_SHOWERS,OH(showers),TYP_UINT,1},
    {"snowfall",HA_SNOW,OH(snowfall),TYP_UINT,1},
    {"precipitation_probability",HA_PREPRO,OH(prepro),TYP_UINT,1},
    {"time",0,OH(time),TYP_DATETIME,2},
    {NULL,0,0,0}};
        

char namebuffer[32];
char valbuffer[32];

int8_t get_sval(const char **str)
{
    char *c;
    if (!**str) return -1;
    if (**str == ',') (*str)++;
    if (**str == ']' || **str=='}') return 0;
    if (* (*str)++ != '"') return -1;
    c=namebuffer;
    while (c-namebuffer < 31 && **str != '"') {
        if ((*c++ = *(*str)++) == 0) return -1;
    }
    if (**str != '"') return -1;
    *c=0;
    (*str)++;
    if (*(*str)++ != ':') return -1;
    if (**str == '[') return 2;
    c=valbuffer;
    if (**str == '"') {
        *c++=*(*str)++;
        while (c-valbuffer < 30 && **str != '"') {
            if ((*c++ = *(*str)++) == 0) return -1;
        }
        if ((*c++ = *(*str)++) != '"') return -1;
        *c=0;
        return 1;
    }
    while (c-valbuffer < 31 && !strchr(",]}",**str)) {
        if ((*c++ = *(*str)++) == 0) return -1;
    }
    if (!strchr(",]}",**str)) return -1;
    *c=0;
    return 1;
    
    
}


static const char *getSection(const char *str,const char *secname)
{
    char buf[128];
    sprintf(buf,"\"%s\":{",secname);
    const char *s=strstr(str,buf);
    
    return s+strlen(buf);
}

const struct wepars *findWeparser(const char *name, const struct wepars *arr)
{
    while (arr->name) {
        if (!strcmp(arr->name,name)) return arr;
        arr++;
    }
    return NULL;
}

static const int dfyr[]={0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static uint16_t jday(int d, int m, int y)
{
    
    int day = 365 * y + (y-1)/4 + d - 1 + dfyr[m-1];
    if (! (y & 3) && m > 2) day++;
    return day;
}

static uint16_t parseDate(const char *value, const char **evalue)
{
    uint16_t y,d,m;
    if (*value == '"') value++;
    y=strtol(value,(char **)&value,10);
    if (*value++ != '-') return 0;
    m=strtol(value,(char **)&value,10);
    if (*value++ != '-') return 0;
    d=strtol(value,(char **)&value,10);
    if (*value == '"') value++;
    *evalue=value;
    return jday(d,m,y-2000);
}

static uint16_t parseTime(const char *value)
{
    if (*value == '"') value++;
    int t = strtol(value,(char **)&value,10);
    if (*value++ != ':') return 0;
    value=strchr(value,'"')+1;
    return t * 60 + strtol(value, NULL,10);
}

int parseValue(const char *value,int vtype)
{
    static const uint8_t wmax[]={1,18,29,40,50,72};
    int i,v;
    uint32_t dti;
    switch(vtype) {
        case TYP_I10:
        return (int)(strtod(value,NULL) * 10);

        case TYP_UINT:
        return (int)(strtod(value,NULL));

        case TYP_WINDIR:
        return ((atoi(value) + 360 - 22) / 45) & 7;

        case TYP_WINDC:
        v=atoi(value);
        for (i=0;i<6;i++) if (v <= wmax[i]) break;
        return i;

        case TYP_DATETIME:
        dti=parseDate(value,&value);
        if (*value++ == 'T') {
            dti |= parseTime(value) << 16;
        }
        return dti;
    }
    return atoi(value);        
}

uint8_t parseDaily(const char *section)
{
    int z;
    int num;
    for (num=0;num<3;num++) dlywea[num].control = 0;
    for (;;) {
        z = get_sval(&section);
        if (z < 0) return 1;
        if (z ==0) return 0;
        
        if (z == 1) continue;
        const struct wepars *wp=findWeparser(namebuffer,dlypars);
        if (!wp) {
            section = strchr(section,']');
            if (!section) return 1;
            section++;
            continue;
        }
        num=0;
        section++;
        while (*section != ']') {
                int v=parseValue(section,wp->typ);

                if (wp->width==2) {
                    * (uint32_t *)((uint8_t *)&(dlywea[num]) + wp->ofset) = v;
                }
                else if (wp->width==1) {
                    * (int16_t *)((int8_t *)&(dlywea[num]) + wp->ofset) = v;
                }
                else {
                    *((uint8_t *)&(dlywea[num]) + wp->ofset) = v;
                }
                dlywea[num].control |= wp->cbit;
                
                num += 1;
                const char *s=strpbrk(section,"],");
                if (!s) return 1;
                else if (*s==',') s++;
                section=s;
                if (num >= 3) {
                    if (*section != ']') section = strchr(section,']');
                    if (!section) return 1;
                }
            
        }
        section++;
    }
    return 0;
}

uint8_t parseHourly(const char *section)
{
    int z;
    int num;
    for (num=0;num<72;num++) hrlwea[num].control = 0;
    for (;;) {
        z = get_sval(&section);
        if (z < 0) return 1;
        if (z ==0) return 0;
        
        if (z == 1) continue;
        const struct wepars *wp=findWeparser(namebuffer,hrlpars);
        if (!wp) {
            section = strchr(section,']');
            if (!section) return 1;
            section++;
            continue;
        }
        num=0;
        section++;
        while (*section != ']') {
                int v=parseValue(section,wp->typ);

                if (wp->width==2) {
                    * (uint32_t *)((uint8_t *)&(hrlwea[num]) + wp->ofset) = v;
                }
                else if (wp->width==1) {
                    * (int16_t *)((int8_t *)&(hrlwea[num]) + wp->ofset) = v;
                }
                else {
                    *((uint8_t *)&(hrlwea[num]) + wp->ofset) = v;
                }
                hrlwea[num].control |= wp->cbit;
                
                num += 1;
                const char *s=strpbrk(section,"],");
                if (!s) return 1;
                else if (*s==',') s++;
                section=s;
                
                if (num >= 72) {
                    if (*section != ']') section = strchr(section,']');
                    if (!section) return 1;
                }
            
        }
        section++;
    }
    return 0;
}

uint8_t parseWeather(const char *str)
{
    if (geoPrefs.autoelev == ELEV_AUTO && geoPrefs.elev == ELEV_AUTO) {
        char *s=strstr(str,"\"elevation\":");
        if (s) {
            s=strchr(s,':')+1;
            int ael=strtol(s,NULL,10);
            geoPrefs.autoelev=ael;
            storeGeo();
        }
    }
            
    const char *section = getSection(str,"current");
    if (!section) {
        return 1;
    }
    curweather.control = 0;
    if (section) {
        while (get_sval(&section) == 1) {
            const struct wepars *wp=findWeparser(namebuffer,curwepars);
            if (!wp) continue;
            int v=parseValue(valbuffer,wp->typ);
            if (wp->width==2) {
                * (uint32_t *)((uint8_t *)&curweather + wp->ofset) = v;
            }
            else if (wp->width==1) {
                * (int16_t *)((int8_t *)&curweather + wp->ofset) = v;
            }
            else {
                *((uint8_t *)&curweather + wp->ofset) = v;
            }
            curweather.control |= wp->cbit;
        }
    }
    section = getSection(str,"daily");
    if (!section) return 1;
    if (parseDaily(section)) return 1;
    section = getSection(str,"hourly");
    if (!section) return 1;    
    return parseHourly(section);
}

char weabuf[2048];

struct weatherTrip {
    int idx;
    const char *descr[2];
};

static const struct weatherTrip pogoda_chmury[]={
    {12,{"bezchmurnie","bezchmurnie"}},
    {37,{"zachmurzenie małe","małego"}},
    {62,{"zachmurzenie umiarkowane","umiarkowanego"}},
    {95,{"zachmurzenie duże","dużego"}},
    {0,{"zachmurzenie całkowite","całkowitego"}}};

const char * chmury(int ile)
{
    int i;
    for (i=0;i<4;i++) if (ile <=pogoda_chmury[i].idx) break;
    return pogoda_chmury[i].descr[0];
}

const char *chmury2(int ile1, int ile2)
{
    int i,j;
    static char chbuf[64];
    for (i=0;i<4;i++) if (ile1 <=pogoda_chmury[i].idx) break;
    for (j=0;j<4;j++) if (ile2 <=pogoda_chmury[j].idx) break;
    if (i == j) sprintf(chbuf,"%s. ", pogoda_chmury[i].descr[0]);
    else if (i == 0) sprintf(chbuf,"%s do zachmurzenia %s. ",
        pogoda_chmury[i].descr[0],
        pogoda_chmury[j].descr[1]);
    
    else sprintf(chbuf,"%s do %s. ",
        pogoda_chmury[i].descr[0],
        pogoda_chmury[j].descr[1]);
    return chbuf;
}


struct weatherPair {
    int idx;
    const char *descr;
};

static const struct weatherPair pogoda_Opis[]={
{0, "Czyste niebo"},
{1, "Przeważnie bezchmurnie"},
{2, "Zachmurzenie częściowe"},
{3, "Pochmurno"},
{45, "Mgła"},
{48, "Marznąca mgła"},
{51, "Lekka mżawka"},
{53, "Umiarkowana mżawka"},
{55, "Intensywmna mżawka"},
{56, "Lekka marznąca mżawka"},
{57, "Intensywna marznąca mżawka"},
{61, "Niewielki deszcz"},
{63, "Umiarkowany deszcz"},
{65, "Intensywny deszcz"},
{66, "Lekki marznący deszcz"},
{67, "Silny marznący deszcz"},
{71, "Słabe opady śniegu"},
{73, "Uniarkowane opady śniegu"},
{75, "Silne opady śniegu"},
{77, "Śnieg ziarnisty"},
{80, "Przelotne lekkie opady deszczu"},
{81, "Przelotne umiarkowane opady deszczu"},
{82, "Przelotne gwałtowne opady deszczu"},
{85, "Przelotne słabe opady sniegu"},
{86, "Przelotne intensywne opady sniegu"},
{95, "Burza z piorunami"},
{96, "Burza z lekkim gradem"},
{99, "Burza z silnym gradem"},
{100, NULL}};

static const char *weaname(int idx)
{
    int i;
    for (i=0;pogoda_Opis[i].descr;i++) {
        if (pogoda_Opis[i].idx == idx) return pogoda_Opis[i].descr;
        
    }
    return "";
}

static int prognoPos,firstHour,lastHour;
static int findWeas(int n)
{
    int i;
    for (i=0; i<3;i++) {
        if (!dlywea[i].control) continue;
        if ((dlywea[i].time & 0xffff) == (curweather.curtime & 0xffff)+n) break;
    }
    if (i >= 3) return 1;
    prognoPos = i;
    firstHour = -1;
    for (i=0;i<72;i++) {
        if (!hrlwea[i].control) continue;
        if ((hrlwea[i].time & 0xffff) == (curweather.curtime & 0xffff) + n) {
            if (firstHour < 0) firstHour=i;
            lastHour=i;
        }
    }
    if (firstHour < 0) return 1;
    return 0;
}

const char * const wiatry[][2]={
        {"bezwietrznie","bezwietrznie"},
        {"wiatr słaby","słabego"},
        {"wiatr umiarkowany","umiarkowanego"},
        {"wiatr dość silny","dość silnego"},
        {"wiatr silny","silnego"},
        {"wiatr bardzo silny","bardzo silnego"},
        {"wichura","wichury"}};

static const char *windirs[]={
    " północny", " północno-wschodni"," wschodni"," południowo-wschodni",
    " południowy"," południowo-zachodni"," zachodni"," północno-zachodni"};


static const char *const weadays[]={"dziś","jutro","pojutrze"};

static const char *houra(int n)
{
    static char wbuf[16];
    if (n == 0) return "północy";
    sprintf(wbuf,"godzinie %d", n);
    return wbuf;
}

uint8_t rainyDay[26];
uint8_t rainyHour[26];
uint8_t rainyCount;
static const char *const hoursodo[]={
    "północy","pierwszej","drugiej","trzeciej",
    "czwartej","piątej","szóstej","siódmej",
    "ósmej","dziewiętej","dziesiątej","jedenastej",
    "dwunastej","trzynastej","czternastej","piętnastej",
    "szesnastej","siedemnastej","osiemnastej","dziewiętnastej",
    "dwudziestej","dwudziestej pierwszej","dwudziestej drugiej",
    "dwudziestej trzeciej","północy"};
    
static char *rainMaker(char *out)
{
    int i,ni;
    for (i=firstHour,ni=1;i<=lastHour && ni < 26;i++,ni++) {
        rainyHour[ni]=(hrlwea[i].time >> 16) / 60+1;
        rainyDay[ni]=hrlwea[i].prepro  > weaPrefs.minpre;
        }
    rainyCount=ni;
    rainyHour[0]=0;
    if (firstHour > 0) {
        rainyDay[0]=hrlwea[firstHour-1].prepro > weaPrefs.minpre;
    }
    else {
        rainyDay[0] = rainyDay[1];
    }
    for (i=1;i<rainyCount-1;i++) if (rainyDay[i-1] && rainyDay[i+1]) rainyDay[i]=1;
    int opa = 0;
    int pos = 0;
    int rbeg,rend;
    int was = 0;
    for (opa=0;;opa++) {
        for (;pos < rainyCount;pos++) if (rainyDay[pos]) break;
        if (pos >= rainyCount) break;
        rbeg = rainyHour[pos];
        if (rbeg >= 24) break;
        pos++;
        for (;pos < rainyCount;pos++) if (!rainyDay[pos]) break;
        if (pos >=rainyCount) rend=-1;
        else rend = rainyHour[pos];
        if (!opa) 
            strcpy(out,"Przewidywane opady:");
        else
            strcpy(out,", ");
        out += strlen(out);
        
        if (rbeg) {
            out += sprintf(out," od %s",hoursodo[rbeg]);
            was = 1;
        }
        if (rend >= 0 && rend < 24) {
            out += sprintf(out," do %s",hoursodo[rend]);
            was = 1;
        }
    }
    if (opa) {
        if (!was) {
            strcpy(out," całą dobę");
            out += strlen(out);
        }
        strcpy(out,". ");
        out += 2;
    }
        
    return out;
}

static char *makeDay(int n,char *out)
{
    if (findWeas(n)) return out;
    out += sprintf(out,"Prognoza na %s. ",weadays[n]);
    const char *s;
    s=weaname(dlywea[prognoPos].weaCode);
    if (s && *s) {
        out += sprintf(out,"%s. ",s);
    }
    int w1=dlywea[prognoPos].windmin;
    int w2=dlywea[prognoPos].windmax;
    out += sprintf(out,"%s",wiatry[w1][0]);
    if (w2 > w1) {
        if (w1 == 0 && w2 != 6) {
            out += sprintf(out," do wiatru %s",wiatry[w2][1]);
        }
        else {
            out += sprintf(out," do %s",wiatry[w2][1]);
        }
    }
    
    int w3 = dlywea[prognoPos].windguts;
    if (w3 > w2) {
        const char *s;
        s=wiatry[w2][0];
        if (w1 == 0 && w3 != 6) {
            s+=6;
        }
        out += sprintf(out,", w porywach %s",s);
    }
    out += sprintf(out,". ");
    w1 = dlywea[prognoPos].cloudmin;
    w2 = dlywea[prognoPos].cloudmax;
    strcpy(out,chmury2(w1,w2));
    out += strlen(out);
    int tmax = -1000;
    int hmax;
    int i,nh;
    for (i=firstHour;i<=lastHour;i++) if (hrlwea[i].realtemp > tmax) {
        tmax = hrlwea[i].realtemp;
        hmax = (hrlwea[i].time >> 16) / 60;
        nh=i;
    }
    out += sprintf(out,"Temperatura maksymalna, %d° o %s. ",
        tmax/10,houra(hmax));
        int tmin1=1000;
    int tmin2=1000;
    int hmin1;
    int hmin2;
    for (i=firstHour;i<nh;i++) if (hrlwea[i].realtemp/10 <= tmin1) {
        tmin1 = hrlwea[i].realtemp/10;
        hmin1 = (hrlwea[i].time >> 16) / 60;
    }
    for (i=nh+1;i<=lastHour;i++) if (hrlwea[i].realtemp/10 < tmin2) {
        tmin2 = hrlwea[i].realtemp/10;
        hmin2 = (hrlwea[i].time >> 16) / 60;
    }
    if (tmin1 < 1000 || tmin2 < 1000) {
        strcpy(out,"Temperatura minimalna, ");
        out+=strlen(out);
        if (tmin1 < 1000) out += sprintf(out,"%d° o %s",tmin1,houra(hmin1));
        if (tmin2 < 1000) {
            if (tmin1 < 1000) {
                strcpy(out,", ");
                out += 2;
            }
            out += sprintf(out,"%d° o %s",tmin2,houra(hmin2));
        }
        strcpy(out,". ");
        out += 2;
    }
    out = rainMaker(out);
    return out;
}
    
    

const char * makeWeather()
{
    char *out=weabuf;
    int n = (weaPrefs.flags & SPKWEAM_CURRENT) >> SPKWEAS_CURRENT;
    if (n == 1) {
        out += sprintf(weabuf,"Teraz: %s, temperatura %d°, odczuwalna %d°.",
        weaname(curweather.weaCode),
        curweather.realtemp/10, curweather.appatemp/10);
    }
    else if (n > 1) {
        out += sprintf(weabuf,"Teraz: %s, temperatura %d°, odczuwalna %d°. Wilgotność %d%%. ",
            weaname(curweather.weaCode),
            curweather.realtemp/10, curweather.appatemp/10,curweather.humidity);
        out+=sprintf(out,"Ciśnienie %d hPa. ",curweather.presmsl);
        out+=sprintf(out,"%s. ",chmury(curweather.cloud));
        out+=sprintf(out,"%s%s. ",wiatry[curweather.windCode][0],
        (curweather.windCode >0 && curweather.windCode<6)?windirs[curweather.windir]:"");
    }
    if (_hour < weaPrefs.splithour) out = makeDay(0,out);
    out = makeDay(1,out);
    if (_hour >= weaPrefs.splithour) out = makeDay(2,out);
    return weabuf;
}
