#**********************************************************
#* config keywords definition
#* $Id: qconf.x,v 1.1.1.1 2003/07/12 21:27:11 sisoft Exp $
#**********************************************************
address,C_ADDRL,falist_t *,1,NULL
aftermail,C_STR,char *,0,NULL
aftersession,C_STR,char *,0,NULL
alwayskillfiles,C_YESNO,int,0,"no"
alwaysoverwrite,C_STRL,slist_t *,0,NULL
autoskip,C_STRL,slist_t *,0,NULL
autosuspend,C_STRL,slist_t *,0,NULL
autoticskip,C_YESNO,int,0,"no"
callonflavors,C_STR,char *,0,"NDCIR"
cancall,C_STR,char *,0,"CM"
chathallostr,C_STR,char *,0,"\n * Hallo %s!\n"
chatlog,C_PATH,char *,0,NULL
chatlognetmail,C_YESNO,int,0,"no"
chattoemail,C_STR,char *,0,NULL
clearundial,C_INT,int,0,"0"
defperm,C_OCT,int,0,"644"
dialdelay,C_INT,int,0,"60"
dialdelta,C_INT,int,0,"0"
dialprefix,C_STR,char *,0,"ATD"
dialsuffix,C_STR,char *,0,"|"
dirperm,C_OCT,int,0,"755"
emsifreqtime,C_STR,char *,0,NULL
emsilog,C_PATH,char *,0,NULL
estimatedtime,C_YESNO,int,0,"no"
extrp,C_STR,char *,0,NULL
fails_hold_div,C_INT,int,0,"0"
fails_hold_time,C_INT,int,0,"0"
filebox,C_ADRSTRL,faslist_t *,0,NULL
flags,C_STR,char *,0,""
freqfrom,C_STR,char *,0,"Freq Processor"
freqsubj,C_STR,char *,0,"File request report"
freqtime,C_STR,char *,0,NULL
history,C_PATH,char *,0,NULL
holdonnodial,C_INT,int,0,"0"
holdonsuccess,C_INT,int,0,"0"
hrxwin,C_INT,int,0,"0"
hstimeout,C_INT,int,0,"60"
htxwin,C_INT,int,0,"0"
ignorenrq,C_YESNO,int,0,"no"
immonflavors,C_STR,char *,0,"CI"
inbound,C_PATH,char *,1,NULL
ipcperm,C_OCT,int,0,"666"
jrxwin,C_INT,int,0,"0"
jtxwin,C_INT,int,0,"0"
lockdir,C_PATH,char *,1,NULL
log,C_PATH,char *,1,NULL
loglevels,C_STR,char *,0,""
longboxpath,C_PATH,char *,0,NULL
longrescan,C_INT,int,0,"0"
mailonly,C_STR,char *,0,NULL
mapin,C_STR,char *,0,NULL
mapout,C_STR,char *,0,NULL
mappath,C_STRL,slist_t *,0,NULL
masterlog,C_PATH,char *,1,NULL
max_fails,C_INT,int,0,"20"
maxrings,C_INT,int,0,"0"
maxsession,C_INT,int,0,"0"
mincpsdelay,C_INT,int,0,"10"
mincpsin,C_INT,int,0,"0"
mincpsout,C_INT,int,0,"0"
minspeed,C_INT,int,0,"0"
modemalive,C_STR,char *,0,"AT|"
modembusy,C_STRL,slist_t *,1,NULL
modemconnect,C_STRL,slist_t *,1,NULL
modemerror,C_STRL,slist_t *,1,NULL
modemhangup,C_STRL,slist_t *,0,NULL
modemnodial,C_STRL,slist_t *,0,NULL
modemok,C_STRL,slist_t *,1,NULL
modemreset,C_STRL,slist_t *,0,NULL
modemringing,C_STR,char *,0,"RINGING"
modemstat,C_STRL,slist_t *,0,NULL
needalllisted,C_YESNO,int,0,"no"
nlpath,C_PATH,char *,1,NULL
nodelist,C_STRL,slist_t *,1,NULL
nodial,C_PATH,char *,0,NULL
osname,C_STR,char *,0,NULL
outbound,C_PATH,char *,1,NULL
password,C_ADRSTRL,faslist_t *,0,NULL
phone,C_STR,char *,0,""
phonetr,C_STRL,slist_t *,1,NULL
pidfile,C_PATH,char *,1,NULL
place,C_STR,char *,0,""
pollflavor,C_STR,char *,0,"N"
port,C_STRL,slist_t *,1,NULL
progname,C_STR,char *,0,NULL
protorder,C_STR,char *,0,"JHZ1C"
qstoutbound,C_PATH,char *,0,NULL
realmincps,C_YESNO,int,0,"yes"
recodetable,C_PATH,char *,0,NULL
remoterecode,C_YESNO,int,0,"yes"
rescanperiod,C_INT,int,0,"300"
rootdir,C_PATH,char *,0,NULL
runonchat,C_STR,char *,0,NULL
runonemsi,C_STR,char *,0,NULL
showintro,C_YESNO,int,0,"yes"
showpkt,C_YESNO,int,0,"no"
speed,C_INT,int,0,"300"
srifrp,C_PATH,char *,0,NULL
standardemsi,C_YESNO,int,0,"yes"
station,C_STR,char *,0,""
subst,C_ADRSTRL,faslist_t *,0,NULL
sysop,C_STR,char *,0,""
translatesubst,C_YESNO,int,0,"no"
useproctitle,C_YESNO,int,0,"yes"
version,C_STR,char *,0,NULL
waitcarrier,C_INT,int,0,"60"
waithrq,C_INT,int,0,"60"
waitreset,C_INT,int,0,"10"
worktime,C_STR,char *,0,""
zmh,C_STR,char *,0,NULL
zrxwin,C_INT,int,0,"0"
ztxwin,C_INT,int,0,"0"
