#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Pogoda.h"

#ifdef ENABLE_OTA
uint8_t doOtaUpdate = 0;
#endif

static AsyncWebServer server(SERVER_PORT);
static uint32_t lastServerRequest;

#include "webdata.h"

static void handleNotFound(AsyncWebServerRequest *request)
{
    request->send(404,"text/plain","File not found");
}

static void getRespSpk(AsyncResponseStream *response, bool first)
{
    if (first) response->printf("{");
    else response->printf(",\n");
    response->printf("\"spktempo\":%d,\"spkpitch\":%d,\"spkvol\":%d,\"spkcon\":%d",
        spkPrefs.tempo, spkPrefs.pitch, spkPrefs.volume, spkPrefs.contrast);
}

static void getRespClk(AsyncResponseStream *response, bool first)
{
    if (first) response->printf("{");
    else response->printf(",\n");
    response->printf("\"clkoffset\":%d,\"clkdst\":%d,\"i_hds\":%d",clkPrefs.offset,clkPrefs.useDST,thrPrefs.ds3231);
}

static void getRespWea(AsyncResponseStream *response, bool first)
{
    if (first) response->printf("{");
    else response->printf(",\n");
    response->printf("\"weaspkpres\":%d,\"weaspkrepres\":%d,\
        \"weaspkhgrin\":%d, \"weaspkhgrout\":%d,\
        \"weaspkhly\":%d, \"weasphchu\":%d, \"weaspktom\":%d,\
        \"weaspkhall\":%d,\"weaspkcrmo\": %d,\
        \"weaspkinname\":\"%s\",\
        \"weaspkexname\":\"%s\",\n\
        \"weaspkpre\":%d,\"weaspkspl\":%d",
            weaPrefs.flags & SPKWEA_PRESS,
            weaPrefs.flags & SPKWEA_REPRESS,
            weaPrefs.flags & SPKWEA_HYGRIN,
            weaPrefs.flags & SPKWEA_HYGROUT,
            weaPrefs.flags & SPKWEA_HOLLY,
            weaPrefs.flags & SPKWEA_CHURCH,
            weaPrefs.flags & SPKWEA_TOMORROW,
            weaPrefs.flags & SPKWEA_HALWAYS,
            (weaPrefs.flags & SPKWEAM_CURRENT) >> SPKWEAS_CURRENT,
            weaPrefs.inname, weaPrefs.outname,
            weaPrefs.minpre, weaPrefs.splithour);
}

void getRespGeo(AsyncResponseStream *response, bool first)
{
    if (first) response->printf("{");
    else response->printf(",\n");
    int l=geoPrefs.lon;char c='E';
    if (l < 0) {c='W';l=-l;}
    response->printf("\"geolon\":\"%02d°%02d'%02d %c\",\n",
        l / 3600, (l % 3600) / 60, l %60, c);
    l=geoPrefs.lat;c='N';
    if (l < 0) {c='S';l=-l;}
    response->printf("\"geolat\":\"%02d°%02d'%02d %c\",\n",
        l / 3600, (l % 3600) / 60, l %60, c);
    if (geoPrefs.elev == ELEV_AUTO) {
        response->printf("\"geoelev\":\"\",\"autoelev\":1");
    }
    else {
        response->printf("\"geoelev\":%d,\"autoelev\":0", geoPrefs.elev);
    }
    response->printf(",\n\"geoautz\":%d",geoPrefs.autotz);
    
}



static void handleGetData(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getRespClk(response,true);
    getRespSpk(response,false);
    getRespWea(response,false);
    getRespGeo(response,false);
#if defined(UNIT_USE_EVENTS)
    if (haveEE) response->printf(",\n\"linkcontainer\":\"kx kxdouble\"");
#endif
#ifdef USE_CITY_FINDER
    if (haveCityFinder) response->printf(",\n\"citysel\":\"kx2a kx\"");
#endif
    response->printf(",\n\"firmversion\":\"%s\"",firmware_version);
#ifdef ENABLE_OTA
    response->printf(",\n\"chkversion\":\"verbutton\"");
#endif
    response->printf("}\n");
    request->send(response);
}

void handleUniRev(AsyncWebServerRequest *request)
{
    String cmd=request->arg("pref");
    const char *cmds=cmd.c_str();
    AsyncResponseStream *response;
    
    if (!strcmp(cmds,"spk")) {
        response = request->beginResponseStream("application/json");
        loadGadacz();
        getRespSpk(response,true);
    }
    else if (!strcmp(cmds,"geo")) {
        response = request->beginResponseStream("application/json");
        loadGeo();
        getRespGeo(response,true);
    }
    else if (!strcmp(cmds,"wea")) {
        response = request->beginResponseStream("application/json");
        loadWea();
        getRespWea(response,true);
    }
    else if (!strcmp(cmds,"wea")) {
        response = request->beginResponseStream("application/json");
        loadClk();
        getRespClk(response,true);
    }
    else {
        request->send(400,"text/plain","Błąd wewnętrzny");
        return;
    }
    response->printf(",\"alert\":\"Przywrócono dane\"}\n");
    //response->printf("}\n");
    request->send(response);
        
}

void handleUniSet(AsyncWebServerRequest *request)
{
    String cmd=request->arg("cmd");
    const char *cmds=cmd.c_str();
    int n;
    Serial.printf("CMD %s\n",cmd.c_str());
    if (!strcmp(cmds,"settz")) {
        n= atoi(request->arg("clkoffset").c_str());
        if (n < -60 || n > 240 || (n % 15)) {
            request->send(200,"text/plain","Nieprawidłowa wartość offsetu");
            return;
        }
        clkPrefs.offset = n;
        clkPrefs.useDST = atoi(request->arg("clkdst").c_str()) ? 1 : 0;
        request->send(200,"text/plain","Ustawiono dane zegara");
        return;
    }
    if (!strcmp(cmds,"setwea")) {
        struct weaPrefs lwp;
        memset((void *)&lwp,0, sizeof(lwp));
        const char *c=request->arg("weaspkinname").c_str();
        const char *err=NULL;
        if (strlen(c) < 4) err="Nazwa wewnątrz jest za krótka";
        else if (strlen(c) > 23) err="Nazwa wewnątrz jest za długa";
        else if (strchr(c,'"')) err="Nazwa wewnątrz zawiera nieprawidłowe znaki";
        if (err) {
            request->send(200,"text/plain",err);
            return;
        }
        strcpy(lwp.inname,c);
        
        c=request->arg("weaspkexname").c_str();
        if (strlen(c) < 4) err="Nazwa na zewnątrz jest za krótka";
        else if (strlen(c) > 23) err="Nazwa zewnątrz jest za długa";
        else if (strchr(c,'"')) err="Nazwa nna zewnątrz zawiera nieprawidłowe znaki";
        if (err) {
            request->send(200,"text/plain",err);
            return;
        }
        strcpy(lwp.outname,c);
        n=atoi(request->arg("weaspkpre").c_str());
        if (n < 10 || n > 60) {
            request->send(200,"text/plain","Wartość prawdopodobieństwa poza zakresem");
            return;
        }
        lwp.minpre=n;
        n=atoi(request->arg("weaspkspl").c_str());
        if (n < 12 || n > 16) {
            request->send(200,"text/plain","Godzina poza zakresem");
            return;
        }
        lwp.splithour=n;
        lwp.flags=0;
        if (atoi(request->arg("weaspkpres").c_str())) lwp.flags |= SPKWEA_PRESS;
        if (atoi(request->arg("weaspkrepres").c_str())) lwp.flags |= SPKWEA_REPRESS;
        if (atoi(request->arg("weaspkhgrin").c_str())) lwp.flags |= SPKWEA_HYGRIN;
        if (atoi(request->arg("weaspkhgrout").c_str())) lwp.flags |= SPKWEA_HYGROUT;
        if (atoi(request->arg("weaspkhly").c_str())) lwp.flags |= SPKWEA_HOLLY;
        if (atoi(request->arg("weaspkchu").c_str())) lwp.flags |= SPKWEA_CHURCH;
        if (atoi(request->arg("weaspktom").c_str())) lwp.flags |= SPKWEA_TOMORROW;
        if (atoi(request->arg("weaspkhall").c_str())) lwp.flags |= SPKWEA_HALWAYS;
        lwp.flags |= (atoi(request->arg("weaspkcrmo").c_str()) << SPKWEAS_CURRENT)
            & SPKWEAM_CURRENT;
        weaPrefs = lwp;
        haveWeather = 0;
        request->send(200,"text/plain","Ustawiono wymowę pogody");
        return;
    }
    if (!strcmp(cmds, "setgeo")) {
        int lon = parseGeo(Serial, (char *)request->arg("geolon").c_str(), "eEwW", 180);
        if (lon == BADGEO) {
            request->send(200,"text/plain","Błędna wartość długości");
            return;
        }
        int lat = parseGeo(Serial, (char *)request->arg("geolat").c_str(), "nNsS", 90);
        if (lat == BADGEO) {
            request->send(200,"text/plain","Błędna wartość szerokości");
            return;
        }
        int elev;
        if (atoi(request->arg("autoelev").c_str()) != 0) {
            elev=ELEV_AUTO;
        }
        else {
            const char *c=request->arg("geoelev").c_str();
            if (!*c) elev=ELEV_AUTO-1;
            else elev = strtol(c,(char **)&c,10);
            if (*c || elev < -400 || elev > 3000) {
                request->send(200,"text/plain","Błędna wartość wysokości");
                return;
            } 
        }
        geoPrefs.lon=lon;
        geoPrefs.lat=lat;
        geoPrefs.elev=elev;
        geoPrefs.autoelev=ELEV_AUTO;
        geoPrefs.autotz=atoi(request->arg("geoautz").c_str());
        haveWeather = 0;
        request->send(200,"text/plain","Ustawiono położenie geograficzne");
        return;
    }
    if (!strcmp(cmds,"setspk")) {
        int te, pi, vo, co;
        te=atoi(request->arg("spktempo").c_str());
        if (te < 0 || te > 24) {
            request->send(200,"text/plain","Błędna wartość tempa");
            return;
        } 
        pi=atoi(request->arg("spkpitch").c_str());
        if (pi < 0 || pi > 24) {
            request->send(200,"text/plain","Błędna wartość wysokości");
            return;
        } 
        vo=atoi(request->arg("spkvol").c_str());
        if (vo < 0 || vo > 24) {
            request->send(200,"text/plain","Błędna wartość głośności");
            return;
        } 
        co=atoi(request->arg("spkcon").c_str());
        if (co < 0 || co > 100) {
            request->send(200,"text/plain","Błędna wartość wyrazistości");
            return;
        } 
        spkPrefs.tempo=te;
        spkPrefs.pitch=pi;
        spkPrefs.volume=vo;
        spkPrefs.contrast=co;
        request->send(200,"text/plain","Ustawiono parametry mowy");
        return;
    }
    request->send(200,"text/plain","Nie rozumiem");
}

void handleUniSave(AsyncWebServerRequest *request)
{
    String cmd=request->arg("pref");
    const char *cmds=cmd.c_str();
    if (!strcmp(cmds,"tz")) {
        storeClk();
        request->send(200,"text/plain","Dane zegara zostały zapisane");
        return;
    }
    if (!strcmp(cmds,"geo")) {
        storeGeo();
        request->send(200,"text/plain","Dane położenia zostały zapisane");
        return;
    }
    if (!strcmp(cmds,"spk")) {
        storeGadacz();
        request->send(200,"text/plain","Parametry mowy zostały zapisane");
        return;
    }
    if (!strcmp(cmds,"wea")) {
        storeWea();
        request->send(200,"text/plain","Parametry prognozy zostały zapisane");
        return;
    }
    request->send(200,"text/plain","Nie rozumiem");
}

void handleSynClock(AsyncWebServerRequest *request)
{
    const char *rc=srvSynchronizeClock();
    request->send(200,"text/plain",rc);
}

static void jsonstr(AsyncResponseStream *stream, const char *txt)
{
    uint8_t znak;
    uint16_t znak16;
    stream->printf("\"");
    while (znak=*txt++) {
        //if (znak == '"' || znak=='\'') {
        //    stream->printf("\\%c",znak);
        //    continue;
        //}
        //if (znak == '\n') {
        //    stream->printf("\\n");
        //    continue;
        //}
        if (znak == '\\') {
            stream->printf("\\\\");
            continue;
        }
        if (znak >= 0x20 && znak <=0x7e && znak != '\'' && znak != '"') {
            stream->printf("%c",znak);
            continue;
        }
        if (!(znak & 0x80)) {
            stream->printf("\\u%04x",znak);
            continue;
        }
        int n;
        if ((znak & 0xe0) == 0xc0) n=1;
        else if ((znak & 0xf0) == 0xf0) n=2;
        else break;
        znak16 = znak & 0x1f;
        while (n--) {
            znak=*txt;
            if ((znak & 0xc0) != 0x80) break;
            znak16 = (znak16 << 6) | (znak & 0x3f);
            txt++;
        }
        stream->printf("\\u%04x",znak16);
    }
    stream->printf("\"");
}


static void ptipadr(
    AsyncResponseStream *response, const char *name, uint32_t ipadr)
{
    response->printf(",\n\"%s\":\"%d.%d.%d.%d\"",
        name,
        ipadr & 255,
        (ipadr >> 8) & 255,
        (ipadr >> 16) & 255,
        (ipadr >> 24) & 255);
}
    
static void handleGetAp(AsyncWebServerRequest *request)
{
    extern char **scannedWiFi;

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\n\"scanned\":[");
    int i;
    if (scannedWiFi) for (i=0;scannedWiFi[i];i++) {
        if (i) response->printf(",\n");
        jsonstr(response, scannedWiFi[i]);
        Serial.printf("%d %s\n",i,scannedWiFi[i]);
    }
    response->printf("],\n\"cred\":{\"ssid\":");
    jsonstr(response,netPrefs.ssid);
    
    response->printf(",\n\"passwd\":");
    jsonstr(response,netPrefs.pass);
    response->printf(",\n\"mdns\":");
    jsonstr(response,netPrefs.name);
    response->printf(",\n\"dhcp\":%d",netPrefs.dhcp);
    ptipadr(response,"ipadr",netPrefs.ip);
    ptipadr(response,"ipmask",netPrefs.mask);
    ptipadr(response,"ipgw",netPrefs.gw);
    ptipadr(response,"ipns1",netPrefs.ns1);
    ptipadr(response,"ipns2",netPrefs.ns2);
    
    response->printf("}}\n");
    request->send(response);
}

static void ptmptd(AsyncResponseStream *response, const char *name,uint8_t flag, float val)
{
    response->printf(",\n\"%s\":", name);
    if (flag) response->printf("%.1f",val);
    else response->printf("null");
}

struct thermoData *serverCurData = NULL;
void handleCurData(AsyncWebServerRequest *request)
{
    struct thermoData tdata;
    static const char *const acnames[]={
            "brak","ładowanie","naładowany","wymaga ładowania","rozładowany"};
            
    if (serverCurData) {
        request->send(500,"text/plain","Trwa pobieranie danych");
        return;
    }
    serverCurData = &tdata;
    int i;
    for (i=0;i<50;i++) {
        delay(50);
        if (!serverCurData) break;
    }
    if (i >= 50) {
        request->send(500,"text/plain","Timeout");
        return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\n\"extpower\":");
    if (tdata.flags & THV_EACU) response->printf("[%d,%d,\"%s\"]",
        tdata.accStatusExt,tdata.accVoltageOut,
        acnames[constrain(tdata.accStatusExt,0,4)]);
    else response->printf("[null,null,\"brak danych\"]");

    response->printf(",\n\"inpower\":");
    if (tdata.flags & THV_ACU) response->printf("[%d,%d,\"%s\"]",
        tdata.accStatusIn,tdata.accVoltageIn,
        acnames[constrain(tdata.accStatusIn,0,4)]);
    else response->printf("[null,null,\"brak danych\"]");

    ptmptd(response, "tempin",tdata.flags & THV_TEMP, tdata.tempIn);
    ptmptd(response, "tempout",tdata.flags & THV_ETEMP, tdata.tempOut);
    ptmptd(response, "pressin",tdata.flags & THV_PRES, tdata.pressIn);
    ptmptd(response, "pressout",tdata.flags & THV_EPRES, tdata.pressOut);
    ptmptd(response, "hummin",tdata.flags & THV_HGR, tdata.hygrIn);
    ptmptd(response, "hummout",tdata.flags & THV_EHGR, tdata.hygrOut);
        response->printf("}\n");
    request->send(response);
}
    
void handleSpkDemo(AsyncWebServerRequest *request)
{
    gadaczSayDemo();
    request->send(200,"text/plain","OK");
}

static void sendFavIcon(AsyncWebServerRequest *request) {
    lastServerRequest = millis();
    if (request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(mainETag)) {
        request->send(304);
        return;
        }
    AsyncWebServerResponse *response = request->beginResponse_P(200,
     "image/x-png",(const uint8_t *)src_favicon_shtml,sizeof(src_favicon_shtml));
    response->addHeader("ETag",mainETag);
    request->send(response);
    
}

volatile uint8_t restartAPServer=0;
#define sef(a) do {request->send(400,"text/plain",a);return;} while(0)
static void handleSetAp(AsyncWebServerRequest *request) {
    struct netPrefs pfs=netPrefs;
    char *nst;
    String arg=request->arg("ssid");
    if (arg.length() > 63) sef("Za długi SSID");
    strcpy(pfs.ssid,arg.c_str());
    arg=request->arg("passwd");
    if (arg.length() > 63) sef("Za długie hasło");
    strcpy(pfs.pass,arg.c_str());
    arg=request->arg("mdns");
    if (arg.length() > 0 && arg != "-") {
        if (!properAvahi(arg.c_str(),pfs.name)) sef("Błędna nazwa lokalna");
    }
    else pfs.name[0] = 0;
    pfs.dhcp=request->arg("dhcp").c_str()[0] == '1';
    if (!pfs.dhcp) {
        IPAddress IA;
        IA.fromString(request->arg("ipadr"));
        if (!(pfs.ip = (uint32_t)IA)) sef("Błędny adres IP urządzenia");
        IA.fromString(request->arg("ipmask"));
        if (!(pfs.mask = (uint32_t)IA)) sef("Błędna maska podsieci");
        IA.fromString(request->arg("ipgw"));
        if (!(pfs.gw = (uint32_t)IA)) sef("Błędny adres bramy");
        IA.fromString(request->arg("ipns1"));
        if (!(pfs.ns1= (uint32_t)IA)) sef("Błędny adres serwera nazw");
        arg=request->arg("ipns2");
        if (arg == "") pfs.ns2 = 0;
        else {
            IA.fromString(arg);
            if (!(pfs.ns2= (uint32_t)IA)) sef("Błędny adres serwera nazw");
        }
    }
    netPrefs = pfs;
    storeNet();
    request->send(200,"text/plain","Zapisane, restartuję urządzenie");
    restartAPServer=1;
}

static void send_index(AsyncWebServerRequest *request) {
    lastServerRequest = millis();
    char etag[32];
    strcpy(etag,mainETag);
    if (imAp) etag[0] = 'Y';
    if (request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(etag)) {
        request->send(304);
        return;
        }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html",
        imAp?src_index_ap_shtml:src_index_shtml);
    response->addHeader("ETag",etag);
    request->send(response);
    
}

static void handleGetCal(AsyncWebServerRequest *request) {
    int i,n;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\"calendar\":[");
    for (i=n=0;i<32;i++) {
        if (calPrefs.days[i].day && calPrefs.days[i].month && calPrefs.days[i].text[0]) {
            response->printf("%s[%d,%d,", n? ",\n": "",
                calPrefs.days[i].day,
                calPrefs.days[i].month);
            jsonstr(response,calPrefs.days[i].text);
            response->printf("]");
            n++;
        }
    }
    response->printf("]\n");
    response->printf(",\"dualhead\":%s}",
#if defined(UNIT_USE_EVENTS)
    haveEE ? "true" : "false"
#else
    "false"
#endif
    );
    request->send(response);
}

static void handleSetCal(AsyncWebServerRequest *request) {
    String arg=request->arg("cal");
    int i,err;
    const char *s;
    int pass;
    for (pass=0;pass<2;pass++) {
        i=err=0;
        s=arg.c_str();
        while (*s && i<32) {
            if (*s=='\n') {
                s++;
                continue;
            }
            int day=strtol(s,(char **)&s,10);
            if (day < 1 || day > 31) {err=1;break;}
            if (*s++ != ' ') {err=2;break;}
            int mon=strtol(s,(char **)&s,10);
            if (mon < 1 || mon > 12) {err=3;break;}
            if (*s++ != ' ') {err=4;break;}
            if (properDate(day,mon)) {
                char buf[64];
                sprintf(buf,"Błędna data %d %s",day,monthNM[mon-1]);
                request->send(400,"text/plain",buf);
                return;
            }
            const char *se=strchr(s,'\n');
            if (!se) se=s+strlen(s);
            if (se - s > 29) {
                request->send(400,"text/plain","Za długa nazwa");
                return;
            }
            if (pass) {
                calPrefs.days[i].day = day;
                calPrefs.days[i].month=mon;
                memcpy((void *) calPrefs.days[i].text, s,se-s);
                calPrefs.days[i].text[se-s]=0;
            }
            s=se;
            i++;
        }
        if (err) {
            request->send(400,"text/plain","Błędne zapytanie");
            return;
        }
    }
    for (;i<32;i++) {
        calPrefs.days[i].day=0;
        calPrefs.days[i].month=0;
        calPrefs.days[i].text[0]=0;
    }
    storeCal();
    request->send(200,"text/plain","Nowy kalendarz został zapisany");
}

#if defined(UNIT_USE_EVENTS)

static uint8_t isProperDate(int d, int m, int y)
{
    if (m < 1 || m >12) return 0;
    y = y % 100;
    int md=monlens[m+1];
    if (m == 2 && !(y & 3)) md += 1;
    if (d<1 || d >md) return 0;
    return 1;
}

static void setEvtError(AsyncWebServerRequest *request, const char *err)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\"error\":");
    jsonstr(response, err);
    response->printf("}\n");
    request->send(response);
}

static void handleDelEvt(AsyncWebServerRequest *request) {
    if (!haveEE) {
        setEvtError(request,"Urządzenie nie ma terminarza");return;
    }
    int idx=atoi(request->arg("index").c_str());
    if (idx < 1 || idx>=EVENT_COUNT) {
        setEvtError(request,"Błędny indeks");return;
    }
    Serial.printf("Removing event %d (%d,%d)\n",idx, eEvents[idx-1].day,eEvents[idx-1].pos);
    LockPrefs();
    eEvents[idx-1].day = 0;
    eEvents[idx-1].pos = 0;
    UnlockPrefs();
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\"idx\":%d}",idx);
    request->send(response);
}

    
    
static void handleSetEvt(AsyncWebServerRequest *request) {
    int i,n;
    if (!haveEE) {
        setEvtError(request,"Urządzenie nie ma terminarza");return;
    }
    getLocalTime();
    int day=atoi(request->arg("day").c_str());
    int mon=atoi(request->arg("month").c_str());
    int year=atoi(request->arg("year").c_str()) % 100;
    String data=request->arg("data");
    if (!isProperDate(day,mon, year)) {
        setEvtError(request,"Błędna data");return;
    }
    if (dateInPast(day, mon,year)) {
        setEvtError(request,"Data w przeszłości");return;
    }
    if (data.length() < 5) {
        setEvtError(request,"Za krótki opis");return;
    }
    n=insertEvent(day,mon,year,data.c_str());
    if (n<0) {
        if (n == -1) setEvtError(request,"Za długi opis");
        else if (n == -2) setEvtError(request,"Brak miejsca na nowy wpis");
        else {
            n=-n-8;
            Serial.printf("%d %d %d\n",n,eEvents[n].day,eEvents[n].pos);
            setEvtError(request,"Wpis na ten dzień już istnieje");
        }
        return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\"result\":[%d,%d,%d,%d,",day, mon, year,n+1);
    jsonstr(response,data.c_str());
    response->printf("]}\n");
    request->send(response);
}

static void evtList(AsyncResponseStream *response)
{
    int i,n;
    getLocalTime();
    response->printf("{\"events\":[");
    for (i=n=0;i<EVENT_COUNT;i++) {
        if (eEvents[i].day && eEvents[i].pos && !eventInPast(&eEvents[i])) {
            response->printf("%s[%d,%d,%d,%d,",n?",\n":"",
                eEvents[i].day,eEvents[i].month,eEvents[i].year,eEvents[i].pos);
            jsonstr(response,eEvents[i].data);
            response->printf("]");
            n++;
        }
    }
    response->printf("]");
}


static void handleGetEvt(AsyncWebServerRequest *request) {
    int i,n;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    if (!haveEE) {
        response->printf("{\"error\":\"Urządzenie nie ma terminarza\"}\n");
        request->send(response);
        return;
    }
    evtList(response);
    response->printf("}\n");
    request->send(response);
}

static void handleSavEvt(AsyncWebServerRequest *request) {
    if (!haveEE) {
        request->send(400,"text/plain","Urządzenie nie ma terminarza");
        return;
    }
    storeEvts();
    request->send(200,"text/plain","Zapamiętano terminarz");
}

static void handleRevEvt(AsyncWebServerRequest *request) {
    int i,n;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    if (!haveEE) {
        response->printf("{\"error\":\"Urządzenie nie ma terminarza\"}\n");
        request->send(response);
        return;
    }
    restoreEvts();
    evtList(response);
    response->printf(",\n\"message\":\"Przywrócono terminarz\"");
    response->printf("}\n");
    request->send(response);
}


#endif

#ifdef USE_CITY_FINDER
void handleCityFinder(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    if (!haveCityFinder) {
        response->printf("{\"error\":\"Brak bazy danych\"}\n");
        request->send(response);
        return;
    }
    String arg=request->arg("city");
    const char *instr = arg.c_str();
    while (*instr && isspace(*instr)) instr++;
    if (strlen(instr)<2) {
        response->printf("{\"error\":\"Za krótki tekst\"}\n");
        request->send(response);
        return;
    }
    uint16_t city,cities[16];
    char outbuf[160];
    int fnd, i;
    if (isdigit(*instr)) {
        uint32_t kod=extractCode(instr);
        if (!kod) {
            response->printf("{\"error\":\"Błędny kod\"}\n");
            request->send(response);
            return;
        }
        city = findByCode(kod);
        if (!city) {
            response->printf("{\"error\":\"Brak kodu w bazie danych\"}\n");
            request->send(response);
            return;
        }
        cities[0] = city;
        fnd = 1;
    }
    else {
        fnd=0;
        while (fnd < 16) {
            city=findByNamePart((const uint8_t*)instr,city);
            if (!city) break;
            cities[fnd++] = city;
        }
        if (fnd == 0) {
            response->printf("{\"error\":\"Żadna nazwa nie pasuje do wzorca\"}\n");
            request->send(response);
            return;
        }

        if (fnd == 16 && findByNamePart((const uint8_t*)instr,cities[fnd-1])) {
            response->printf("{\"error\":\"Do wzorca pasuje ponad 16 nazw, uściślij\"}\n");
            request->send(response);
            return;
        }
    }
    response->printf("{\"data\":[\n");
    for (i = 0; i<fnd; i++) {
        if (i) response->printf(",\n");
        makeCityString(outbuf,cities[i]);
        jsonstr(response,outbuf);
    }
    response->printf("]}\n");
    request->send(response);
}
        
#endif

#ifdef ENABLE_OTA

void handleUpdateOta(AsyncWebServerRequest *request)
{
    doOtaUpdate = 1;
    request->send(200,"text/plain","OK");
}

void handleCheckOta(AsyncWebServerRequest *request)
{
    int n = checkOta();
    if (n == -1) {
        request->send(200,"application/json","{\"message\":\"Brak aktualizacji\"}");
        return;
    }
    if (n == -2) {
        request->send(200,"application/json","{\"message\":\"Brak nowej wersji\"}");
        return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->printf("{\"message\":\"Dostępna wersja %d.%d.%d\",\"doit\":true}\n",
        (n >> 16) & 255,(n >> 8) & 255,n & 255);
    request->send(response);
    
}


#endif

void initServer()
{
    initServerConst();
    server.on("/",send_index);
    server.on("/index.html",send_index);
    server.on("/favicon.ico",sendFavIcon);
    if (!imAp) {
        server.on("/getdata.cgi",handleGetData);
        server.on("/uniset.cgi",handleUniSet);
        server.on("/unisave.cgi",handleUniSave);
        server.on("/unirev.cgi",handleUniRev);
        server.on("/synclock.cgi",handleSynClock);
        server.on("/spkdemo.cgi",handleSpkDemo);
        server.on("/current.cgi",handleCurData);
        server.on("/getcal.cgi",handleGetCal); 
        server.on("/setcal.cgi",handleSetCal); 
#ifdef USE_CITY_FINDER
        server.on("/city.cgi",handleCityFinder);
#endif
#ifdef  ENABLE_OTA
        server.on("/checkota.cgi",handleCheckOta);
        server.on("/otaupdate.cgi",handleUpdateOta);
#endif
#if defined(UNIT_USE_EVENTS)
        server.on("/savevt.cgi",handleSavEvt);
        server.on("/revevt.cgi",handleRevEvt);
        server.on("/getevt.cgi",handleGetEvt);
        server.on("/addevt.cgi",handleSetEvt);
        server.on("/delevt.cgi",handleDelEvt);
#endif
    }
    else {
        server.on("/getap.cgi",handleGetAp);
        server.on("/setap.cgi",handleSetAp);
    }
    server.onNotFound(handleNotFound);
}

void startServer()
{
    server.begin();
}

void stopServer()
{
    server.end();
}
