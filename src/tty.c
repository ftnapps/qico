/**********************************************************
 * File: tty.c
 * Created at Thu Jul 15 16:14:24 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: tty.c,v 1.5 2000/10/26 19:05:58 lev Exp $
 **********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include "ftn.h"
#include "tty.h"
#include "defs.h"
#include "qconf.h"

char *tty_errs[]={"Ok","tcget/setattr error", "bad speed", "open error",
				"read error", "write error", "timeout", "close error",
				"can't lock port", "can't set/get flags"};


struct termios savetios;
char *tty_port=NULL;
int tty_hangedup=0;

#define BUF_READ

#ifdef BUF_READ
#define MAXBUF       16384
unsigned char buffer[MAXBUF];
int bufpos=0, bufmax=0;
#endif

void tty_sighup(int sig)
{
	tty_hangedup=1;
	log("interrupted!");
	return;
}

int tty_isfree(char *port, char *nodial)
{
	char lckname[MAX_PATH];
	FILE *f;int pid;
	struct stat s;
	
	sprintf(lckname, "%s.%s", nodial, port);
	if(!stat(lckname, &s)) return 0;
	sprintf(lckname,"%s/LCK..%s",cfgs(CFG_LOCKDIR),port);
	if ((f=fopen(lckname,"r")))	{
		fscanf(f,"%d",&pid);         
		fclose(f);		
		if (kill(pid,0) && (errno ==  ESRCH)) {
			unlink(lckname);
			return 1;
		}
		return 0;
	}
	return 1;
}

char *tty_findport(slist_t *ports, char *nodial)
{
	while(ports) {
		if(tty_isfree(baseport(ports->str), nodial)) return ports->str;
		ports=ports->next;
	}
	return NULL;
}

int tty_openport(char *port)
{
	char str[20]="/dev/", *p;
	int speed = 0;
	if(*port!='/') strcat(str, port);else strcpy(str, port);
	p=strchr(str, ':');
	if (p) {
		*p++=0;
		speed = atoi(p);
	}
	if (!speed) speed = DEFAULT_SPEED;
	return tty_open(str,speed);
}
	
void tty_unlock(char *port)
{
	char *p, lckname[MAX_PATH];
	int pid;
	FILE *f;
	
	if ((p=strrchr(port,'/')) == NULL) p=port; else p++;
	sprintf(lckname,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
	if ((f=fopen(lckname,"r")))	{
		fscanf(f,"%d",&pid);         
		fclose(f);
	}
	if(pid==getpid()) unlink(lckname);
}

int tty_lock(char *port)
{
	char lckname[MAX_PATH], *p;
	int rc=-1;
	
	if ((p=strrchr(port,'/')) == NULL) p=port; else p++;
	sprintf(lckname,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
	rc=lockpid(lckname);
	if(rc) return 0;
	return -1;
}


int tty_open(char *port, int speed)
{
	int rc, fd;

	tty_port=strdup(port);
	if(tty_lock(port))
		return ME_CANTLOCK;

	tty_hangedup=0;
	
	fflush(stdin);fflush(stdout);
	setbuf(stdin, NULL);setbuf(stdout, NULL);
	close(0);close(1);
	fd=open(port, O_RDONLY|O_NONBLOCK);
	if(fd!=0) return ME_OPEN;
	fd=open(port, O_WRONLY|O_NONBLOCK);
	if(fd!=1) return ME_OPEN;
	clearerr(stdin);clearerr(stdout);

	rc=tty_setattr(speed);
	if(rc) return rc;

	return tty_block(); 
}

int tty_setattr(int speed)
{
	int rc;
	struct termios tios;
	speed_t tspeed=tty_transpeed(speed);

	signal(SIGHUP, tty_sighup);
	signal(SIGPIPE, tty_sighup);

	rc=tcgetattr(0, &savetios);
	if(rc) {
		rc=ME_ATTRS;
		return rc;
	}

	memcpy(&tios, &savetios, sizeof(tios));
	tios.c_cflag &= ~(CSTOPB | PARENB | PARODD);
	tios.c_cflag |= CS8 | CREAD | HUPCL | CLOCAL;
	tios.c_iflag = 0;
	tios.c_oflag = 0;
	tios.c_lflag = 0;

	if(tspeed) {
		cfsetispeed(&tios,tspeed);
		cfsetospeed(&tios,tspeed);
	}
		
	tios.c_cc[VTIME]=0;
	tios.c_cc[VMIN]=1;

	tcflush(0, TCIFLUSH);
	rc=tcsetattr(0, TCSANOW, &tios);
	if(rc) rc=ME_ATTRS;
	return rc;
}	

speed_t tty_transpeed(int speed)
{
        speed_t tspeed;
        switch (speed)
			{
        case 0:         tspeed=0; break;
#if defined(B50)
        case 50:        tspeed=B50; break;
#endif
#if defined(B75)
        case 75:        tspeed=B75; break;
#endif
#if defined(B110)
        case 110:       tspeed=B110; break;
#endif
#if defined(B134)
        case 134:       tspeed=B134; break;
#endif
#if defined(B150)
        case 150:       tspeed=B150; break;
#endif
#if defined(B200)
        case 200:       tspeed=B200; break;
#endif
#if defined(B300)
        case 300:       tspeed=B300; break;
#endif
#if defined(B600)
        case 600:       tspeed=B600; break;
#endif
#if defined(B1200)
        case 1200:      tspeed=B1200; break;
#endif
#if defined(B1800)
        case 1800:      tspeed=B1800; break;
#endif
#if defined(B2400)
        case 2400:      tspeed=B2400; break;
#endif
#if defined(B4800)
        case 4800:      tspeed=B4800; break;
#endif
#if defined(B7200)
        case 7200:      tspeed=B7200; break;
#endif
#if defined(B9600)
        case 9600:      tspeed=B9600; break;
#endif
#if defined(B12000)
        case 12000:     tspeed=B12000; break;
#endif
#if defined(B14400)
        case 14400:     tspeed=B14400; break;
#endif
#if defined(B19200)
        case 19200:     tspeed=B19200; break;
#elif defined(EXTA)
        case 19200:     tspeed=EXTA; break;
#endif
#if defined(B38400)
        case 38400:     tspeed=B38400; break;
#elif defined(EXTB)
        case 38400:     tspeed=EXTB; break;
#endif
#if defined(B57600)
        case 57600:     tspeed=B57600; break;
#endif
#if defined(B115200)
        case 115200:    tspeed=B115200; break;
#endif
        default:        tspeed=0; break;
        }
        return tspeed;
}

int tty_local()
{
	struct termios tios;
	int rc;

	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
		
	if(!isatty(0)) return ME_NOTATT;

	rc=tcgetattr(0, &tios);
	if(rc) return ME_ATTRS;
	tios.c_cflag|=CLOCAL;
	tios.c_cflag|=CRTSCTS;

	rc=tcsetattr(0, TCSANOW, &tios);
	return rc;
}

int tty_nolocal()
{
	struct termios tios;
	int rc;
	
	signal(SIGHUP, tty_sighup);
	signal(SIGPIPE, tty_sighup);

	if(!isatty(0)) return ME_NOTATT;
	
	rc=tcgetattr(0, &tios);
	if(rc) return ME_ATTRS;
	tios.c_cflag&=~CLOCAL;
	tios.c_cflag|=CRTSCTS;

	rc=tcsetattr(0, TCSANOW, &tios);
	if(rc) rc=ME_ATTRS;
	return rc;
}

int tty_cooked()
{
	int rc;
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	if(!isatty(0)) return ME_NOTATT;
	rc=tcsetattr(0, TCSAFLUSH, &savetios);
	if(rc) rc=ME_ATTRS;
	return rc;
}

int tty_close()
{
	if(!tty_port) return ME_CLOSE;
	fflush(stdin);fflush(stdout);	
	tty_cooked();
	fclose(stdin);fclose(stdout);
	tty_unlock(tty_port);
	sfree(tty_port);
	return ME_OK;
}

int tty_unblock()
{
	int flags, rc=0;
	
	flags=fcntl(0, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags|=O_NONBLOCK;
	rc|=fcntl(0, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	
	flags=fcntl(1, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags|=O_NONBLOCK;
	rc|=fcntl(1, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	return ME_OK;
}

int tty_block()
{
	int flags, rc=0;
	
	flags=fcntl(0, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags&=~O_NONBLOCK;
	rc|=fcntl(0, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	
	flags=fcntl(1, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags&=~O_NONBLOCK;
	rc|=fcntl(1, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	return ME_OK;
}

int tty_put(char *buf, int size)
{
	int rc;

	if(tty_hangedup) return RCDO;
	rc=write(1, buf, size);
	if(rc!=size) {
		if(tty_hangedup || errno==EPIPE) 
			return RCDO;
		else
			return ERROR;
	}		
	return OK;
}
		
int tty_get(char *buf, int size, int timeout)
{
	fd_set rfds, efds, wfds;
	struct timeval tv;
	int rc;
	
	if(tty_hangedup) return RCDO;
	FD_ZERO(&rfds);FD_ZERO(&wfds);FD_ZERO(&efds);
	FD_SET(0,&rfds);FD_SET(0,&efds);
	tv.tv_sec=timeout;
	tv.tv_usec=0;

	rc=select(1, &rfds, &wfds, &efds, &tv);
	if(rc<0) {
		if(tty_hangedup) 
			return RCDO;
		else
			return ERROR;
	}
	if(rc==0) return TIMEOUT;
	if(FD_ISSET(0, &efds)) return ERROR;
	
	rc=read(0, buf, size);
	if(rc<1) {
		if(tty_hangedup || errno==EPIPE)
			return RCDO;
		else
			return (errno!=EAGAIN && errno!=EINTR)?ERROR:TIMEOUT;
	}
	return rc;
}

int tty_putc(char ch)
{
	return tty_put(&ch, 1);
}

#ifdef BUF_READ
int tty_getc(int timeout)
{
 	int rc;
 	if(bufpos<bufmax) return buffer[bufpos++];
 	rc=tty_get(buffer, MAXBUF, timeout);
 	if(rc<0) return rc;
 	bufpos=0;bufmax=rc;
 	return buffer[bufpos++];
}
#else
int tty_getc(int timeout)
{
	int ch, rc;
	rc=tty_get((char *)&ch, 1, timeout);
	if(rc<0) return rc;
	return ch&255;
}
#endif

int tty_hasdata(int sec, int usec)
{
	fd_set rfds, efds;
	struct timeval tv;
	int rc;

	if(tty_hangedup) return RCDO;
#ifdef BUF_READ	
  	if(bufpos<bufmax) return OK; 
#endif
	FD_ZERO(&rfds);
	FD_ZERO(&efds);
	FD_SET(0,&rfds);
	FD_SET(0,&efds);
	tv.tv_sec=sec;
	tv.tv_usec=usec;

	rc=select(1, &rfds, NULL, &efds, &tv);
	if(rc<0) {
		if(tty_hangedup) 
			return RCDO;
		else
			return ERROR;
	}
	if(rc==0) return TIMEOUT;
	if(rc>0) if(FD_ISSET(0, &efds) || !FD_ISSET(0, &rfds)) return ERROR;
	return OK;
}

void tty_purge() {
	tcflush(0, TCIFLUSH);
}

void tty_purgeout() {
	tcflush(1, TCOFLUSH);
}

char canistr[] = {
 24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
};

int tty_gets(char *what, int n, int timeout)
{
	int to=timeout, p=0;int ch=0;
	time_t t1;

	while(to>0 && p<n-1 && ch!='\r' && ch!='\n') {
		t1=t_start();
		ch=tty_getc(to);
		if(NOTTO(ch)) return ch;
		to-=t_time(t1);
		if(ch>=0) {what[p]=ch;p++;}
	}
	what[--p]=0;
	return (ch=='\r' || ch=='\n')?OK:TIMEOUT;
}

int tty_expect(char *what, int timeout)
{
	int to=timeout, got=0, p=0;int ch;
	time_t t1;
	
	while(!got && to>0) {
		t1=t_start();
		ch=tty_getc(to);
/* 		log("getc got '%c' %03d", C0(ch), ch); */
		if(ch<0) return ch;
		to-=t_time(t1);
		if(ch==what[p]) p++;else p=0;
		if(!what[p]) got=1;
	}
	return got?OK:TIMEOUT;
}


char *baseport(char *p)
{
	static char pn[20];
	char *q;
	strcpy(pn, basename(p));
	if((q=strrchr(pn,':'))) *q=0;
	return pn;
}


int modem_sendstr(char *cmd)
{
	int rc=1;
	if(!cmd) return 1;
	while(*cmd && rc>0) {
		switch(*cmd) {
		case '|': rc=write(1, "\r", 1);usleep(300000);break;
		case '~': sleep(1);rc=1;break;
		case '\'': usleep(200000L);rc=1;break;
		default: rc=write(1, cmd, 1);
		}
		cmd++;
	}
	return rc;
}
	
int modem_chat(char *cmd, slist_t *oks, slist_t *ers, slist_t *bys,
			   char *ringing, int maxr, int timeout, char *rest)
{
	char buf[MAX_STRING];int rc, nrng=0;
	slist_t *cs;time_t t1=t_set(timeout);

	rc=modem_sendstr(cmd);
	if(rc!=1) {
		if(rest) strcpy(rest, "FAILURE");
		return MC_FAIL;
	}
	if(!oks && !ers && !bys) return MC_OK;
	rc=OK;
	while(ISTO(rc) && !t_exp(t1) && (!maxr || nrng<maxr)) {
		rc=tty_gets(buf, MAX_STRING-1, t_rest(t1));
		if(!*buf) continue;
/*      		log("gets got %d '%s'", rc, buf);  */
		if(rc!=OK) {
			if(rest) strcpy(rest, "FAILURE");
			return MC_FAIL;
		}
		if(ringing && !strncmp(buf, ringing, strlen(ringing))) {
			nrng++;
			continue;
		}
		for(cs=oks;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(rest) strcpy(rest, buf);
				return MC_OK;
			}
		for(cs=ers;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(rest) strcpy(rest, buf);
				return MC_ERROR;
			}
		for(cs=bys;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(rest) strcpy(rest, buf);
				return MC_BUSY;
			}
	}
	
	if(rest) {
		if(nrng && maxr && nrng>=maxr) sprintf(rest, "%d RINGINGs", nrng);
		else strcpy(rest, "TIMEOUT");
	}
	return MC_FAIL;
}

