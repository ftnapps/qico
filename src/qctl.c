/***************************************************************************
 * command-line qico control tool
 * $Id: qctl.c,v 1.20 2004/05/31 13:15:39 sisoft Exp $
 ***************************************************************************/
#include <config.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include "replace.h"
#include "types.h"
#include "qcconst.h"
#include "byteop.h"
#include "qslib.h"
#include "crc.h"
#include "clserv.h"

static int sock=-1;
static char qflgs[Q_MAXBIT]=Q_CHARS;
static char buf[MSG_BUFFER];

static void usage(char *ex)
{
	printf("usage: %s [<options>] [<nodes>] [<files>]\n"
 		   "<nodes>        must be in ftn-style (i.e. zone:net/node[.point])\n"
		   "-h             this help screen\n"
	           "-P port        connect to <port> (default: qicoui or %u)\n"
	           "-a host        connect to <host> (default: localhost)\n"
	           "-w password    set <password> for connect.\n"
 		   "-q             stop daemon\n"
 		   "-Q             force queue rescan\n"
 		   "-R             reread config\n"
		   "-K             kill outbound of <node> [<node2> <nodeN>]\n"
		   "-f             query info about <node>\n"
		   "-o             query queue (outbound)\n"
		   "-p[n|c|d|h|i]  poll <node1> [<node2> <nodeN>] with specified flavor\n"
		   "               flavors: <n>ormal, <c>rash, <d>irect, <h>old, <i>mm\n"
		   "-r             freq from <node> files <files>\n"
		   "-s[n|c|d|h|i]  attach files <files> to <node> with specified flavor\n"
		   "               flavors: <n>ormal, <c>rash, <d>irect, <h>old, <i>mm\n"
		   "-k             kill attached files after transmission (for -s)\n"
		   "-x[UuWwIi]     set[UWI]/reset[uwi] <node> state(s)\n"
		   "               <u>ndialable, <i>mmediate, <w>ait\n"
		   "-H tty         hangup modem session on <tty>\n"
                   "-v             show version\n"
		   "\n",ex,DEF_SERV_PORT);
	exit(0);
}

void write_log(char *str,...)
{
	va_list args;
	char tmp[MAX_STRING];
	va_start(args,str);
	vsnprintf(tmp,MAX_STRING-1,str,args);
	va_end(args);
	fprintf(stderr,"%s.\n",tmp);
}

static RETSIGTYPE timeout(int sig)
{
	write_log("got timeout");
	exit(1);
}

static int getanswer()
{
	int rc;
	signal(SIGALRM, timeout);
	alarm(5);
	rc=xrecv(sock,buf,MSG_BUFFER-1,1);
	if(!rc) {
		write_log("connection to server broken");
		return 1;
	}
	if(rc>0&&rc<MSG_BUFFER)buf[rc]=0;
	if(rc<3||(FETCH16(buf)&&FETCH16(buf)!=(unsigned short)getpid()))return 1;
	if(buf[2])write_log(buf+3);
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return buf[2];
}

static void print_worktime(char *flags)
{
	char *p;
	time_t tm=time(NULL);
	long tz=gmtoff(tm,1)/3600;
	while((p=strsep(&flags, ","))) {
		if(!strcmp(p,"CM")) {
			printf(" WkTime: 00:00-24:00\n");
			break;
		}
		if(p[0]=='T' && p[3]==0) {
			printf(" WkTime: %02ld:%02d-%02ld:%02d\n",
				   (toupper(p[1])-'A'+tz)%24,
				   islower((int)p[1]) ? 30:0,
				   (toupper(p[2])-'A'+tz)%24,
				   islower((int)p[2]) ? 30:0);
			break;
		}
	}
}

static char *infostrs[]={
	"Address: %s\n",
	"Station: %s\n",
	"  Place: %s\n",
	"  Sysop: %s\n",
	"  Phone: %s\n",
	"  Flags: %s\n",
	"  Speed: %s\n",
};

static int getnodeinfo()
{
	char *p, *u;
	int rc;
	if(!getanswer())
		for(p=buf+3,rc=0;strlen(p);rc++) {
			printf(infostrs[rc], p);
			u=p;p+=strlen(p)+1;
			if(rc==5) print_worktime(u);
		}
	return buf[2];
}

static int getqueueinfo()
{
	char *p;
	char cflags[Q_MAXBIT+1];
	char *a, *m, *f, *t;
	long flags;
	int k;
	printf("%-20s %10s %10s %10s %11s\n","Address","Mail","Files","Trys","Flags");
	printf("-----------------------------------------------------------------\n");
	do {
		if(!getanswer())
		    if(buf[3]) {
			a = buf + 4;
			m = a + strlen(a) + 1;
			f = m + strlen(m) + 1;
			t = f + strlen(f) + 1;
			p = t + strlen(t) + 1;
			sscanf(p,"%ld",&flags);
			for(k=0;k<Q_MAXBIT;k++) cflags[k] = (flags&(1<<k))?qflgs[k]:'.';
			cflags[Q_MAXBIT] = '\x00';
			printf("%-20s %10s %10s %10s %11s\n",a,m,f,t,cflags);
		}
	} while (buf[3]);
	return buf[2];
}

static int xsendget(int sock,char *buf,size_t len)
{
	xsend(sock,buf,len);
	return getanswer();
}

int main(int argc, char **argv,char **envp)
{
	int action=-1, kfs=0, len=0,lkfs,rc=1;
	char filename[MAX_PATH];
	unsigned char digest[16]={0};
	char c,*str="",flv='?',*port=NULL,*addr=NULL,*tty=NULL,*pwd=NULL;
	struct stat filestat;
#ifdef HAVE_SETLOCALE
 	setlocale(LC_ALL, "");
#endif
 	while((c=getopt(argc, argv, "Khqovrp:fkRQH:s:x:P:a:w:"))!=EOF) {
		switch(c) {
		case 'P':
			if(optarg&&*optarg)port=xstrdup(optarg);
			break;
		case 'a':
			if(optarg&&*optarg)addr=xstrdup(optarg);
			break;
		case 'w':
			if(optarg&&*optarg)pwd=xstrdup(optarg);
			break;
		case 'k':
			kfs=1;
			break;
		case 'x':
			action=QR_STS;
			str=optarg;
			break;
		case 's':
			action=QR_SEND;
			flv=toupper(*optarg);
			if(strchr("0123456789:./",flv)) {
				flv='?';
				optind--;
			}
			break;
		case 'K':
			action=QR_KILL;
			break;
		case 'p':
			action=QR_POLL;
			flv=toupper(*optarg);
			if(strchr("0123456789:./",flv)) {
				flv='?';
				optind--;
			}
			break;
		case 'o':
			action=QR_QUEUE;
			break;
		case 'f':
			action=QR_INFO;
			break;
		case 'r':
			action=QR_REQ;
			break;
		case 'q':
			action=QR_QUIT;
			break;
		case 'R':
			action=QR_CONF;
			break;
		case 'Q':
			action=QR_SCAN;
			break;
		case 'H':
			if(optarg&&*optarg)tty=xstrdup(optarg);
			action=QR_HANGUP;
			break;
		case 'v':
			u_vers("qctl");
		default:
			usage(argv[0]);
		}
	}
	if(action!=QR_QUIT
	 &&action!=QR_SCAN
	 &&action!=QR_CONF
	 &&action!=QR_QUEUE
	 &&optind>=argc)usage(argv[0]);

	signal(SIGPIPE,SIG_IGN);
	sock=cls_conn(CLS_UI,port,addr);
	if(sock<0) {
		write_log("can't connect to server: %s",strerror(errno));
		return 1;
	}
	signal(SIGALRM, timeout);
	alarm(6);
	rc=xrecv(sock,buf,MSG_BUFFER-1,1);
	if(!rc) {
		write_log("connection to server broken");
		return 1;
	}
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	if(strcmp(buf,"qs-noauth")) {
		if(pwd&&*pwd=='-'&&!pwd[1]) {
			xfree(pwd);
			pwd=getpass("password: ");
			if(!pwd) {
				write_log("getpass() error: %s",strerror(errno));
				cls_close(sock);
				return 1;
			}
			pwd=xstrdup(pwd);
		}
		if(!pwd||(pwd&&!*pwd)) {
			write_log("can't connect: password required");
			cls_close(sock);
			return 1;
		}
		md5_cram_get((unsigned char*)pwd,(unsigned char*)buf,10,digest);
	}
	STORE16(buf,0);
	buf[2]=QR_STYPE;
	buf[3]='c';/*control*/
	memcpy(buf+4,digest,16);
	snprintf(buf+20,MSG_BUFFER-21,"qctl-%s",version);
	xsend(sock,buf,strlen(buf+20)+20);
	if(getanswer()) {
		cls_close(sock);
		return 1;
	}
	STORE16(buf,0);
	buf[2]=action;

	switch(action) {
	    case QR_QUIT:
	    case QR_SCAN:
	    case QR_CONF:
		rc=xsendget(sock,buf,3);
		break;
	    case QR_INFO:
		xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
		xsend(sock,buf,strlen(argv[optind])+4);
		rc=getnodeinfo();
		break;
	    case QR_KILL:
	    case QR_POLL:
		do {
			STORE16(buf,0);buf[2]=action;
			xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
  			buf[4+strlen(buf+3)]=flv;
			rc=xsendget(sock,buf,strlen(argv[optind++])+8);
		} while(optind<argc);
		break;
	    case QR_HANGUP:
		if(tty) {
			strncpy(buf+3,tty,16);
			rc=xsendget(sock,buf,strlen(buf+3));
		} else usage(argv[0]);
		break;
	    case QR_STS:
		xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
		len=strlen(argv[optind])+4;
		xstrcpy(buf+len, str, MSG_BUFFER-len);
		rc=xsendget(sock,buf,len+strlen(str)+1);
		break;
	    case QR_REQ:
		for(str=buf+3;optind<argc;optind++) {
			xstrcpy(str, argv[optind], MSG_BUFFER-(str-(char*)buf));
			str+=strlen(str)+1;
		}
		xstrcpy(str, "", 2);str+=2;
		rc=xsendget(sock,buf,str-buf);
		break;
	    case QR_SEND: {
		int nf=0;
		str=buf+3;
		xstrcpy(str, argv[optind++], MSG_BUFFER-3);
		str+=strlen(str)+1;
		*str++=flv;*str++=0;
		for(;optind<argc;optind++) {
			lkfs=kfs;
			memset(filename,0,MAX_PATH);
			if(argv[optind][0]!='/') {
				getcwd(filename, MAX_PATH-1);xstrcat(filename, "/", MAX_PATH);
			}
			xstrcat(filename, argv[optind], MAX_PATH);
			if(access(filename,R_OK) == -1) {
				write_log("can't access to %s: %s. skipped",filename,strerror(errno));
				continue;
			}
			if(stat(filename,&filestat) == -1) {
				write_log("can't stat file %s: %s. skipped",filename,strerror(errno));
				continue;
			}
			if(!S_ISREG(filestat.st_mode)) {
				write_log("file %s is not regular file. skipped",filename);
				continue;
			}
			if(lkfs && (access(filename,W_OK )== -1)) {
				write_log("have no write access to %s. file wouldn't be removed",filename);
				lkfs=0;
			}
			str[0]=lkfs?'^':0;str[1]=0;
			xstrcat(str, filename, MSG_BUFFER-(str-(char*)buf));
			str+=strlen(str)+1;nf++;
		}
		if(nf) {
			xstrcpy(str, "", 2);str+=2;
			rc=xsendget(sock,buf,str-buf);
		} else write_log("no files to send");
	      } break;
	    case QR_QUEUE:
		xsend(sock,buf,3);
		rc=getqueueinfo();
		break;
	}
	cls_close(sock);
	return rc;
}
