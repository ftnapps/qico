/***************************************************************************
 * command-line qico control tool
 * $Id: qctl.c,v 1.15 2004/03/20 16:04:16 sisoft Exp $
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
#include "xstr.h"
#include "clserv.h"
#include "ver.h"

extern time_t gmtoff(time_t tt,int mode);

static int sock=-1;
static char qflgs[Q_MAXBIT]=Q_CHARS;

static void usage(char *ex)
{
	printf("usage: %s [<options>] [<port>] [<node>] [<files>] [<tty>]\n"
 		   "<node>         must be in ftn-style (i.e. zone:net/node[.point])!\n"
		   "-h             this help screen\n"
	           "-P             connect to <port> (default: qicoui or %u)\n"
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
		   "-H             hangup modem session on <tty>\n"
                   "-v             show version\n"
		   "\n",ex,DEF_SERV_PORT);
	exit(0);
}

static RETSIGTYPE timeout(int sig)
{
	fprintf(stderr,"got timeout\n");
	exit(1);
}

static int getanswer()
{
	char buf[MSG_BUFFER];
	int rc;
	signal(SIGALRM, timeout);
	alarm(5);
	rc=xrecv(sock,buf,MSG_BUFFER-1,1);
	if(!rc) {
		fprintf(stderr,"connection to server broken.\n");
		return 1;
	}
	if(rc>0&&rc<MSG_BUFFER)buf[rc]=0;
	if(rc<3||(FETCH16(buf)&&FETCH16(buf)!=(unsigned short)getpid()))return 1;
	if(buf[2])fprintf(stderr, "%s\n", buf+3);
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
	char buf[MSG_BUFFER], *p, *u;
	int rc;
	signal(SIGALRM, timeout);
	alarm(5);
	rc=xrecv(sock,buf,MSG_BUFFER-1,1);
	if(rc<3||(FETCH16(buf)&&FETCH16(buf)!=(unsigned short)getpid()))return 1;
	if(buf[2])fprintf(stderr, "%s\n", buf+3);
	    else {
		for(p=buf+3,rc=0;strlen(p);rc++) {
			printf(infostrs[rc], p);
			u=p;p+=strlen(p)+1;
			if(rc==5) print_worktime(u);

		}
	}
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return buf[2];
}

static int getqueueinfo()
{
	char buf[MSG_BUFFER], *p;
	char cflags[Q_MAXBIT+1];
	int rc;
	char *a, *m, *f, *t;
	long flags;
	int k;
	printf("%-20s %10s %10s %10s %11s\n","Address","Mail","Files","Trys","Flags");
	printf("-----------------------------------------------------------------\n");
	do {
		signal(SIGALRM, timeout);
		alarm(5);
		rc=xrecv(sock,buf,MSG_BUFFER-1,1);
		alarm(0);
		if(rc<3||(FETCH16(buf)&&FETCH16(buf)!=(unsigned short)getpid()))return 1;
		if(buf[2])fprintf(stderr, "%s\n", buf+3);
		    else if(buf[3]) {
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
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	return buf[2];
}

int main(int argc, char *argv[])
{
	int action=-1, kfs=0, len=0,lkfs;
	char c, *str="", flv='?', buf[MSG_BUFFER],filename[MAX_PATH],*port=NULL;
	struct stat filestat;
#ifdef HAVE_SETLOCALE
 	setlocale(LC_ALL, "");
#endif
 	while((c=getopt(argc, argv, "Khqovrp:fkRQHs:x:P:"))!=EOF) {
		switch(c) {
		case 'P':
			if(optarg&&*optarg)port=strdup(optarg);
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
			action=QR_HANGUP;
			break;
		case 'v':
			u_vers("qctl");
		default:
			usage(argv[0]);
		}
	}
	signal(SIGPIPE,SIG_IGN);
	sock=cls_conn(CLS_UI,port);
	if(sock<0) {
		fprintf(stderr,"can't connect to server: %s\n",strerror(errno));
		return 1;
	}

	STORE16(buf,0);
	buf[2]=QR_STYPE;
	snprintf(buf+3,MSG_BUFFER-4,"cqctl-%s",version);
	xsend(sock,buf,strlen(buf+3)+4);
	if(getanswer())return 1;
	STORE16(buf,0);
	buf[2]=action;

	switch(action) {
	case QR_QUIT:
	case QR_SCAN:
	case QR_CONF:
		xsend(sock,buf,3);
		return getanswer();
	case QR_INFO:
		if(optind<argc) {
			xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
			xsend(sock,buf,strlen(argv[optind])+4);
			return getnodeinfo();
		} else {
			usage(argv[0]);
			return 1;
		}
	case QR_KILL:
	case QR_POLL:
		while(optind<argc){
			xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
  			buf[4+strlen(buf+3)]=flv;
			xsend(sock,buf,strlen(argv[optind++])+8);
		}
		return getanswer();
	case QR_HANGUP:
		if(optind<argc){
			strncpy(buf+3,argv[optind],16);
			xsend(sock,buf,strlen(buf+3));
		} else { usage(argv[0]); return 1; }
		return getanswer();
	case QR_STS:
		if(optind<argc) {
			xstrcpy(buf+3, argv[optind], MSG_BUFFER-3);
			len=strlen(argv[optind])+4;
			xstrcpy(buf+len, str, MSG_BUFFER-len);
			xsend(sock,buf,len+strlen(str)+1);
			return getanswer();
		} else {
			usage(argv[0]);
			return 1;
		}
	case QR_REQ:
		for(str=buf+3;optind<argc;optind++) {
			xstrcpy(str, argv[optind], MSG_BUFFER-(str-(char*)buf));
			str+=strlen(str)+1;
		}
		xstrcpy(str, "", 2);str+=2;
		xsend(sock,buf,str-buf);
		return getanswer();
	case QR_SEND:
		str=buf+3;
		if(optind<argc) {
			xstrcpy(str, argv[optind++], MSG_BUFFER-3);
			printf("add '%s'\n",str);
			str+=strlen(str)+1;
		}
		*str++=flv;*str++=0;
		for(;optind<argc;optind++) {
			lkfs=kfs;
			memset(filename,0,MAX_PATH);
			if(argv[optind][0]!='/') {
				getcwd(filename, MAX_PATH-1);xstrcat(filename, "/", MAX_PATH);
			}
			xstrcat(filename, argv[optind], MAX_PATH);
			if(access(filename,R_OK) == -1) {
				printf("Can't access to %s: %s. File skipped!\n",filename,strerror(errno));
				break;
			}
			if(stat(filename,&filestat) == -1) {
				printf("Can't stat file %s: %s. File skipped!\n",filename,strerror(errno));
				break;
			}
			if(!S_ISREG(filestat.st_mode)) {
				printf("File %s is not regular file. Skipped!\n",filename);
				break;
			}
			if(lkfs && (access(filename,W_OK )== -1)) {
				printf("Have no write access to %s. File wouldn't be removed!\n",filename);
				lkfs=0;
			}
			str[0]=lkfs?'^':0;str[1]=0;
			xstrcat(str, filename, MSG_BUFFER-(str-(char*)buf));
			str+=strlen(str)+1;
		}
		xstrcpy(str, "", 2);str+=2;
		xsend(sock,buf,str-buf);
		return getanswer();
	case QR_QUEUE:
		xsend(sock,buf,3);
		return getqueueinfo();
	}
	cls_close(sock);
	usage(argv[0]);
	return 1;
}
