/**********************************************************
 * work with tty's
 * $Id: tty.c,v 1.1.1.1 2003/07/12 21:27:18 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "defs.h"
#include "tty.h"

#undef DEBUG_SLEEP
//#define DEBUG_SLEEP

char *tty_errs[]={"Ok","tcget/setattr error", "bad speed", "open error",
			"read error", "write error", "timeout", "close error",
				"can't lock port", "can't set/get flags"};
struct termios savetios;
char *tty_port=NULL;
int tty_hangedup=0,calling=0;

#define IN_MAXBUF       16384
unsigned char in_buffer[IN_MAXBUF];
int in_bufpos=0, in_bufmax=0;

#define OUT_MAXBUF       16384
unsigned char out_buffer[OUT_MAXBUF];
int out_bufpos=0;

char ipcbuf[MSG_BUFFER];

void tty_sighup(int sig)
{
	char *sigs[]={"","HUP","INT","QUIT","ILL","TRAP","IOT","BUS","FPE",
			  "KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM"};
	tty_hangedup=1;
	DEBUG(('M',3,"tty: got SIG%s signal",sigs[sig]));
	return;
}

int selectmy(int n,fd_set *rfs,fd_set *wfs,fd_set *efs,struct timeval *to)
{
	int sec=to->tv_sec,rc;
	do {
		if(calling)if(qrecvpkt(ipcbuf)&&ipcbuf[8]==QR_HANGUP) {
			tty_hangedup=1;
			return -1;
		}
		to->tv_sec=sec?1:0;
		rc=select(n,rfs,wfs,efs,to);
	} while(sec--&&rc<0);
	to->tv_sec=sec;
#ifdef DEBUG_SLEEP
	if(is_ip)usleep(30);
#endif
	return rc;
}

int tty_isfree(char *port, char *nodial)
{
	char lckname[MAX_PATH];
	FILE *f;int pid;
	struct stat s;
	
	snprintf(lckname, MAX_PATH, "%s.%s", nodial, port);
	if(!stat(lckname, &s)) return 0;
	snprintf(lckname, MAX_PATH, "%s/LCK..%s",cfgs(CFG_LOCKDIR),port);
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
	if(*port!='/') xstrcat(str, port, sizeof(str));else xstrcpy(str, port, sizeof(str));
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
	snprintf(lckname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
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
	snprintf(lckname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
	rc=lockpid(lckname);
	if(rc) return 0;
	return -1;
}


int tty_open(char *port, int speed)
{
	int rc, fd;

	tty_port=xstrdup(port);
	if(tty_lock(port))
		return ME_CANTLOCK;

	tty_hangedup=0;
	
	fflush(stdin);fflush(stdout);
	setbuf(stdin, NULL);setbuf(stdout, NULL);
	close(STDIN_FILENO);close(STDOUT_FILENO);
	fd=open(port, O_RDONLY|O_NONBLOCK);
	if(dup2(fd,STDIN_FILENO)!=STDIN_FILENO) return ME_OPEN;
	if(fd!=STDIN_FILENO) close(fd);
	fd=open(port, O_WRONLY|O_NONBLOCK);
	if(dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO) return ME_OPEN;
	if(fd!=STDOUT_FILENO) close(fd);
	clearerr(stdin);clearerr(stdout);

	rc=tty_setattr(speed);
	if(rc) return rc;

	return tty_block(); 
}

int tty_setattr(int speed)
{
	pid_t me = getpid();
	int rc;
	struct termios tios;
	speed_t tspeed=tty_transpeed(speed);

	signal(SIGHUP, tty_sighup);
	signal(SIGPIPE, tty_sighup);

	if(getsid(me) == me) {
#ifdef HAVE_TIOCSCTTY
		/* We are on BSD system, set this terminal as out control terminal */
		ioctl(STDIN_FILENO,TIOCSCTTY);
#endif
		tcsetpgrp(STDIN_FILENO,me);
	}

	rc=tcgetattr(STDIN_FILENO, &savetios);
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
		/* Speed is zero on answer, and we don't want to flush incoming EMSI_DAT */
		tcflush(STDIN_FILENO, TCIFLUSH);
	}
		
	tios.c_cc[VTIME]=0;
	tios.c_cc[VMIN]=1;

	rc=tcsetattr(STDIN_FILENO, TCSANOW, &tios);
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
		
	if(!isatty(STDIN_FILENO)) return ME_NOTATT;

	rc=tcgetattr(STDIN_FILENO, &tios);
	if(rc) {
		DEBUG(('M',3,"tty_local: tcgetattr failed, errno=%d",errno));
		return ME_ATTRS;
	}
	tios.c_cflag|=CLOCAL;
	tios.c_cflag|=CRTSCTS;

	rc=tcsetattr(STDIN_FILENO, TCSANOW, &tios);
	if (rc) {
		DEBUG(('M',3,"tty_local: tcsetattr failed, errno=%d",errno));
		rc=ME_ATTRS;
	} else DEBUG(('M',4,"tty_local: completed"));
	tty_hangedup=0;
	return rc;
}

int tty_nolocal()
{
	struct termios tios;
	int rc;
	
	signal(SIGHUP, tty_sighup);
	signal(SIGPIPE, tty_sighup);

	if(!isatty(STDIN_FILENO)) return ME_NOTATT;
	
	rc=tcgetattr(STDIN_FILENO, &tios);
	if(rc) {
		DEBUG(('M',3,"tty_nolocal: tcgetattr failed, errno=%d",errno));
		return ME_ATTRS;
	}
	tios.c_cflag&=~CLOCAL;
	tios.c_cflag|=CRTSCTS;

	rc=tcsetattr(STDIN_FILENO, TCSANOW, &tios);
	if(rc) {
		DEBUG(('M',3,"tty_nolocal: tcsetattr failed, errno=%d",errno));
		rc=ME_ATTRS;
	} else DEBUG(('M',4,"tty_nolocal: completed"));
	return rc;
}

int tty_cooked()
{
	int rc;
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	if(!isatty(STDIN_FILENO)) return ME_NOTATT;
	rc=tcsetattr(STDIN_FILENO, TCSAFLUSH, &savetios);
	if(rc) {
		DEBUG(('M',3,"tty_cooked: tcsetattr failed, errno=%d",errno));
		rc=ME_ATTRS;
	} else DEBUG(('M',4,"tty_cooked: completed"));
	return rc;
}

int tty_setdtr(int dtr)
{
	int status,rc;
	rc=ioctl(STDIN_FILENO, TIOCMGET, &status);
	if(rc<0) {
		DEBUG(('M',3,"tty_setdtr: TIOCMGET failed, dtr=%d, errno=%d",dtr,errno));
		return 0;
	}
	if(dtr) status |= TIOCM_DTR;
	    else status &= ~TIOCM_DTR;
	rc=ioctl(STDIN_FILENO, TIOCMSET, &status);
	if(rc<0) {
		DEBUG(('M',3,"tty_setdtr: TIOCMSET failed, dtr=%d, errno=%d",dtr,errno));
		return 0;
	}
	DEBUG(('M',4,"tty_setdtr: completed"));
	return 1;
}

int tty_close()
{
	if(!tty_port) return ME_CLOSE;
	fflush(stdin);fflush(stdout);	
	tty_cooked();
	fclose(stdin);fclose(stdout);
	tty_unlock(tty_port);
	xfree(tty_port);
	return ME_OK;
}

int tty_unblock()
{
	int flags, rc=0;
	
	flags=fcntl(STDIN_FILENO, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags|=O_NONBLOCK;
	rc|=fcntl(STDIN_FILENO, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	
	flags=fcntl(STDOUT_FILENO, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags|=O_NONBLOCK;
	rc|=fcntl(STDOUT_FILENO, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	return ME_OK;
}

int tty_block()
{
	int flags, rc=0;
	
	flags=fcntl(STDIN_FILENO, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags&=~O_NONBLOCK;
	rc|=fcntl(STDIN_FILENO, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	
	flags=fcntl(STDOUT_FILENO, F_GETFL, 0L);
	if(flags<0) { rc=1;flags=0; }
	flags&=~O_NONBLOCK;
	rc|=fcntl(STDOUT_FILENO, F_SETFL, flags);
	if(rc) return ME_FLAGS;
	return ME_OK;
}

int tty_put(byte *buf, int size)
{
	int rc;
	if(tty_hangedup) return RCDO;
	rc=write(STDOUT_FILENO, buf, size);
	if(rc!=size) {
		if(tty_hangedup || errno==EPIPE) return RCDO;
		else return ERROR;
	}		
#ifdef DEBUG_SLEEP
	if(is_ip)usleep(300);
#endif
	return OK;
}
		
int tty_get(byte *buf, int size, int *timeout)
{
	fd_set rfds, efds, wfds;
	struct timeval tv;
	int rc;
	time_t t;
	if(tty_hangedup) return RCDO;
	FD_ZERO(&rfds);FD_ZERO(&wfds);FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=*timeout;
	tv.tv_usec=0;
	
	t=time(NULL);
	rc=selectmy(1, &rfds, &wfds, &efds, &tv);
	if(rc<0) {
		if(tty_hangedup) return RCDO;
		else return ERROR;
	}
	*timeout-=(time(NULL)-t);
	if(rc==0) return TIMEOUT;
	if(FD_ISSET(STDIN_FILENO, &efds)) return ERROR;
	
	rc=read(STDIN_FILENO, buf, size);
	if(rc<1) {
		if(tty_hangedup || errno==EPIPE) return RCDO;
		else return (errno!=EAGAIN && errno!=EINTR)?ERROR:TIMEOUT;
	}
#ifdef DEBUG_SLEEP
	if(is_ip)usleep(300);
#endif
	return rc;
}


int tty_bufc(char ch)
{
	int rc = OK;
	if(OUT_MAXBUF==out_bufpos) {
		rc=tty_put(out_buffer,OUT_MAXBUF);
		out_bufpos=0;
	}
	out_buffer[out_bufpos++]=ch;
	return rc;
}


int tty_bufflush()
{	int rc=tty_put(out_buffer,out_bufpos);
	out_bufpos=0;
	return rc;
}

void tty_bufclear() { out_bufpos = 0; }


int tty_putc(char ch) { return tty_put((byte *)&ch, 1); }


int tty_getc(int timeout)
{
	int rc;
	if(in_bufpos<in_bufmax) return in_buffer[in_bufpos++];
	if((rc=tty_get(in_buffer, IN_MAXBUF, &timeout))<0) return rc;
	in_bufpos=0;in_bufmax=rc;
 	return in_buffer[in_bufpos++];
}

int tty_getc_timed(int *timeout)
{
	int rc;
	if(in_bufpos<in_bufmax) return in_buffer[in_bufpos++];
	if((rc=tty_get(in_buffer, IN_MAXBUF, timeout))<0) return rc;
	in_bufpos=0;in_bufmax=rc;
 	return in_buffer[in_bufpos++];
}


int tty_hasdata(int sec, int usec)
{
	fd_set rfds, efds;
	struct timeval tv;
	int rc;

	if(tty_hangedup) return RCDO;
	if(in_bufpos<in_bufmax) return OK; 
	FD_ZERO(&rfds);
	FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);
	FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=sec;
	tv.tv_usec=usec;

	rc=selectmy(1, &rfds, NULL, &efds, &tv);
	if(rc<0) {
		if(tty_hangedup) return RCDO;
		else return ERROR;
	}
	if(rc==0) return TIMEOUT;
	if(rc>0) if(FD_ISSET(STDIN_FILENO, &efds) || !FD_ISSET(STDIN_FILENO, &rfds)) return ERROR;
	return OK;
}


int tty_hasdata_timed(int *timeout)
{
	fd_set rfds, efds;
	struct timeval tv;
	int rc;
	time_t t;

	if(tty_hangedup) return RCDO;
	if(in_bufpos<in_bufmax) return OK; 
	FD_ZERO(&rfds);
	FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);
	FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=*timeout;
	tv.tv_usec=0;

	t=time(NULL);
	rc=selectmy(1, &rfds, NULL, &efds, &tv);
	if(rc<0) {
		if(tty_hangedup) return RCDO;
		else return ERROR;
	}
	*timeout-=(time(NULL)-t);
	if(rc==0) return TIMEOUT;
	if(rc>0) if(FD_ISSET(STDIN_FILENO, &efds) || !FD_ISSET(STDIN_FILENO, &rfds)) return ERROR;
	return OK;
}


void tty_purge() {
	in_bufmax = in_bufpos = 0;
	tcflush(STDIN_FILENO, TCIFLUSH);
}

void tty_purgeout() {
	out_bufpos = 0;
	tcflush(STDOUT_FILENO, TCOFLUSH);
}

char canistr[] = {
 24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
};

int tty_gets(char *what, int n, int timeout)
{
	int to=timeout,p=0,ch=0;
	time_t t1;
	*what=0;
	while(to>0 && p<n-1 && ch!='\r' && ch!='\n') {
		t1=t_start();
		ch=tty_getc(to);
		if(NOTTO(ch)) return ch;
		to-=t_time(t1);
		if(ch>=0) {what[p]=ch;p++;}
	}
	what[p>0?--p:0]=0;
	if(p>0)DEBUG(('M',1,"<< %s",what));
	if(ch=='\r' || ch=='\n') {
		DEBUG(('M',5,"tty_gets: completed"));
		return OK;
	} else if(to<=0) {
		DEBUG(('M',3,"tty_gets: timed out"));
	} else DEBUG(('M',3,"tty_gets: line too long, consider increasing MAX_STRING"));
	return TIMEOUT;
}

int tty_expect(char *what, int timeout)
{
	int to=timeout, got=0, p=0;int ch;
	time_t t1;

	calling=0;
	while(!got && to>0) {
		t1=t_start();
		ch=tty_getc(to);
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
	xstrcpy(pn, basename(p), sizeof(pn));
	if((q=strrchr(pn,':'))) *q=0;
	return pn;
}


int modem_sendstr(char *cmd)
{
	int rc=1;
	if(!cmd) return 1;
	DEBUG(('M',1,">> %s",cmd));
	while(*cmd && rc>0) {
		switch(*cmd) {
		case '|': rc=write(STDOUT_FILENO, "\r", 1);usleep(300000L);break;
		case '~': sleep(1);rc=1;break;
		case '\'': usleep(200000L);rc=1;break;
		case '^': rc=tty_setdtr(1); break;
		case 'v': rc=tty_setdtr(0); break;
		default: rc=write(STDOUT_FILENO, cmd, 1);
		DEBUG(('M',4,">>> %c",*cmd));
		}
		cmd++;
	}
	if(rc>0) DEBUG(('M',4,"modem_sendstr: sent"));
	    else DEBUG(('M',3,"modem_sendstr: error, rc=%d, errno=%d",rc,errno));
	return rc;
}
	
int modem_chat(char *cmd, slist_t *oks, slist_t *nds, slist_t *ers, slist_t *bys,
			   char *ringing, int maxr, int timeout, char *rest, size_t restlen)
{
	char buf[MAX_STRING];
	int rc,nrng=0;
	slist_t *cs;
	time_t t1;
	calling=1;
	DEBUG(('M',4,"modem_chat: cmd=\"%s\" timeout=%d",cmd,timeout));
	rc=modem_sendstr(cmd);
	if(rc!=1) {
		if(rest) xstrcpy(rest, "FAILURE", restlen);
		DEBUG(('M',3,"modem_chat: modem_sendstr failed, rc=%d",rc));
		return MC_FAIL;
	}
	if(!oks && !ers && !bys) return MC_OK;
	rc=OK;
	t1=t_set(timeout);
	while(ISTO(rc) && !t_exp(t1) && (!maxr || nrng<maxr)) {
		if(qrecvpkt(ipcbuf)&&ipcbuf[8]==QR_HANGUP)tty_hangedup=1;
		rc=tty_gets(buf, MAX_STRING-1, t_rest(t1));
		if(rc==RCDO) {
			if(rest)xstrcpy(rest,"HANGUP",restlen);
	    		return MC_BUSY;
		}
		if(rc!=OK) {
			if(rest) xstrcpy(rest, "FAILURE", restlen);
			DEBUG(('M',3,"modem_chat: tty_gets failed, rc=%d",rc));
			return MC_FAIL;
		}
		if(!*buf)continue;
		for(cs=oks;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_OK;
			}
		for(cs=ers;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_ERROR;
			}
		if(ringing&&!strncmp(buf,ringing,strlen(ringing))) {
			if(!nrng&&strlen(ringing)==4) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_RING;
			}
			nrng++;
			continue;
		}
		for(cs=nds;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_NODIAL;
			}
		for(cs=bys;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_BUSY;
			}
	}
	if(rest) {
		if(nrng && maxr && nrng>=maxr) snprintf(rest, restlen, "%d RINGINGs", nrng);
		    else if(ISTO(rc)) xstrcpy(rest, "TIMEOUT", restlen);
			else xstrcpy(rest, "FAILURE", restlen);
	}
	return MC_FAIL;
}

int modem_stat(char *cmd, slist_t *oks, slist_t *ers,int timeout, char *stat, size_t stat_len)
{
	char buf[MAX_STRING];int rc;
	slist_t *cs;time_t t1=t_set(timeout);

	DEBUG(('M',4,"modem_stat: cmd=\"%s\" timeout=%d",cmd,timeout));

	rc=modem_sendstr(cmd);
	if(rc!=1) {
		if(stat) xstrcpy(stat, "FAILURE", stat_len);
		DEBUG(('M',3,"modem_stat: modem_sendstr failed, rc=%d",rc));
		return MC_FAIL;
	}
	if(!oks && !ers) return MC_OK;
	rc=OK;
	while(ISTO(rc) && !t_exp(t1)) {
		rc=tty_gets(buf, MAX_STRING-1, t_rest(t1));
		if(!*buf) continue;
		if(rc!=OK) {
			if(stat) xstrcat(stat, "FAILURE", 
				stat_len);
			DEBUG(('M',3,"modem_stat: tty_gets failed"));
			return MC_FAIL;
		}
		for(cs=oks;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(stat) xstrcat(stat, buf, 
					stat_len);
				return MC_OK;
			}
		for(cs=ers;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(stat) xstrcat(stat, buf, 
					stat_len);
				return MC_ERROR;
			}
		if(stat)
			{
			xstrcat(stat,buf,stat_len);
		 	xstrcat(stat,"\n",stat_len);
			}
	}
	
	if(stat) {
		if (ISTO(rc)) xstrcat(stat, "TIMEOUT", stat_len);
		else xstrcat(stat, "FAILURE", stat_len);
	}
	return MC_FAIL;
}
