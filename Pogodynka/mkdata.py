#!/usr/bin/env python3
import os,re,time

mimetypes={
'html':'text/html; charset=utf-8',
'js':'text/javascript',
'css':'text/css'}
szablon="""static void send_%s(AsyncWebServerRequest *request) {
    lastServerRequest = millis();
    if (request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(mainETag)) {
        //if (debugging) printf("Sending 304\\n");
        request->send(304);
        return;
        }
    //if (debugging) printf("Sending %s\\n");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "%s", %s);
    response->addHeader("ETag",mainETag);
    request->send(response);
    
}
"""

subs=[]

f=open('webdata.h','w')
f.write("/*\n  Plik wygenerowany automatycznie - nie edytuj!\n\
  Edytuj pliki w folderze www i uruchom program mkdata.py\n\
*/\n\n")
f.write("""const char mainETag[]="X_%d";\n""" % time.time())
dy=list(x for x in os.listdir('www') if x.endswith('.js') or x.endswith('.css') or x.endswith('html'))
for flik in dy:
    jq=open('www/' + flik,'rb').read()
    if flik.startswith('placeholder'):
        continue
    if re.match(r'^jquery.*\.js$', flik):
        flik='jquery.js'
    fc = re.match(r'^(.*)\.(.*?)$',flik)
    if not fc:
        continue
    fn='src_' +re.sub(r'[^a-zA-Z0-9]+','_',flik)
    f.write("static const char PROGMEM %s[]={\n" % fn)
    line="";
    for a in jq:
        if isinstance(a,int):
            line += str(a)+','
        else:
            line += str(ord(a))+','
        if (len(line) > 72):
            f.write(line+'\n')
            line=''
    line += '0};\n'
    f.write(line)
    if not flik.endswith('.shtml'):
        mimetype = mimetypes[fc.group(2)]
        f.write(szablon % (fn[4:], fn[4:], mimetype, fn))
        subs.append([flik, fn])

f.write('void initServerConst(void)\n{\n')
for subr in subs:
    f.write('\tserver.on("/%s",send_%s);\n' % (subr[0], subr[1][4:]))
    if subr[0] == 'index.html':
        f.write('\tserver.on("/",send_%s);\n' % (subr[1][4:]))
f.write("}\n");
    
