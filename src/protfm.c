/******************************************************************
 * File: protfm.c
 * Created at Sun Jan  2 16:00:15 2000 by pk // aaz@ruxy.org.ru
 * common protocols' file management  
 * $Id: protfm.c,v 1.3 2000/07/19 20:47:07 lev Exp $
 ******************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <fnmatch.h>
#include "mailer.h"
#include "qconf.h"
#include "globals.h"

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

int rxopen(char *name, time_t rtime, size_t rsize, FILE **f)
{
	struct stat sb;
	slist_t *i;
	char p[MAX_PATH], *bn=mapname(basename(name), cfgs(CFG_MAPIN));

	recvf.start=time(NULL);strcpy(recvf.fname, bn);
	recvf.mtime=rtime;recvf.ftot=rsize;
	if(recvf.toff+rsize > recvf.ttot) recvf.ttot+=rsize;
	recvf.nf++;if(recvf.nf>recvf.allf) recvf.allf++;

	for(i=cfgsl(CFG_AUTOSKIP);i;i=i->next) 
		if(!fnmatch(i->str, bn, FNM_PATHNAME)) {
			log(weskipstr,recvf.fname);
			return FOP_SKIP;
		}
	
	sprintf(p, "%s/tmp/", cfgs(CFG_INBOUND));
	if(stat(p, &sb)) 
		if(mkdirs(p) && errno!=EEXIST) {
			log("can't make directory %s: %s", p, strerror(errno));
			log(wesusstr,recvf.fname);
			return FOP_SUSPEND;
		}
	sprintf(p, "%s/%s", ccs, bn);
	if(!stat(p, &sb) && sb.st_size==rsize) {
		log(weskipstr,recvf.fname);
		return FOP_SKIP;
	}
	
	sprintf(p, "%s/tmp/%s", ccs, bn);
	if(!stat(p, &sb)) {
		if(sb.st_size<rsize/*   && sb.st_mtime==rtime */) {
			*f=fopen(p, "ab");
			if(!*f) {
				log("can't open file %s for writing!", p);
				log(wesusstr,recvf.fname);
				return FOP_SUSPEND;
			}
			recvf.foff=recvf.soff=ftell(*f);
			return FOP_CONT;
		}
	}

	*f=fopen(p, "wb");
	if(!*f) {
		log("can't open file %s for writing!", p);
		log(wesusstr,recvf.fname);
		return FOP_SUSPEND;
	}
	recvf.foff=recvf.soff=0;
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
		log("recd: %s, %d bytes (from %d), %d cps [%s]",
			recvf.fname, recvf.foff, recvf.soff, cps, ss);
	else
		log("recd: %s, %d bytes, %d cps [%s]",
			recvf.fname, recvf.foff, cps, ss);
	fclose(*f);*f=NULL;
	sprintf(p, "%s/tmp/%s", cfgs(CFG_INBOUND), recvf.fname);
	sprintf(p2, "%s/%s", cfgs(CFG_INBOUND), recvf.fname);
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
						log("can't find situable name for %s: leaving in temporary directory",p);
						p2[0] = '\x00';
					}
				}
			}
			if(p2[0]) {
				if(rename(p, p2)) {
					log("can't rename %s to %s: %d", p, p2, strerror(errno));
				} else {
					utime(p2, &ut);chmod(p2, cfgi(CFG_DEFPERM));
				}
			}
		}
		break;
	}
	return what;
}

FILE *txopen(char *tosend, char *sendas)
{
	FILE *f;
	struct stat sb;
	
	if(stat(tosend, &sb)) {
		log("can't find file %s!", tosend);
		return NULL;
	}
	strcpy(sendf.fname, sendas);
	sendf.ftot=sb.st_size;sendf.mtime=sb.st_mtime;
	sendf.foff=sendf.soff=0;sendf.start=time(NULL);
	if(sendf.toff+sb.st_size > sendf.ttot) sendf.ttot+=sb.st_size;
	sendf.nf++;if(sendf.nf>sendf.allf) sendf.allf++;
	f=fopen(tosend, "rb");
	if(!f) {
		log("can't open file %s for reading!", tosend);
		return NULL;
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
		log("sent: %s, %d bytes (from %d), %d cps [%s]",
			sendf.fname, sendf.foff, sendf.soff, cps, ss);
	else
		log("sent: %s, %d bytes, %d cps [%s]",
			sendf.fname, sendf.foff, cps, ss);
	sendf.foff=0;
	fclose(*f);*f=NULL;
	return what;
}

