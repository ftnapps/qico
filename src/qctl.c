/***************************************************************************
 * File: qctl.c
 * command-line qico control tool
 * Created at Sun Aug 27 21:24:09 2000 by pqr@yasp.com
 * $Id: qctl.c,v 1.13 2001/02/17 13:29:10 lev Exp $
 ***************************************************************************/
#include <unistd.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <config.h>
#include <errno.h>
#include "qcconst.h"
#include "ver.h"
#include "replace.h"

extern time_t gmtoff(time_t tt);

int qipc_msg;
char qflgs[Q_MAXBIT]=Q_CHARS;

void usage(char *ex)
{
	printf("qctl-%s copyright (c) pavel kurnosoff, 1999-2000\n"
		   "usage: %s [<options>] [<node>] [<files>]\n"
 		   "<node>         must be in ftn-style (i.e. zone:net/node[.point])!\n" 
		   "-h             this help screen\n"
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
		   "               < u>ndialable, <i>mmediate, <w>ait\n"
		   "\n", version, ex);
}

void timeout(int sig)
{
	signal(SIGALRM, SIG_DFL);
	fprintf(stderr,"got timeout\n");
}

int getanswer()
{
	char buf[MSG_BUFFER];
	int rc;
	
	signal(SIGALRM, timeout);
	alarm(1);
	rc=msgrcv(qipc_msg, buf, MSG_BUFFER-1, getpid(), 0);
	if(rc<4) return 1;
	if(buf[4]) fprintf(stderr, "%s\n", buf+5);
	signal(SIGALRM, SIG_DFL);
	alarm(0);
	return buf[4];
}

void print_worktime(char *flags)
{
	char *p;
	time_t tm=time(NULL);
	long tz=gmtoff(tm)/3600;
	
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

char *infostrs[]={
	"Address: %s\n",
	"Station: %s\n",
	"  Place: %s\n",
	"  Sysop: %s\n",
	"  Phone: %s\n",
	"  Flags: %s\n",
	"  Speed: %s\n",
};
	

int getnodeinfo()
{
	char buf[MSG_BUFFER], *p, *u;
	int rc;
	
	signal(SIGALRM, timeout);
	alarm(5);
	rc=msgrcv(qipc_msg, buf, MSG_BUFFER-1, getpid(), 0);
	if(rc<4) return 1;
	if(buf[4]) {
		fprintf(stderr, "%s\n", buf+5);
	} else {
		for(p=buf+5,rc=0;strlen(p);rc++) {
			printf(infostrs[rc], p);
			u=p;p+=strlen(p)+1;
			if(rc==5) print_worktime(u);
				
		}
	}
	signal(SIGALRM, SIG_DFL);
	alarm(0);
	return buf[4];
}

int getqueueinfo()
{
	char buf[MSG_BUFFER], *p;
	char cflags[Q_MAXBIT+1];
	int rc;
	char *a, *m, *f, *t;
	long flags;
	int k;

	printf("%-20s %10s %10s %10s %10s\n","Address","Mail","Files","Trys","Flags");
	printf("----------------------------------------------------------------\n");
	do {
		signal(SIGALRM, timeout);
		alarm(5);
		rc=msgrcv(qipc_msg, buf, MSG_BUFFER-1, getpid(), 0);
		alarm(0);
		if(rc<4) return 1;
		if(buf[4]) {
			fprintf(stderr, "%s\n", buf+5);
		} else if(buf[5]) {
			a = buf + 6;
			m = a + strlen(a) + 1;
			f = m + strlen(m) + 1;
			t = f + strlen(f) + 1;
			p = t + strlen(t) + 1;
			sscanf(p,"%ld",&flags);
			for(k=0;k<Q_MAXBIT;k++) cflags[k] = (flags&(1<<k))?qflgs[k]:'.';
			cflags[Q_MAXBIT] = '\x00';
			printf("%-20s %10s %10s %10s %10s\n",a,m,f,t,cflags);
		}
	} while (buf[5]);
	signal(SIGALRM, SIG_DFL);
	alarm(0);
	return buf[4];
}

int main(int argc, char *argv[])
{
	key_t qipc_key;
	int action=-1, kfs=0;
	char c, *str="", flv='?', buf[MSG_BUFFER];
	
 	setlocale(LC_ALL, "");
 	while((c=getopt(argc, argv, "Khqovrp:fkRQs:x:"))!=EOF) {
		switch(c) {
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
		case 'h':
			usage(argv[0]);
			return 0;
		case 'q':
			action=QR_QUIT;
			break;
		case 'R':
			action=QR_CONF;
			break;
		case 'Q':
			action=QR_SCAN;
			break;
		default:
			return 1;
		}
	}


	if((qipc_key=ftok(QIPC_KEY,QR_MSGQ))<0) {
		fprintf(stderr, "can't get key\n");
		return 1;
	}
	if((qipc_msg=msgget(qipc_key, 0666))<0) {
		fprintf(stderr, "can't get message queue, may be there's no daemon: %s\n",strerror(errno));
		return 1;
	}
	
	*((int *)buf)=1;
	*((int *)buf+1)=getpid();
	buf[8]=action;
	
	switch(action) {
	case QR_QUIT:
	case QR_SCAN:
	case QR_CONF:
		msgsnd(qipc_msg, buf, 9, 0);
		return getanswer();
	case QR_INFO:
		if(optind<argc) {
			strcpy(buf+9, argv[optind]);
			msgsnd(qipc_msg, buf, strlen(argv[optind])+10, 0);
			return getnodeinfo();
		} else {
			usage(argv[0]);
			return 1;
		}
	case QR_KILL:
	case QR_POLL:
		while(optind<argc){
  		    strcpy(buf+9, argv[optind]);
  		    buf[10+strlen(buf+9)]=flv;
			msgsnd(qipc_msg, buf, strlen(argv[optind++])+11, 0);
		}
		return getanswer();
	case QR_STS:
		strcpy(buf+9, argv[optind]);
		strcpy(strlen(argv[optind])+buf+10, str);
		msgsnd(qipc_msg, buf, strlen(argv[optind])+strlen(str)+11, 0);
		return getanswer();
	case QR_REQ:
		for(str=buf+9;optind<argc;optind++) {
			strcpy(str, argv[optind]);
			str+=strlen(str)+1;
		}
		strcpy(str, "");str+=2;
		msgsnd(qipc_msg, buf, str-buf, 0);
		return getanswer();
	case QR_SEND:
		str=buf+9;
		if(optind<argc) {
			strcpy(str, argv[optind++]);
			printf("add '%s'\n",str);
			str+=strlen(str)+1;
		}
		*str++=flv;*str++=0;
		for(;optind<argc;optind++) {
			str[0]=kfs?'^':0;str[1]=0;
			if(argv[optind][0]!='/') {
				getcwd(str+kfs, MAX_PATH-1);strcat(str, "/");
			}
			strcat(str, argv[optind]);
			str+=strlen(str)+1;
		}
		strcpy(str, "");str+=2;
		msgsnd(qipc_msg, buf, str-buf, 0);
		return getanswer();
	case QR_QUEUE:
		msgsnd(qipc_msg, buf, 9, 0);
		return getqueueinfo();
	default:
		usage(argv[0]);
		return 1;
	}
}
