/******************************************************************
 * File: protfm.c
 * Created at Sun Jan  2 16:00:15 2000 by pk // aaz@ruxy.org.ru
 * common protocols' file management  
 * $Id: protfm.c,v 1.21.4.1 2003/01/24 08:59:22 cyrilm Exp $
 ******************************************************************/
#include "headers.h"
#include <utime.h>
#include <fnmatch.h>

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

char weskipstr[]="recd: %s, 0 bytes, 0 cps [skipped]";
char wesusstr[]="recd: %s, 0 bytes, 0 cps [suspended]";

int sifname(char *s)
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
	int prevcps = (recvf.start&&(time(NULL)-recvf.start>2))?recvf.cps:effbaud/10;

	xstrcpy(bn, basename(name), MAX_PATH);
	mapname(bn, cfgs(CFG_MAPIN), MAX_PATH);

 	recvf.start=time(NULL);
	xfree(recvf.fname);
 	recvf.fname=xstrdup(bn);
	recvf.mtime=rtime;recvf.ftot=rsize;
	if(recvf.toff+rsize > recvf.ttot) recvf.ttot+=rsize;
	recvf.nf++;if(recvf.nf>recvf.allf) recvf.allf++;

	for(i=cfgsl(CFG_AUTOSKIP);i;i=i->next) 
		if(!fnmatch(i->str, bn, FNM_PATHNAME)) {
			write_log(weskipstr,recvf.fname);
			return FOP_SKIP;
		}
	
	snprintf(p, MAX_PATH, "%s/tmp/", cfgs(CFG_INBOUND));
	if(stat(p, &sb)) 
		if(mkdirs(p) && errno!=EEXIST) {
			write_log("can't make directory %s: %s", p, strerror(errno));
			write_log(wesusstr,recvf.fname);
			return FOP_SUSPEND;
		}
	snprintf(p, MAX_PATH, "%s/%s", ccs, bn);
	if(!stat(p, &sb) && sb.st_size==rsize) {
		write_log(weskipstr,recvf.fname);
		return FOP_SKIP;
	}
	
	snprintf(p, MAX_PATH, "%s/tmp/%s", ccs, bn);
	if(!stat(p, &sb)) {
		if(sb.st_size<rsize && sb.st_mtime==rtime) {
			*f=fopen(p, "ab");
			if(!*f) {
				write_log("can't open file %s for writing!", p);
				write_log(wesusstr,recvf.fname);
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
		write_log("can't open file %s for writing!", p);
		write_log(wesusstr,recvf.fname);
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
	int cps=time(NULL)-recvf.start, rc;
	char *ss, p[MAX_PATH], p2[MAX_PATH];
	struct utimbuf ut;struct stat sb;

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
		utime(p, &ut);
		break;
	case FOP_OK:
		rc=receive_callback?receive_callback(p):0;
		if(rc) lunlink(p);
		else {
			ss=p2+strlen(p2)-1;
			while(!stat(p2, &sb) && p2[0]) {
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
				if(rename(p, p2)) {
					write_log("can't rename %s to %s: %d", p, p2, strerror(errno));
				} else {
					utime(p2, &ut);chmod(p2, cfgi(CFG_DEFPERM));
				}
			}
		}
		break;
	}
	recvf.start=0;
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
	xfree(sendf.fname);
 	sendf.fname=xstrdup(sendas);
	sendf.ftot=sb.st_size;sendf.mtime=sb.st_mtime;
	sendf.foff=sendf.soff=0;sendf.start=time(NULL);
	if(sendf.toff+sb.st_size > sendf.ttot) sendf.ttot+=sb.st_size;
	sendf.nf++;if(sendf.nf>sendf.allf) sendf.allf++;
	f=fopen(tosend, "rb");
	if(!f) {
		write_log("can't open file %s for reading!", tosend);
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
	if(sendf.soff)
		write_log("sent: %s, %d bytes (from %d), %d cps [%s]",
			sendf.fname, sendf.foff, sendf.soff, cps, ss);
	else
		write_log("sent: %s, %d bytes, %d cps [%s]",
			sendf.fname, sendf.foff, cps, ss);
	sendf.foff=0;
	sendf.start=0;
	fclose(*f);*f=NULL;
	return what;
}

void check_cps()
{
	int cpsdelay=cfgi(CFG_MINCPSDELAY);
	
	sendf.cps=time(NULL)-sendf.start;
	if(!sendf.cps) sendf.cps=1;
	sendf.cps=(sendf.foff-sendf.soff)/sendf.cps;

	recvf.cps=time(NULL)-recvf.start;
	if(!recvf.cps) recvf.cps=1;
	recvf.cps=(recvf.foff-recvf.soff)/recvf.cps;

	if(sendf.start && cfgi(CFG_MINCPSOUT)>0 &&
	   (time(NULL)-sendf.start)>cpsdelay &&
	   sendf.cps<cci) {
		write_log("mincpsout=%d reached, aborting session", cci);
		tty_hangedup=1;
	}
	if(recvf.start && cfgi(CFG_MINCPSIN)>0 &&
	   (time(NULL)-recvf.start)>cpsdelay &&
	   recvf.cps<cci) {
		write_log("mincpsin=%d reached, aborting session", cci);
		tty_hangedup=1;
	}
}
