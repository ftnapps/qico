protorder,C_STR,char *,0,"RHZ1"
mailonly,C_STR,char *,0,NULL
zmh,C_STR,char *,0,NULL
freqtime,C_STR,char *,0,NULL
station,C_STR,char *,0,""
place,C_STR,char *,0,""
sysop,C_STR,char *,0,""
worktime,C_STR,char *,0,""
flags,C_STR,char *,0,""
phone,C_STR,char *,0,""
speed,C_INT,int,0,"300"
address,C_ADDRL,falist_t *,1,NULL
nlpath,C_PATH,char *,1,NULL
nodelist,C_STRL,slist_t *,1,NULL
outbound,C_PATH,char *,1,NULL
log,C_PATH,char *,1,NULL
masterlog,C_PATH,char *,1,NULL
port,C_STRL,slist_t *,1,NULL
dialdelay,C_INT,int,0,"60"
dialdelta,C_INT,int,0,"0"
rescanperiod,C_INT,int,0,"300"
subst,C_ADRSTRL,faslist_t *,0,NULL
phonetr,C_STRL,slist_t *,1,NULL
cancall,C_STR,char *,0,"CM"
modemok,C_STRL,slist_t *,1,NULL
modemconnect,C_STRL,slist_t *,1,NULL
modemerror,C_STRL,slist_t *,1,NULL
modembusy,C_STRL,slist_t *,1,NULL
modemreset,C_STRL,slist_t *,0,NULL
modemhangup,C_STRL,slist_t *,0,NULL
dialprefix,C_STR,char *,0,"ATD"
dialsuffix,C_STR,char *,0,"|"
max_fails,C_INT,int,0,"20"
waitreset,C_INT,int,0,"5"
waitcarrier,C_INT,int,0,"60"
password,C_ADRSTRL,faslist_t *,0,NULL
inbound,C_STR,char *,1,NULL
nodial,C_PATH,char *,0,NULL
skipfiles,C_STR,char *,0,NULL
showintro,C_YESNO,int,0,"yes"
maxsession,C_INT,int,0,"0"
waithrq,C_INT,int,0,"60"
modemringing,C_STR,char *,0,"RINGING"
maxrings,C_INT,int,0,"0"
extrp,C_PATH,char *,0,NULL
freqfrom,C_STR,char *,0,"Freq Processor"
freqsubj,C_STR,char *,0,"File Request Report"
history,C_PATH,char *,0,NULL
filebox,C_ADRSTRL,faslist_t *,0,NULL
longboxpath,C_PATH,char *,0,NULL
mappath,C_STRL,slist_t *,0,NULL
mapout,C_STR,char *,0,NULL
mapin,C_STR,char *,0,NULL
minspeed,C_INT,int,0,"0"
autoskip,C_STRL,slist_t *,0,NULL
aftersession,C_STR,char *,0,NULL
emsilog,C_STR,char *,0,NULL
pidfile,C_STR,char *,0,NULL
defperm,C_OCT,int,0,"644"
dirperm,C_OCT,int,0,"755"
ipcperm,C_OCT,int,0,"666"
progname,C_STR,char *,0,NULL
version,C_STR,char *,0,NULL
osname,C_STR,char *,0,NULL
mincpsin,C_INT,int,0,"0"
mincpsout,C_INT,int,0,"0"
mincpsdelay,C_INT,int,0,"10"
lockdir,C_STR,char *,1,NULL
clearundial,C_INT,int,0,"0"
aftermail,C_STR,char *,0,NULL
