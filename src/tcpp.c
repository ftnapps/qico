/******************************************************************
 * File: tcpp.c
 * Created at Fri Dec 17 00:14:49 1999 by pk // aaz@ruxy.org.ru
 * Base version of this file was taken from Eugene Crosser's ifcico,
 * but... it was a very bad piece of code ;)
 * $Id: tcpp.c,v 1.1.1.1 2000/07/18 12:37:21 lev Exp $
 ******************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "mailer.h"
#include "qconf.h"

#define TCP_CMD	0x87
#define TCP_DATA 0xE1

int tcppsend(int zap)
{
	flist_t *l;
	int rc;
	unsigned long total=totalf+totalm;

	log("tcp send");sline("Init tcpsend...");
	rc=getsync();
	if(rc) {
		log("can't synchronize tcp");
		return 1;
	}
	sendf.cps=1;sendf.allf=totaln;sendf.ttot=totalf;
 	if(!rc) for(l=fl;l;l=l->next) {
		if(l->sendas) {
			rc=sendfile(l->tosend, l->sendas);
			if(rc<0) break;
			if(!rc) {
				if(l->type==IS_REQ) was_req=1;
				flexecute(l);
			}
		} else flexecute(l);
	}

	sline("Done tcpsend...");
	if(!rc) rc=finsend();
	qpreset(1);
	if(rc<0) return 1;
	return 0;
}


int tcprecv()
{
	int rc,bufl;
	long filesize,filetime;
	char *filename,*p;	

	log("tcpp receive");
	if(getsync()){
		logerr("can't synchronize tcpp");
		return 1;
	}
	while(1) {
		if((rc=tcp_rblk(rxbuf,&bufl))==0){
			if(strncmp(rxbuf,"SN",2)==0){
				rc=tcp_sblk("RN",2,TCP_CMD);
				return rc;
			}
			else if(strncmp(rxbuf,"S ",2)==0){
				p=strchr(rxbuf+2,' ');
				if (p!=NULL) {
					*p=0;
				} else return 1;
				filename=xstrcpy(rxbuf+2);
				p++;
				filesize=strtol(p,&p,10);
				filetime=strtol(++p,(char **)NULL,10);			
			}
			else return rc==0?1:rc;
			
			if(strlen(filename) && filesize && filetime)
				rc=receivefile(filename,filetime,filesize);
			if (txfd)
				{ 
					if (closeit(0)) {
						logerr("Error closing file");
					}
					(void)tcp_sblk("FERROR",6,TCP_CMD);
				}
		}
	}

	loginf("TCP receive rc=%d",rc);
	return abs(rc);
}

int sendfile(char *tosend,char *sendas)
{
	int rc=0;
	int bufl;
	long offset;
	
	txfd=txopen(tosend, sendas);
	if(!txfd) return 1;
	
	sprintf(txbuf,"S %s %ld %ld", sendas, sendf.ftot, sendf.mtime);
	bufl=strlen(txbuf);
	rc=tcp_sblk(txbuf,bufl,TCP_CMD);
	rc=tcp_rblk(rxbuf,&bufl);
	if(!strncmp(rxbuf,"RS",2)) {
        /*	file considered normally sent */
		txclose(&txfd, FOP_SKIP);
		return 0;
	}
	else if(!strncmp(rxbuf,"RN",2)) {
        /*	remote refusing receiving, aborting */
		txclose(&txfd, FOP_ERROR);
		return 2;
	}
	else if(!strncmp(rxbuf,"ROK",3)){
		if(bufl>3 && rxbuf[3]==' '){
			offset=strtol(rxbuf+4,(char **)NULL,10);
			if(fseek(txfd,offset,SEEK_SET)){
				log("can't seek in file %s",ln);
				return 1;
			}
		} else offset=0;
	} else return rc;

	while((bufl=fread(&txbuf,1,1024,in)) && !rc) 
		rc=tcp_sblk(txbuf,bufl,TCP_DATA)>0;
	if(!rc) {
		strcpy(txbuf,"EOF");
		rc=tcp_sblk(txbuf,3,TCP_CMD);
		rc=tcp_rblk(rxbuf,&bufl);
	}
	
	if(!rc && !strncmp(rxbuf,"FOK",3)) {
		txclose(&txfd, FOP_OK);
		return 0;
	} else if(strncmp(rxbuf,"FERROR",6)==0){
		txclose(&txfd, FOP_SUSPEND);
		return !rc;
	}
	return !rc;
}

int resync(off_t off)
{
	sprintf(txbuf,"ROK %ld", (long) off);
	return 0;
}

int closeit(int success)
{
	int rc;

	rxclose(
	return rc;
}

int finsend()
{
	int rc,bufl;
	rc=tcp_sblk("SN",2,TCP_CMD);
	if(rc) return rc;
	rc=tcp_rblk(rxbuf,&bufl);
	if(strncmp(rxbuf,"RN",2)==0)
		return rc;
	else
		return 1;
}

int receivefile(char *fn, time_t ft, off_t fs)
{
	int rc,bufl;

	loginf("TCP receive \"%s\" (%lu bytes)",fn,fs);
	strcpy(txbuf,"ROK");
	txfd=openfile(fn,ft,fs,&rxpos,resync);
	if (!txfd) return 1;
	(void)time(&startime);
	txpos=rxpos;

	if (fs == rxpos) {
		loginf("Skipping %s", fn);
		closeit(1);
		rc=tcp_sblk("RS",2,TCP_CMD);
		return rc;
	}
	bufl=strlen(txbuf);
	rc=tcp_sblk(txbuf,bufl,TCP_CMD);

	while((rc=tcp_rblk(rxbuf,&bufl))==0){
		if(rx_type==TCP_CMD)
			break;
		if(fwrite(rxbuf,1,bufl,txfd)!=bufl)
			break;
		rxpos+=bufl;
	}
	if(rc) return rc;
	if(rx_type==TCP_CMD && bufl==3 && strncmp(rxbuf,"EOF",3)==0){
		if(ftell(txfd)==fs){
			closeit(1);
			rc=tcp_sblk("FOK",3,TCP_CMD);
			return rc;
		}
		else	return 1;
	}
	else	return 1;		
}

int tcp_sblk(char *buf,int len,int typ)
{
	PUTCHAR(0xc6);PUTCHAR(typ);
	PUTCHAR((len>>8)&0x0ff);
	PUTCHAR(len&0x0ff);
	PUTBLK(buf,len);
	PUTCHAR(0x6c);
	return HASDATA(0);
}

int tcp_rblk(char *buf,int *len)
{
	int c, typ;

	*len=0;c=GETCHAR(120);
	if(c!=0xc6)	return c;
	c=GETCHAR(120);typ=c;
	if(typ!=TCP_CMD && typ!=TCP_DATA) return c;
	c=GETCHAR(120);*len=c<<8;
	c=GETCHAR(120);*len+=c;
	GETBLK(buf,*len,120);
	c=GETCHAR(120);
	if(c!=0x6c)	return c;
	return typ;
}

int getsync()
{
	int c, st;
	PUTCHAR(0xAA);
	PUTCHAR(0x55);

	st=0;
	while(1) {
		c=GETCHAR(120);
		if(c<0) return c;
		switch(st) {
		case 0:
			if(c==0xAA) st=1;
			break;
		case 1:
			if(c==0x55) return OK;
			st=0;
			break;
		}
	}
}
