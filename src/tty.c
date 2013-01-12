/**********************************************************
 * work with tty's
 * $Id: tty.c,v 1.16 2004/06/22 08:28:30 sisoft Exp $
 **********************************************************/
#include "headers.h"
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "tty.h"

#define DEBUG_SLEEP 0

char *tty_errs[]={"Ok","tcget/setattr error", "bad speed", "open error",
			"read error","write error","timeout","close error",
				"can't lock port","can't set/get flags"};
static struct termios savetios;
#define IN_MAXBUF 16384
static unsigned char in_buffer[IN_MAXBUF];
static int in_bufpos=0,in_bufmax=0;
#define OUT_MAXBUF 16384
static unsigned char out_buffer[OUT_MAXBUF];
static int out_bufpos=0;

RETSIGTYPE tty_sighup(int sig)
{
	tty_hangedup=1;
	DEBUG(('M',1,"tty: got SIG%s signal",sigs[sig]));
	return;
}

static int selectmy(int n,fd_set *rfs,fd_set *efs,struct timeval *to)
{
	fd_set r,e;
	int sec=to->tv_sec,rc;
	do {
		if(rfs)r=*rfs; else FD_ZERO(&r);
		if(efs)e=*efs; else FD_ZERO(&e);
		if(calling) {
			getevt();
			if(tty_hangedup)return -1;
		}
		if(sec)to->tv_sec=1,sec--;
		    else to->tv_sec=0;
		rc=select(n,&r,NULL,&e,to);
	} while(sec&&!rc);
	to->tv_sec=sec;
	*rfs=r;*efs=e;
#if DEBUG_SLEEP==1
	if(is_ip)usleep(1000);
#endif
	return rc;
}

int tty_isfree(char *port,char *nodial)
{
	int pid;
	FILE *f;
	struct stat s;
	char lckname[MAX_PATH];
	snprintf(lckname,MAX_PATH,"%s.%s",nodial,port);
	if(!stat(lckname,&s)) return 0;
	snprintf(lckname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),port);
	if((f=fopen(lckname,"r"))) {
		fscanf(f,"%d",&pid);
		fclose(f);
		if(kill(pid,0)&&(errno==ESRCH)) {
			lunlink(lckname);
			return 1;
		}
		return 0;
	}
	return 1;
}

char *tty_findport(slist_t *ports,char *nodial)
{
	for(;ports;ports=ports->next)
		if(tty_isfree(baseport(ports->str),nodial))return ports->str;
	return NULL;
}

char *baseport(char *p)
{
	char *q;
	static char pn[20];
	xstrcpy(pn,basename(p),sizeof(pn));
	if((q=strrchr(pn,':')))*q=0;
	return pn;
}

int tty_openport(char *port)
{
	char str[20]="/dev/",*p;
	int speed=0;
	if(*port!='/')xstrcat(str,port,sizeof(str));
	    else xstrcpy(str,port,sizeof(str));
	if((p=strchr(str,':'))) {
		*p++=0;
		speed=atoi(p);
	}
	if(!speed)speed=DEFAULT_SPEED;
	return tty_open(str,speed);
}

void tty_unlock(char *port)
{
	int pid;
	FILE *f;
	char *p,lckname[MAX_PATH];
	DEBUG(('M',4,"tty_unlock"));
	if(!(p=strrchr(port,'/')))p=port; else p++;
	snprintf(lckname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
	if((f=fopen(lckname,"r"))) {
		fscanf(f,"%d",&pid);
		fclose(f);
	}
	if(pid==getpid())lunlink(lckname);
}

int tty_lock(char *port)
{
	int rc=-1;
	char lckname[MAX_PATH],*p;
	DEBUG(('M',4,"tty_lock"));
	if(!(p=strrchr(port,'/')))p=port; else p++;
	snprintf(lckname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),p);
	rc=lockpid(lckname);
	return rc?0:-1;
}

int tty_open(char *port,int speed)
{
	int rc,fd;
	tty_hangedup=0;
	tty_port=xstrdup(port);
	if(tty_lock(port))return ME_CANTLOCK;
	DEBUG(('M',3,"tty_open"));
	fflush(stdin);fflush(stdout);
	setbuf(stdin,NULL);setbuf(stdout,NULL);
	close(STDIN_FILENO);close(STDOUT_FILENO);
	fd=open(port,O_RDONLY|O_NONBLOCK);
	if(dup2(fd,STDIN_FILENO)!=STDIN_FILENO)return ME_OPEN;
	if(fd!=STDIN_FILENO)close(fd);
	fd=open(port,O_WRONLY|O_NONBLOCK);
	if(dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO)return ME_OPEN;
	if(fd!=STDOUT_FILENO)close(fd);
	clearerr(stdin);clearerr(stdout);
	rc=tty_setattr(speed);
	if(rc)return rc;
	return tty_block(1);
}

int tty_close()
{
	if(!tty_port) return ME_CLOSE;
	DEBUG(('M',2,"tty_close"));
	fflush(stdin);fflush(stdout);
	tty_cooked();
	fclose(stdin);fclose(stdout);
	tty_unlock(tty_port);
	xfree(tty_port);
	return ME_OK;
}

int tty_setattr(int speed)
{
	int rc;
	pid_t me=getpid();
	struct termios tios;
	speed_t tspeed=tty_transpeed(speed);
	signal(SIGHUP,tty_sighup);
	signal(SIGPIPE,tty_sighup);
	if(getsid(me)==me) {
#ifdef HAVE_TIOCSCTTY
		/* We are on BSD system, set this terminal as out control terminal */
		ioctl(STDIN_FILENO,TIOCSCTTY);
#endif
		tcsetpgrp(STDIN_FILENO,me);
	}
	rc=tcgetattr(STDIN_FILENO,&savetios);
	if(rc)return ME_ATTRS;
	memcpy(&tios,&savetios,sizeof(tios));
	tios.c_cflag&=~(CSTOPB|PARENB|PARODD);
	tios.c_cflag|=CS8|CREAD|HUPCL|CLOCAL|HARDW_HS;
	tios.c_iflag=0;
	tios.c_oflag=0;
	tios.c_lflag=0;
	if(tspeed) {
		cfsetispeed(&tios,tspeed);
		cfsetospeed(&tios,tspeed);
		/* Speed is zero on answer, and we don't want to flush incoming EMSI_DAT */
		tcflush(STDIN_FILENO,TCIFLUSH);
	}
	tios.c_cc[VTIME]=0;
	tios.c_cc[VMIN]=1;
	rc=tcsetattr(STDIN_FILENO,TCSANOW,&tios);
	return rc?ME_ATTRS:0;
}

speed_t tty_transpeed(int speed)
{
        switch (speed) {
#ifdef B50
        case 50:        return B50;
#endif
#ifdef B75
        case 75:        return B75;
#endif
#ifdef B110
        case 110:       return B110;
#endif
#ifdef B134
        case 134:       return B134;
#endif
#ifdef B150
        case 150:       return B150;
#endif
#ifdef B200
        case 200:       return B200;
#endif
#ifdef B300
        case 300:       return B300;
#endif
#ifdef B600
        case 600:       return B600;
#endif
#ifdef B900
        case 900:       return B900;
#endif
#ifdef B1200
        case 1200:      return B1200;
#endif
#ifdef B1800
        case 1800:      return B1800;
#endif
#ifdef B2400
        case 2400:      return B2400;
#endif
#ifdef B3600
        case 3600:      return B3600;
#endif
#ifdef B4800
        case 4800:      return B4800;
#endif
#ifdef B7200
        case 7200:      return B7200;
#endif
#ifdef B9600
        case 9600:      return B9600;
#endif
#ifdef B12000
        case 12000:     return B12000;
#endif
#ifdef B14400
        case 14400:     return B14400;
#endif
#ifdef B19200
        case 19200:     return B19200;
#elif defined(EXTA)
        case 19200:     return EXTA;
#endif
#ifdef B38400
        case 38400:     return B38400;
#elif defined(EXTB)
        case 38400:     return EXTB;
#endif
#ifdef B57600
        case 57600:     return B57600;
#endif
#ifdef B115200
        case 115200:    return B115200;
#endif
#ifdef B230400
        case 230400:    return B230400;
#endif
#ifdef B460800
        case 460800:    return B460800;
#endif
        }
        return 0;
}

int tty_local(int mode)
{
	int rc;
	struct termios tios;
	signal(SIGHUP,mode?SIG_IGN:tty_sighup);
	signal(SIGPIPE,mode?SIG_IGN:tty_sighup);
	if(!isatty(STDIN_FILENO))return ME_NOTATT;
	rc=tcgetattr(STDIN_FILENO,&tios);
	if(rc) {
		DEBUG(('M',3,"tty_local(%d): tcgetattr failed, errno=%d",mode,errno));
		return ME_ATTRS;
	}
	if(mode)tios.c_cflag|=CLOCAL;
	    else tios.c_cflag&=~CLOCAL;
	tios.c_cflag|=CRTSCTS;
	rc=tcsetattr(STDIN_FILENO,TCSANOW,&tios);
	if(rc) {
		DEBUG(('M',3,"tty_local(%d): tcsetattr failed, errno=%d",mode,errno));
		rc=ME_ATTRS;
	} else DEBUG(('M',4,"tty_local(%d): completed",mode));
	if(mode)tty_hangedup=0;
	return rc;
}

int tty_cooked()
{
	int rc;
	signal(SIGHUP,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
	if(!isatty(STDIN_FILENO))return ME_NOTATT;
	rc=tcsetattr(STDIN_FILENO,TCSAFLUSH,&savetios);
	if(rc) {
		DEBUG(('M',3,"tty_cooked: tcsetattr failed, errno=%d",errno));
		rc=ME_ATTRS;
	} else DEBUG(('M',4,"tty_cooked: completed"));
	return rc;
}

int tty_setdtr(int dtr)
{
	int status,rc;
	rc=ioctl(STDIN_FILENO,TIOCMGET,&status);
	if(rc<0) {
		DEBUG(('M',3,"tty_setdtr: TIOCMGET failed, dtr=%d, errno=%d",dtr,errno));
		return 0;
	}
	if(dtr)status|=TIOCM_DTR;
	    else status&=~TIOCM_DTR;
	rc=ioctl(STDIN_FILENO,TIOCMSET,&status);
	if(rc<0) {
		DEBUG(('M',3,"tty_setdtr: TIOCMSET failed, dtr=%d, errno=%d",dtr,errno));
		return 0;
	}
	DEBUG(('M',4,"tty_setdtr: completed"));
	return 1;
}

int tty_block(int mode)
{
	int flags,rc=0;
	DEBUG(('M',3,"tty_block(%d)",mode));
	flags=fcntl(STDIN_FILENO,F_GETFL,0L);
	if(flags<0)rc=1,flags=0;
	if(mode)flags&=~O_NONBLOCK;
	    else flags|=O_NONBLOCK;
	rc|=fcntl(STDIN_FILENO,F_SETFL,flags);
	if(rc)return ME_FLAGS;
	flags=fcntl(STDOUT_FILENO,F_GETFL,0L);
	if(flags<0)rc=1,flags=0;
	if(mode)flags&=~O_NONBLOCK;
	    else flags|=O_NONBLOCK;
	rc|=fcntl(STDOUT_FILENO,F_SETFL,flags);
	if(rc)return ME_FLAGS;
	return ME_OK;
}

int tty_put(byte *buf,size_t size)
{
	int rc;
	if(tty_hangedup)return RCDO;
	rc=write(STDOUT_FILENO,buf,size);
	if(rc!=size) {
		if(tty_hangedup||errno==EPIPE)return RCDO;
		    else return ERROR;
	}
#if DEBUG_SLEEP==1
	if(is_ip)usleep(1000);
#endif
#ifdef NEED_DEBUG
	for(rc=0;rc<size;rc++)DEBUG(('M',9,"tty_put: '%c' (%d)",C0(buf[rc]),buf[rc]));
#endif
	return OK;
}

int tty_get(byte *buf,size_t size,int *timeout)
{
	int rc,i;
	time_t t;
	fd_set rfds,efds;
	struct timeval tv;
	if(tty_hangedup)return RCDO;
	FD_ZERO(&rfds);FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);
	FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=*timeout;
	tv.tv_usec=0;
	t=time(NULL);
	rc=selectmy(STDIN_FILENO+1,&rfds,&efds,&tv);
	if(rc<0) {
		if(tty_hangedup)return RCDO;
		else return ERROR;
	}
	*timeout-=(time(NULL)-t);
	if(!rc)return TIMEOUT;
	if(FD_ISSET(STDIN_FILENO,&efds))return ERROR;
	rc=read(STDIN_FILENO,buf,size);
	if(rc<1) {
		if(tty_hangedup||errno==EPIPE)return RCDO;
		else return(errno!=EAGAIN&&errno!=EINTR)?ERROR:TIMEOUT;
	}
#if DEBUG_SLEEP==1
	if(is_ip)usleep(1000);
#endif
#ifdef NEED_DEBUG
	for(i=0;i<rc;i++)DEBUG(('M',9,"tty_get: '%c' (%d)",C0(buf[i]),buf[i]));
#endif
	return rc;
}

int tty_bufc(char ch)
{
	int rc=OK;
	if(out_bufpos==OUT_MAXBUF) {
		rc=tty_put(out_buffer,OUT_MAXBUF);
		out_bufpos=0;
	}
	out_buffer[out_bufpos++]=ch;
	return rc;
}

int tty_bufflush()
{	
	int rc=tty_put(out_buffer,out_bufpos);
	out_bufpos=0;
	return rc;
}

void tty_bufclear() { out_bufpos=0; }

int tty_putc(char ch) { return tty_put((byte*)&ch,1); }

int tty_getc(int timeout)
{
	int rc;
	if(in_bufpos<in_bufmax)return in_buffer[in_bufpos++];
	if((rc=tty_get(in_buffer,IN_MAXBUF,&timeout))<0)return rc;
	in_bufpos=0;in_bufmax=rc;
 	return in_buffer[in_bufpos++];
}

int tty_getc_timed(int *timeout)
{
	int rc;
	if(in_bufpos<in_bufmax)return in_buffer[in_bufpos++];
	if((rc=tty_get(in_buffer,IN_MAXBUF,timeout))<0)return rc;
	in_bufpos=0;in_bufmax=rc;
 	return in_buffer[in_bufpos++];
}

int tty_hasdata(int sec,int usec)
{
	int rc;
	fd_set rfds,efds;
	struct timeval tv;
	if(tty_hangedup)return RCDO;
	if(in_bufpos<in_bufmax)return OK;
	FD_ZERO(&rfds);FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);
	FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=sec;
	tv.tv_usec=usec;
	rc=selectmy(STDIN_FILENO+1,&rfds,&efds,&tv);
	if(rc<0) {
		if(tty_hangedup)return RCDO;
		    else return ERROR;
	}
	if(!rc)return TIMEOUT;
	if(rc>0)if(FD_ISSET(STDIN_FILENO,&efds)||!FD_ISSET(STDIN_FILENO,&rfds))return ERROR;
	return OK;
}

int tty_hasdata_timed(int *timeout)
{
	int rc;
	time_t t;
	fd_set rfds,efds;
	struct timeval tv;
	if(tty_hangedup)return RCDO;
	if(in_bufpos<in_bufmax)return OK;
	FD_ZERO(&rfds);FD_ZERO(&efds);
	FD_SET(STDIN_FILENO,&rfds);
	FD_SET(STDIN_FILENO,&efds);
	tv.tv_sec=*timeout;
	tv.tv_usec=0;
	t=time(NULL);
	rc=selectmy(STDIN_FILENO+1,&rfds,&efds,&tv);
	if(rc<0) {
		if(tty_hangedup)return RCDO;
		    else return ERROR;
	}
	*timeout-=(time(NULL)-t);
	if(!rc)return TIMEOUT;
	if(rc>0)if(FD_ISSET(STDIN_FILENO,&efds)||!FD_ISSET(STDIN_FILENO,&rfds))return ERROR;
	return OK;
}

void tty_purge() {
	DEBUG(('M',3,"tty_purge"));
	in_bufmax=in_bufpos= 0;
	tcflush(STDIN_FILENO,TCIFLUSH);
}

void tty_purgeout() {
	out_bufpos=0;
	tcflush(STDOUT_FILENO,TCOFLUSH);
}

char canistr[]={24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0};

int tty_gets(char *what,size_t n,int timeout)
{
	time_t t1;
	int to=timeout,p=0,ch=0;
	*what=0;
	while(to>0&&p<n-1&&ch!='\r'&&ch!='\n') {
		t1=t_start();
		ch=tty_getc(to);
		if(NOTTO(ch))return ch;
		to-=t_time(t1);
		if(ch>=0)what[p++]=ch;
	}
	what[p>0?--p:0]=0;
	if(p>0)DEBUG(('M',1,"<< %s",what));
	if(ch=='\r'||ch=='\n') {
		DEBUG(('M',5,"tty_gets: completed"));
		return OK;
	} else if(to<=0) {
		DEBUG(('M',3,"tty_gets: timed out"));
	} else DEBUG(('M',3,"tty_gets: line too long"));
	return TIMEOUT;
}

int tty_expect(char *what,int timeout)
{
	time_t t1;
	int to=timeout,got=0,p=0,ch;
	calling=0;
	while(!got&&to>0) {
		t1=t_start();
		ch=tty_getc(to);
		if(ch<0)return ch;
		to-=t_time(t1);
		if(ch==what[p])p++;
		    else p=0;
		if(!what[p])got=1;
	}
	return got?OK:TIMEOUT;
}
