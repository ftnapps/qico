/******************************************************************
 * common protocols' file management
 * $Id: protfm.c,v 1.13 2004/02/14 15:58:54 sisoft Exp $
 ******************************************************************/
#include "headers.h"
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#include <fnmatch.h>
#include "hydra.h"
#include "ls_zmodem.h"
#include "binkp.h"
#include "qipc.h"
#include "tty.h"

/*  Common protocols' vars */
FILE   *txfd=NULL,     *rxfd=NULL;
long    txpos,          rxpos;
word    txretries,      rxretries;
long    txwindow,       rxwindow;
word    txblklen,       rxblklen;
long    txsyncid,       rxsyncid;
byte   *txbuf,         *rxbuf;
dword   txoptions,      rxoptions;
unsigned effbaud=9600;
byte    *rxbufptr;
byte    txstate,        rxstate;
byte    *rxbufmax;
long    txstart,        rxstart;
word    txmaxblklen;
word    timeout;
byte    txlastc;

#define CHAT_BUF 16384
static unsigned char qrcv_buf[MSG_BUFFER]={0};
static unsigned char qsnd_buf[CHAT_BUF]={0};
static unsigned char ubuf[CHAT_BUF];
static char hellostr[MAX_STRING];
static unsigned short qsndbuflen=0;

static char weskipstr[]="recd: %s, 0 bytes, 0 cps [%sskipped]";
static char wesusstr[]="recd: %s, 0 bytes, 0 cps [%ssuspended]";

static int sifname(char *s)
{
	static char ALLOWED_CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
	static int CHARS = sizeof(ALLOWED_CHARS) / sizeof(char);
	int i;
	for(i = 0; i < CHARS; i++) if(*s < ALLOWED_CHARS[i]) { *s = ALLOWED_CHARS[i]; break; }
	if(i == CHARS) return 1;
	return 0;
}

char *estimatedtime(size_t size, int cps, unsigned long baud)
{
	static char et[16];
	int h,m,s;
	if (cps < 1) cps = baud / 10;
	if (!cps) cps = 1;
	s = size / cps;
	if (s < 1) s = 1;
	h = s / 3600; s %= 3600;
	m = s / 60;   s %= 60;
	snprintf(et,16,"%02d:%02d:%02d",h,m,s);
	return et;
}

int rxopen(char *name, time_t rtime, size_t rsize, FILE **f)
{
	struct stat sb;
	slist_t *i;
	char p[MAX_PATH], bn[MAX_PATH];
	int prevcps = (recvf.start&&(time(NULL)-recvf.start>2))?recvf.cps:effbaud/10,rc;
	xstrcpy(bn, basename(name), MAX_PATH);
	mapname((char*)bn, cfgs(CFG_MAPIN), MAX_PATH);
 	recvf.start=time(NULL);
	xfree(recvf.fname);
 	recvf.fname=xstrdup(bn);
	recvf.mtime=rtime;recvf.ftot=rsize;
	if(recvf.toff+rsize > recvf.ttot) recvf.ttot+=rsize;
	recvf.nf++;if(recvf.nf>recvf.allf) recvf.allf++;
	if(whattype(name)==IS_PKT&&rsize==60)return FOP_SKIP;
	rc=skipiftic;skipiftic=0;
	if(rc&&istic(bn)&&cfgi(CFG_AUTOTICSKIP)) {
		write_log(rc==FOP_SKIP?weskipstr:wesusstr,recvf.fname,"auto");
		return rc;
	}
	for(i=cfgsl(CFG_AUTOSKIP);i;i=i->next)
		if(!xfnmatch(i->str, bn, FNM_PATHNAME)) {
			write_log(weskipstr,recvf.fname,"");
			skipiftic=FOP_SKIP;
			return FOP_SKIP;
		}
	for(i=cfgsl(CFG_AUTOSUSPEND);i;i=i->next)
		if(!xfnmatch(i->str, bn, FNM_PATHNAME)) {
			write_log(wesusstr,recvf.fname,"");
			skipiftic=FOP_SUSPEND;
			return FOP_SUSPEND;
		}

	snprintf(p, MAX_PATH, "%s/tmp/", cfgs(CFG_INBOUND));
	if(stat(p, &sb))
		if(mkdirs(p) && errno!=EEXIST) {
			write_log("can't make directory %s: %s", p, strerror(errno));
			write_log(wesusstr,recvf.fname,"");
			skipiftic=FOP_SUSPEND;
			return FOP_SUSPEND;
		}
	snprintf(p, MAX_PATH, "%s/%s", ccs, bn);

	if(!stat(p, &sb) && sb.st_size==rsize) {
		write_log(weskipstr,recvf.fname,"");
		skipiftic=FOP_SKIP;
		return FOP_SKIP;
	}

	snprintf(p, MAX_PATH, "%s/tmp/%s", ccs, bn);

	if(!stat(p, &sb)) {
		if(sb.st_size<rsize && sb.st_mtime==rtime) {
			*f=fopen(p, "ab");
			if(!*f) {
				write_log("can't open file %s for writing: %s", p,strerror(errno));
				write_log(wesusstr,recvf.fname,"");
				skipiftic=FOP_SUSPEND;
				return FOP_SUSPEND;
			}
			recvf.foff=recvf.soff=ftell(*f);
			if(cfgi(CFG_ESTIMATEDTIME)) {
				write_log("start recv: %s, %d bytes (from %d), estimated time %s",
					recvf.fname, rsize, recvf.soff, estimatedtime(rsize-recvf.soff,prevcps,effbaud));
			}
			return FOP_CONT;
		}
	}

	*f=fopen(p, "wb");
	if(!*f) {
		write_log("can't open file %s for writing: %s", p,strerror(errno));
		write_log(wesusstr,recvf.fname,"");
		skipiftic=FOP_SUSPEND;
		return FOP_SUSPEND;
	}
	recvf.foff=recvf.soff=0;
	if(cfgi(CFG_ESTIMATEDTIME)) {
		write_log("start recv: %s, %d bytes, estimated time %s",
			recvf.fname, rsize, estimatedtime(rsize,prevcps,effbaud));
	}
	return FOP_OK;
}

int rxclose(FILE **f, int what)
{
	int cps=time(NULL)-recvf.start,rc,overwrite;
	char *ss, p[MAX_PATH], p2[MAX_PATH];
	struct utimbuf ut;struct stat sb;
	slist_t *i;

	if(!f || !*f) return FOP_ERROR;
	recvf.toff+=recvf.foff;recvf.stot+=recvf.soff;

	if(!cps) cps=1;cps=(recvf.foff-recvf.soff)/cps;
	switch(what) {
		case FOP_SUSPEND: ss="suspended";break;
		case FOP_SKIP: ss="skipped";break;
		case FOP_ERROR: ss="error";break;
		case FOP_OK: ss="ok";break;
		default: ss="";
	}
	if(recvf.soff)
		write_log("recd: %s, %d bytes (from %d), %d cps [%s]",
			recvf.fname, recvf.foff, recvf.soff, cps, ss);
	else
		write_log("recd: %s, %d bytes, %d cps [%s]",
			recvf.fname, recvf.foff, cps, ss);
	fclose(*f);*f=NULL;
	snprintf(p, MAX_PATH, "%s/tmp/%s", cfgs(CFG_INBOUND), recvf.fname);
	snprintf(p2, MAX_PATH, "%s/%s", cfgs(CFG_INBOUND), recvf.fname);
	ut.actime=ut.modtime=recvf.mtime;
	recvf.foff=0;
	switch(what) {
	case FOP_SKIP:
		lunlink(p);
		break;
	case FOP_SUSPEND:
	case FOP_ERROR:
		if(whattype(p)==IS_PKT&&cfgi(CFG_KILLBADPKT))lunlink(p);
		    else utime(p,&ut);
		break;
	case FOP_OK:
		rc=receive_callback?receive_callback(p):0;
		if(rc) lunlink(p);
		else {
			ss=p2+strlen(p2)-1;overwrite=0;
			for(i=cfgsl(CFG_ALWAYSOVERWRITE);i;i=i->next)
			    if(!xfnmatch(i->str,recvf.fname,FNM_PATHNAME))
				overwrite=1;
			while(!overwrite&&!stat(p2, &sb)&&p2[0]) {
				if(sifname(ss)) {
					ss--;
					while('.' == *ss && ss >= p2) ss--;
					if(ss < p2) {
						write_log("can't find situable name for %s: leaving in temporary directory",p);
						p2[0] = '\x00';
					}
				}
			}
			if(p2[0]) {
				if(overwrite)lunlink(p2);
				if(rename(p, p2)) {
					write_log("can't rename %s to %s: %s",p,p2,strerror(errno));
				} else {
					utime(p2,&ut);chmod(p2,cfgi(CFG_DEFPERM));
				}
			}
		}
		break;
	}
	if(what==FOP_SKIP||what==FOP_SUSPEND)skipiftic=what;
	recvf.start=0;recvf.ftot=0;
	rxstatus=0;
	return what;
}

FILE *txopen(char *tosend, char *sendas)
{
	FILE *f;
	struct stat sb;
	int prevcps = (sendf.start&&(time(NULL)-sendf.start>2))?sendf.cps:effbaud/10;

	if(stat(tosend, &sb)) {
		write_log("can't find file %s!", tosend);
		return NULL;
	}
	if(whattype(sendas)==IS_PKT&&sb.st_size==60)return NULL;
	xfree(sendf.fname);
 	sendf.fname=xstrdup(sendas);
	sendf.ftot=sb.st_size;sendf.mtime=sb.st_mtime;
	sendf.foff=sendf.soff=0;sendf.start=time(NULL);
	if(sendf.toff+sb.st_size > sendf.ttot) sendf.ttot+=sb.st_size;
	sendf.nf++;if(sendf.nf>sendf.allf) sendf.allf++;
	f=fopen(tosend, "rb");
	if(!f) {
		write_log("can't open file %s for reading: %s", tosend,strerror(errno));
		return NULL;
	}
	if(cfgi(CFG_ESTIMATEDTIME)) {
		write_log("start send: %s, %d bytes, estimated time %s",
			sendf.fname, sendf.ftot, estimatedtime(sendf.ftot,prevcps,effbaud));
	}
	return f;
}

int txclose(FILE **f, int what)
{
	int cps=time(NULL)-sendf.start;
	char *ss;

	if(!f || !*f) return FOP_ERROR;
	sendf.toff+=sendf.foff;sendf.stot+=sendf.soff;

	if(!cps) cps=1;cps=(sendf.foff-sendf.soff)/cps;
	switch(what) {
		case FOP_SUSPEND: ss="suspended";break;
		case FOP_SKIP: ss="skipped";break;
		case FOP_ERROR: ss="error";break;
		case FOP_OK: ss="ok";break;
		default: ss="";
	}
	if(sendf.soff)write_log("sent: %s, %d bytes (from %d), %d cps [%s]",
		sendf.fname, sendf.foff, sendf.soff, cps, ss);
	    else write_log("sent: %s, %d bytes, %d cps [%s]",
		sendf.fname, sendf.foff, cps, ss);
	sendf.foff=0;sendf.ftot=0;
	sendf.start=0;
	fclose(*f);*f=NULL;
	return what;
}

void chatinit(int prot)
{
	xstrcpy(hellostr,"\05\05\05",MAX_STRING);
	strtr(cfgs(CFG_CHATHALLOSTR),'|','\n');strtr(ccs,'$','\a');
	snprintf(hellostr+3,MAX_STRING-7,ccs,rnode->sysop);
	stodos((unsigned char*)hellostr);
	xstrcat(hellostr,"\05\05\05",MAX_STRING);
	chattimer=0;
	chatlg=0;
	qsndbuflen=0;
	*qsnd_buf=0;
	calling=0;
	switch(prot) {
		case P_ZEDZAP:
		case P_DIRZAP:
		case P_ZMODEM:
			chatprot=P_ZMODEM;
			break;
		case P_HYDRA:
#ifdef HYDRA8K16K
		case P_HYDRA4:
		case P_HYDRA8:
		case P_HYDRA16:
#endif/*HYDRA8K16K*/
			chatprot=P_HYDRA;
			break;
		case P_JANUS:
			chatprot=P_JANUS;
			break;
		case 0:
			if(bink)chatprot=P_NCP;
	}
}

int c_devfree()
{
	int rc=0;
	switch(chatprot) {
		case P_ZMODEM:
			rc=z_devfree();
			break;
		case P_HYDRA:
			rc=hydra_devfree();
			break;
		case P_NCP:
			rc=bink_devfree();
	}
	return rc;
}

int c_devsend(unsigned char *str,unsigned len)
{
	int rc=0;
	switch(chatprot) {
		case P_ZMODEM:
			rc=z_devsend(str,len-1);
			break;
		case P_HYDRA:
			rc=hydra_devsend("CON",str,len);
			break;
		case P_NCP:
			rc=bink_devsend(str,len);
	}
	return rc;
}

int chatsend(unsigned char *str)
{
	int i;
	if(!str||!*str)return 0;
	if(!c_devfree())return 1;
	if(chattimer<2) {
		c_devsend((unsigned char*)hellostr,strlen(hellostr)+1);
		chatlg=chatlog_init(rnode->sysop,&rnode->addrs->addr,0);
		qchat("");
	} else if(*str!=5) {
		for(i=0;i<=strlen((char*)str)&&i<CHAT_BUF;i++)ubuf[i]=todos(str[i]);
		if(!c_devsend(ubuf,i-1))return 1;
		if(chatlg)chatlog_write((char*)str,0);
	} else write_log("Chat already opened!");
	chattimer=time(NULL)+TIM_CHAT;
	return 0;
}

void c_devrecv(unsigned char *data,unsigned len)
{
	int i;
	if(!data||!*data||!len)return;
	data[len]=0;
	if(chattimer<2) {
		char *p;
		if(len>5&&data[1]!=5&&(p=strstr(data," * "))&&strstr(p+3,"los"))return;
		c_devsend((unsigned char*)hellostr,strlen(hellostr));
		chatlg=chatlog_init(rnode->sysop,&rnode->addrs->addr,1);
	}
	stokoi(data);
	if(data[strlen((char*)data)-1]==5)data[strlen((char*)data)-1]='\n';
	for(len=i=0;len<=strlen((char*)data);len++)if(data[len]!=5)data[i++]=data[len];
	chattimer=time(NULL)+TIM_CHAT;
	if(chatlg)chatlog_write((char*)data,1);
	qchat((char*)data);
}

void getevt()
{
	int i;
	while(qrecvpkt((char*)qrcv_buf)) {
		switch(qrcv_buf[2]) {
			case QR_SKIP:
				rxstatus=RX_SKIP;
				break;
			case QR_REFUSE:
				rxstatus=RX_SUSPEND;
				break;
			case QR_HANGUP:
				tty_hangedup=1;
				break;
			case QR_CHAT:
				if(qrcv_buf[3]) {
					xstrcpy((char*)(qsnd_buf+qsndbuflen),(char*)(qrcv_buf+3),CHAT_BUF-qsndbuflen);
					qsndbuflen+=strlen((char*)(qrcv_buf+3));
					if(qsndbuflen>CHAT_BUF-128)qsndbuflen=CHAT_BUF-128;
				    } else {
					i=chatprot;chatprot=-1;
					chatsend(qsnd_buf);
					if(chatlg)chatlog_done();
					chatlg=0;chatprot=i;
					xstrcat((char*)qsnd_buf,"\n * Chat closed\n",CHAT_BUF);
					chatsend(qsnd_buf);
					if(chattimer>1)qlcerase();
					qsndbuflen=0;
					chattimer=1;
				}
				break;
		}
		qsnd_buf[qsndbuflen]=0;
	}
	if(qsndbuflen>0)if(!chatsend(qsnd_buf))qsndbuflen=0;
}

void check_cps()
{
	int cpsdelay=cfgi(CFG_MINCPSDELAY),ncps=rnode->realspeed/1000,r=cfgi(CFG_REALMINCPS);
	if(!(sendf.cps=time(NULL)-sendf.start))sendf.cps=1;
	    else sendf.cps=(sendf.foff-sendf.soff)/sendf.cps;
	if(!(recvf.cps=time(NULL)-recvf.start))recvf.cps=1;
	    else recvf.cps=(recvf.foff-recvf.soff)/recvf.cps;
	if(sendf.start&&cfgi(CFG_MINCPSOUT)>0&&(time(NULL)-sendf.start)>cpsdelay&&sendf.cps<(r?cci:cci*ncps)) {
		write_log("mincpsout=%d reached, aborting session",r?cci:cci*ncps);
		tty_hangedup=1;
	}
	if(recvf.start&&cfgi(CFG_MINCPSIN)>0&&(time(NULL)-recvf.start)>cpsdelay&&recvf.cps<(r?cci:cci*ncps)) {
		write_log("mincpsin=%d reached, aborting session",r?cci:cci*ncps);
		tty_hangedup=1;
	}
	getevt();
}
