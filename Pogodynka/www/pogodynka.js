
function simpleAJAX() {
  this.http = new XMLHttpRequest();
}


const http = new simpleAJAX();


simpleAJAX.prototype.get = function (url, callback) {
  var _this = this;

  this.http.open('GET', url, true);

  this.http.onload = function () {
    if (_this.http.status === 200) {
      callback(null, _this.http.responseText);
    } else {
      callback('Error: ' + _this.http.status);
    }
  };

  this.http.send();
}; // HTTP POST Request


simpleAJAX.prototype.post = function (url, data, callback) {
  var _this2 = this;

  this.http.open('POST', url, true);
  this.http.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');

  this.http.onload = function () {
    callback(null, _this2.http.responseText);
  };

  this.http.send(new URLSearchParams(data).toString());
}; 


function gdataCallback(err, data)
{
    if (err) alert(err);
    else try {
        var a,el,alrt=null;
        data=JSON.parse(data);
        for (a in data) {
            if (a == 'alert') {
                alrt = data[a];
                continue;
            }
            el=document.getElementById(a);
            if (!el) continue;
            if (el.tagName.toLowerCase() == "div") {
                el.className = data[a];
            }
            else if (el.tagName.toLowerCase() == "span") {
                if (a == 'firmversion') el.innerText = data[a];
                else el.className = data[a];
            }
            else if (el.tagName.toLowerCase() == "select") {
                el.selectedIndex=data[a];
            }
            else if (el.type == 'text' || el.type == "number") {
                el.value=data[a];
            }
            else if (el.type == 'checkbox') {
                el.checked = data[a] != '0';
            }
            
        }
        autoElev();
        if (alrt) alertia(alrt);
    }
    catch(e) {
        alert(e);
    }
}

function initMe()
{
    http.post("/getdata.cgi", {'placeholder': 0}, gdataCallback);
}

function autoTZ()
{
    var d=new Date();
    var off=new Date(d.getFullYear(),0,1).getTimezoneOffset();
    var off2 = new Date(d.getFullYear(),6,1).getTimezoneOffset();
    var el=document.getElementById('clkoffset');
    if (el) el.value=-off;
    el=document.getElementById('clkdst');
    if (el) el.checked=off != off2;
}

function cpress(a)
{
    return a.trim().replace(/\s+/g,' ');
}

function mkdata()
{
    var i,data,el,name,value,tval;
    data = new Object();
    for (i=0;i<arguments.length;i++) {
        name = arguments[i].split(':');
        
        if (name.length == 2) tval=name[1];
        else tval = null;
        name=name[0];
        
        el=document.getElementById(name);
        if (!el) continue;
        //console.log(name,el.tagName);
        if (el.tagName.toLowerCase() == 'select') value=el.options[el.selectedIndex].value;
        else if (el.type == 'text' || el.type == 'number') value=cpress(el.value);
        else if (el.type == 'checkbox') value = el.checked ? 1 : 0;
        else continue;
        if (tval == 'i') {
            if (!value.match(/^-?\d+$/)) {
                alertia("Błędny zapis liczby '"+value+"'");
                return null;
            }
        }

        data[name] = value;
    }
    return data;
}

function alertia(a)
{
    var dial=document.getElementById('alergia');
    if (!dial) {
        alert(a);
        return;
    }
    document.getElementById('alergen').innerText=a;
    dial.showModal();
}

function closeDialog()
{
    var dial=document.getElementById('alergia');
    if (dial) dial.close();
}


function unisetcb(err, data)
{
    if (err) alert(err);
    else alertia(data);
}

function unierrcb(err, data)
{
    if (err) alert(err);
}

function setTimezone()
{
    var data=mkdata('clkoffset:i','clkdst');
    if (!data) return;
    data['cmd']='settz';
//    console.log(data);
    http.post('/uniset.cgi',data,unisetcb);
    
}

function setWeastyle()
{
    var data=mkdata('weaspkinname','weaspkexname','weaspkpre:i','weaspkspl:i',
    'weaspkpres','weaspkrepres','weaspkhgrin','weaspkhgrout',
    'weaspkhly','weaspkchu','weaspktom','weaspkhall','weaspkcrmo');
    data['cmd']='setwea';
//    console.log(data);
    http.post('/uniset.cgi',data,unisetcb);
}    

function setGadacz()
{
    var data = mkdata('spktempo:i','spkpitch:i','spkvol:i','spkcon:i');
    data['cmd']='setspk';
//    console.log(data);
    http.post('/uniset.cgi',data,unisetcb);
}    
function setGeoloc()
{
    var data = mkdata('geolon','geolat','geoelev','autoelev','geoautz');
    data['cmd']='setgeo';
//    console.log(data);
    http.post('/uniset.cgi',data,unisetcb);
}


function saveData(arg)
{
    var data={'pref':arg};
    http.post('/unisave.cgi',data,unisetcb);
}
    
function revertData(arg)
{
    var data={'pref':arg};
    http.post('/unirev.cgi',data,gdataCallback);
}

function syncClock()
{
    http.post('/synclock.cgi',{'placeholder':'1'},unisetcb);
}

function speakDemo()
{
    http.post('/spkdemo.cgi',{'placeholder':'1'},unierrcb);
}

function autoElev()
{
    var el=document.getElementById("geoelev");
    var fl=document.getElementById("autoelev");
    el.disabled = fl.checked;
}

/* ap mode */

function ShowHideDHCP()
{
    var el=document.getElementById('dhcp');
    var fl=document.getElementById('hidiv');
    if (el && fl) fl.style.display = (el.selectedIndex == 0) ? 'block':'none';
}

var scanList;
var defWiFi="";
function selWiFi(event)
{
    var el=document.getElementById('ssidsel');
    var fl=document.getElementById('ssid');
    if (el.selectedIndex == 0) {
        fl.value=defWiFi;
        fl.disabled=false;
    }
    else {
        defWiFi=fl.value;
        fl.value=el.options[el.selectedIndex].value
        fl.disabled=true;
    }
}

function showWiFiScan(deflt)
{
    var el=document.getElementById('ssidsel'),opt;
    var a,i,di=0;
    defWiFi=deflt;
    for (i=0;i<scanList.length;i++) {
        a=scanList[i].match(/^0*(\d+?):(.*)$/);
        if (!a) continue;
        opt=document.createElement('option')
        opt.value=a[2]
        if (opt.value == deflt) {
            di=i+1;
            defFiWi="";
        }
        var vn=a[2];
        if (vn.length > 20) {
            vn=vn.substr(0,15) + '…' + vn.substr(-4);
        }
            
        opt.appendChild(document.createTextNode(vn+ ' -' + a[1] + 'dB'));
        if (a[2] != vn) opt.title=a[2];
        el.appendChild(opt);
    }
    el.selectedIndex=di;
    el.onchange=selWiFi;
    selWiFi(null);

}

function getapcb(err,data)
{
    if (err) {
        alert(err);
        return;
    }
    try {
        data=JSON.parse(data);
        var cred=data['cred'];
        var a,v,el,fl;
        for (a in cred) {
            el=document.getElementById(a);
            if (!el) continue;
            if (a == "ipns2" && cred[a] == "0.0.0.0") cred[a] = "";
            if (el.tagName.toLowerCase() == 'select') {
                el.selectedIndex=cred[a];
            }
            else {
                el.value=cred[a];
            }
            //console.log(a,cred[a]);
        }
        scanList = data['scanned'];
        scanList.sort();
        showWiFiScan(cred['ssid']);
    } catch(e) {
        alert(e);
    }
    
}
        
function initAp()
{
    http.post('/getap.cgi',{'placeholder':null},getapcb);
}


function badips(data)
{
    //console.log("BADIPS",data);
    const ipv4Pattern = /^(25[0-5]|2[0-4]\d|1\d{2}|\d{1,2})(\.(25[0-5]|2[0-4]\d|1\d{2}|\d{1,2})){3}$/;
    var a=['ipadr','ipmask','ipgw','ipns1','ipns2'];
    var i;
    for (i=0;i<a.length;i++) {
        //console.log(a[i],data[a[i]]);
        if (data[a[i]] == '') {
            if (a[i] == 'ipns2') continue;
            return "Adres IP nie może być pusty";
        }
        if (!ipv4Pattern.test(data[a[i]])) {
            return "Błędny zapis adresu IP: "+data[a[i]];
        }
    }
    return null;
}

function unierrcb1(err, data)
{
    if (err) alert(err);else alertia(data);
}

function storeWiFi()
{
    var data=mkdata('ssid','passwd','dhcp','ipadr','ipmask','ipgw',
        'ipns1','ipns2','mdns');
    if (data['ssid'] == '') {
        alertia("SSID nie może być pusty");
        return;
    }
    //console.log('DHCP',data['dhcp']);
    if (data['dhcp'] == "0") {
        var err=badips(data);
        if (err) {
            alertia(err);
            return;
        }
    }
    //console.log(data);
    http.post('/setap.cgi',data,unierrcb1);

}
// kalendarz

function monat(i)
{
    var s=document.getElementById('newcalmon');
    if (!s) s=document.getElementById('newevtmon');
    return s.options[i-1].text;
}

var caldata;

function dropRow(div)
{
    par = div.parentNode;
    par.removeChild(div);
    if (par.childNodes.length == 1) par.removeChild(par.firstChild);
}

function removeCalRow(event)
{
    var but=event.target;
    var i, fnd=-1;
    for (i=0;i<caldata.length;i++) {
        //console.log(i,but.mday,but.month,caldata[i]);
        if (caldata[i][0] == but.mday && caldata[i][1] == but.month) {
            fnd=i;
            break;
        }
    }
    if (fnd < 0) return;
    caldata.splice(fnd,1);
    //console.log(caldata)
    var div=but.parentNode.parentNode;
    var dav=div.nextSibling;
    if (!dav && div.previousSibling.role != 'caption') dav=div.previousSibling;
    if (dav) dav=dav.firstChild;
    else dav=document.getElementById('newcalday');
    dropRow(div);
    //div.parentNode.removeChild(div);
    ariaAlert("Wpis został usunięty");
    dav.focus();
    
}


function createCalRow(row)
{
    var i,dva,spn,tx,btn;
    
        dva=document.createElement('div');
        dva.setAttribute('aria-role','row');
        dva.setAttribute('role','row');
        dva.className = 'kxca kx';
        spn=document.createElement('span');
        spn.setAttribute('aria-role','gridcell');
        spn.setAttribute('role','gridcell');
        spn.setAttribute('tabIndex','0');
        spn.className = 'spninfo';
        tx=row[0] + " "+monat(row[1])+" "+row[2];
        spn.appendChild(document.createTextNode(tx));
        dva.appendChild(spn);
        row.push(dva)
        spn=document.createElement('span');
        spn.setAttribute('aria-role','gridcell');
        spn.setAttribute('role','gridcell');
        spn.className = 'spnbut';
        btn=document.createElement('button');
        spn.appendChild(btn)
        dva.appendChild(spn);
        btn.appendChild(document.createTextNode("Usuń"))
        spn=document.createElement('span');
        spn.className="aural";
        spn.appendChild(document.createTextNode(' wpis '+tx));
        btn.appendChild(spn);
        btn.mday=row[0];
        btn.month=row[1];
        btn.onclick=removeCalRow;
        return row;
}

function remakeCalRow(idx, txt)
{
    var row=caldata[idx]
    row[2]=txt;
    var tx=tx=row[0] + " "+monat(row[1])+" "+row[2];
    var spans=row[3].getElementsByTagName('span');
    var span=spans[0];
    while (span.firstChild) span.removeChild(span.firstChild);
    span.appendChild(document.createTextNode(tx));
    var i;
    for (i=0;i<spans.length;i++) if (spans[i].className == 'aural') {
        span=spans[i];
        while (span.firstChild) span.removeChild(span.firstChild);
        span.appendChild(document.createTextNode(' wpis '+tx));
        break;
    }
}

function insertCalRow(row)
{
    var i,fnd;
    for (i=0,fnd=-1;i<caldata.length;i++) {
        if (caldata[i][1] > row[1] || (caldata[i][1] == row[1] && caldata[i][0] > row[0])) {
            fnd=i;
            break;
        }
    }
    var cd=document.getElementById('caldar');
    insertCaption(cd,'Wpisy kalendarza');
    
    if (fnd < 0) {
        caldata.push(row)
        cd.appendChild(row[3]);
        return;
    }
    cd.insertBefore(row[3],caldata[fnd][3]);
    caldata.splice(fnd,0,row);
}



function addCalItem()
{
    var day=cpress(document.getElementById('newcalday').value);
    if (!day.match(/^(([1-9])|([12]\d)|(3[01]))$/)) {
        alertia("Błędny dzień");
        return;
    }
    var mon=document.getElementById('newcalmon').selectedIndex+1;
    var txt=cpress(document.getElementById('newcaltxt').value);
    if (txt.length < 3) {
        alertia("Za krótki opis");
        return;
    }
    var i;
    var fnd=-1;
    for (i=0;i<caldata.length;i++) if (caldata[i][0] == day && caldata[i][1] == mon) {
        fnd = i;
        break;
    }
    if (fnd >= 0) {
        remakeCalRow(fnd,txt);
        document.getElementById('newcalday').value='';
        document.getElementById('newcaltxt').value='';
        ariaAlert("Zmieniono istniejący wpis");
        caldata[fnd][3].firstChild.focus();

        return;
    }
    if (caldata.length >=32) {
        alertia("Za dużo wpisów");
        return;
    }
    
    var row=[day,mon,txt];
    createCalRow(row);
    insertCalRow(row);
    document.getElementById('newcalday').value='';
    document.getElementById('newcaltxt').value='';
    
    ariaAlert("Wstawiono nowy wpis");
    row[3].firstChild.focus();
}

function ariaAlert(s)
{
    var el=document.getElementById('arialert');
    el.innerText = "";
    el.innerText = s;
}


function initCalData(data)
{
    var cd=document.getElementById('caldar');
    while (cd.firstChild) cd.removeChild(cd.firstChild);
    var i,dva,spn,tx,btn;
    var calen=data['calendar'];
    for (i=0;i<calen.length;i++) {
        row=createCalRow(calen[i]);
        insertCalRow(row);
    }
    if (data['dualhead']) {
        document.getElementById('linkcontainer').className="kx kxdouble";
    }
    document.getElementById('newcalday').value='';
    document.getElementById('newcaltxt').value='';
}


function loadCal(err,str)
{
    if (err) {
        alert(err);
        return;
    }
    var data;
    try {
        data=JSON.parse(str);
    }
    catch(e) {
        alert(str);
        return;
    }
    caldata=[];
    initCalData(data);
}

function initCal()
{
    http.post('/getcal.cgi',{'placeholder':null},loadCal);
}

function revertCalendar()
{
    http.post('/getcal.cgi',{'placeholder':null},loadCal);  
}
function saveCalendar()
{
    var i;
    var d=[];
    for (i=0;i<caldata.length;i++) {
        d.push(caldata[i][0]+' '+caldata[i][1]+' '+caldata[i][2]);
    }
    d=d.join('\n');
    http.post('/setcal.cgi',{'cal':d},unierrcb1);
    
}

// timetable

function evtPrepTomorrow()
{
    var d=new Date();
    var el=document.getElementById('newevtyer');
    d.setDate(d.getDate()+1)
    var y=d.getFullYear()
    var ydx=0,i;
    for (i=0;i<4;i++) {
        //console.log(el.options[i].value, y);
        if (el.options[i].value == y) {
            ydx=i;
            break;
        }
    }
    el.selectedIndex=ydx;
    document.getElementById('newevtmon').selectedIndex=d.getMonth();
    document.getElementById('newevtday').selectedIndex=d.getDate();
    document.getElementById('newevttxt').value="";
}

var evtdata;

function removeEvtRowCb(err, str)
{
    if (err) {
        alert(err);
        return;
    }
    var data;
    try {
        data=JSON.parse(str);
    }
    catch(e) {
        alert(str);
        return;
    }
    if (data.error) {
        alert(data.error);
        return;
    }
    var idx=data.idx;
    var i, fnd=-1;
    for (i=0;i<evtdata.length;i++) {
        if (evtdata[i][3] == idx) {
            fnd=i;
            break;
        }
    }
    if (fnd < 0) return;
    var div =evtdata[fnd][5];
    evtdata.splice(fnd,1);
    var dav=div.nextSibling;
    if (!dav && div.previousSibling.role !='caption') dav=div.previousSibling;
    if (dav) dav=dav.firstChild;
    else dav=document.getElementById('newevtday');
    dropRow(div);
    ariaAlert("Wpis został usunięty");
    dav.focus();
}

function removeEvtRow(event)
{
    var but=event.target;
    //console.log("REM",but.idx);
    http.post("/delevt.cgi",{"index":but.idx},removeEvtRowCb);
}

function insertCaption(div,tit)
{
    if (div.childNodes.length == 0) {
        var cell=document.createElement(cell);
        cell.innerText = tit;
        cell.className = 'aural';
        cell.setAttribute('role','caption');
        cell.setAttribute('aria-role','caption');
        div.appendChild(cell);
    }
}
function insertEvtRow(row)
{
    var i,fnd;
    for (i=0,fnd=-1;i<evtdata.length;i++) {
        if (evtdata[i][2] < row[2]) continue;
        if (evtdata[i][2] > row[2]) {fnd=i;break;}
        if (evtdata[i][1] < row[1]) continue;
        if (evtdata[i][1] > row[1]) {fnd=i;break;}
        if (evtdata[i][0] > row[0]) {fnd=i;break;}
    }
    var cd=document.getElementById('evdiv');
    insertCaption(cd,'Wpisy terminarza');
    if (fnd < 0) {
        evtdata.push(row)
        cd.appendChild(row[5]);
        return;
    }
    cd.insertBefore(row[5],evtdata[fnd][5]);
    evtdata.splice(fnd,0,row);
}

function createEvtRow(row)
{
    var i,dva,spn,tx,btn;

    dva=document.createElement('div');
    dva.setAttribute('aria-role','row');
    dva.setAttribute('role','row');
    dva.className = 'kxca kx';
    spn=document.createElement('span');
    spn.setAttribute('aria-role','gridcell');
    spn.setAttribute('role','gridcell');
    spn.setAttribute('tabIndex','0');
    spn.className = 'spninfo';
    tx=row[0] + " "+monat(row[1])+" 20"+row[2]+" "+row[4];
    spn.appendChild(document.createTextNode(tx));
    dva.appendChild(spn);
    row.push(dva)
    spn=document.createElement('span');
    spn.setAttribute('aria-role','gridcell');
    spn.setAttribute('role','gridcell');
    spn.className = 'spnbut';
    btn=document.createElement('button');
    spn.appendChild(btn)
    dva.appendChild(spn);
    btn.appendChild(document.createTextNode("Usuń"))
    spn=document.createElement('span');
    spn.className="aural";
    spn.appendChild(document.createTextNode(' wpis '+tx));
    btn.appendChild(spn);
    btn.mday=row[0];
    btn.month=row[1];
    btn.year=row[2];
    btn.idx=row[3];
    btn.onclick=removeEvtRow;
    return row;
}


function loadEvts(err,str)
{
    if (err) {
        alert(err);
        return;
    }
    var data;
    try {
        data=JSON.parse(str);
    }
    catch(e) {
        alert(str);
        return;
    }
    if (data.error) {
        alert(data.error);
        return;
    }
    var el=document.getElementById('evdiv');
    while (el.firstChild) el.removeChild(el.firstChild);
    var lista=data.events;
    evtdata=[];
    for (i=0;i<lista.length;i++) {
        row=createEvtRow(lista[i]);
        insertEvtRow(row);
    }
    if (data.message) alert(data.message);
    //console.log(evtdata);
}


function getEvts()
{
    http.post('/getevt.cgi',{'placeholder':null},loadEvts);  
}


function initEvt()
{
    var el=document.getElementById('newevtyer');
    while (el.firstChild) el.removeChild(el.firstChild);
    var d=new Date();
    var y=d.getFullYear();
    var i;
    for (i=0;i<4;i++) {
        var opt=document.createElement('option')
        opt.value=y+i;
        opt.appendChild(document.createTextNode(y+i));
        el.appendChild(opt);
    }
    evtPrepTomorrow();
    getEvts();
}

function addEvtItemCb(err, str)
{
       if (err) {
        alert(err);
        return;
    }
    var data;
    try {
        data=JSON.parse(str);
    }
    catch(e) {
        alert(str);
        return;
    }
    if (data.error) {
        alert(data.error);
        return;
    }
    //console.log(data.result)
    var row=createEvtRow(data.result);
    insertEvtRow(row);
    document.getElementById('newevtday').value=''
    ariaAlert("Dodano nowy wpis")
    //console.log(row[5].firstChild)
    row[5].firstChild.focus();
    
}


function addEvtItem()
{
    var day=cpress(document.getElementById('newevtday').value);
    if (!day.match(/^(([1-9])|([12]\d)|(3[01]))$/)) {
        alertia("Błędny dzień");
        return;
    }
    var mon=document.getElementById('newevtmon').selectedIndex+1;
    var yel=document.getElementById('newevtyer')
    var year = yel.options[yel.selectedIndex].value;
    var txt=cpress(document.getElementById('newevttxt').value);
    if (txt.length < 3) {
        alertia("Za krótki opis");
        return;
    }
    http.post("/addevt.cgi",{
        "day":day,
        "month": mon,
        "year": year,
        "data": txt}, addEvtItemCb);
}

function saveTimetab()
{
    http.post("/savevt.cgi",{"placeholder":null},unisetcb);
}

function revertTimetab()
{
    http.post("/revevt.cgi",{"placeholder":null},loadEvts);
}

function setCity(event)
{
    event.preventDefault();
    var str=event.target.innerText;
    var lon = str.match(/\d\d°\d\d'\d\d"E/)[0];
    var lat = str.match(/\d\d°\d\d'\d\d"N/)[0];
    document.getElementById('geolon').value=lon;
    document.getElementById('geolat').value=lat;
    document.getElementById('autoelev').checked=true;
    document.getElementById('geoelev').disabled=true;
    document.getElementById('geoautz').selectedIndex=0;
    document.getElementById('miastodont').close();
    var nm=str.split(',')
    ariaAlert('Ustawiono '+nm[0]+' '+nm[1])
    closeMiast();
    document.getElementById('setgeoloc').focus();
    
    
}


function loadCities(err, data)
{
    if (err) {
        alertia(err);
        return;
    }
    console.log(data)
    try {
        data = JSON.parse(data);
    }
    catch(e) {
        alert(e);
        return;
    }
    if (data.error) {
        alertia(data.error);
        return;
    }
    data=data.data;
    var mdt=document.getElementById('citylist');
    while(mdt.firstChild) mdt.removeChild(mdt.firstChild);
    var ss,i;
    for (i=0;i<data.length;i++) {
        var li=document.createElement('li');
        var aa=document.createElement('a');
        li.appendChild(aa)
        var str=data[i]
        var fs=str.match(/^(.*\()(.*?\))$/)
        if (!fs) {
            aa.innerText=data[i];
        }
        else {
            aa.appendChild(document.createTextNode(fs[1]))
            aa.appendChild(document.createElement("span"))
            aa.lastChild.className='aural'
            aa.lastChild.innerHTML='dla miejscowości '
            aa.appendChild(document.createTextNode(fs[2]))
        }
        aa.href='#';
        aa.onclick=setCity;
        mdt.appendChild(li);
    }
    document.getElementById('cityheader').innerText =
        'Znaleziono '+i+((i==1) ? ' miejscowość' : ' miejscowości');
    document.getElementById('miastodont').showModal();
}

function citysearch()
{
    var txt=cpress(document.getElementById('cityfdr').value);
    if (txt.length < 2) {
        alertia("Tekst jest za krótki");
        return;
    }
    http.post("/city.cgi",{"city":txt},loadCities);
}

function closeMiast()
{
    document.getElementById('miastodont').close();
}

//ota
unierrcb
function checkOtaCB(err,data)
{
    if (err) {
        alert(err);
        return;
    }
    try {
        data=JSON.parse(data);
    }
    catch(e) {
        alert(e);
        return;
    }
    if (!data.doit) {
        alertia(data.message);
        return;
    }
    if (!confirm(data.message+'.\nCzy aktualizować?')) return;
    http.post("/otaupdate.cgi",{'placeholder':''},unierrcb);
}


function checkupdate()
{
    http.post("/checkota.cgi",{'placeholder':''},checkOtaCB);
}
