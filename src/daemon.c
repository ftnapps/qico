/**********************************************************
 * qico daemon
 * $Id: daemon.c,v 1.25 2004/05/24 03:21:36 sisoft Exp $
 **********************************************************/
#include <config.h>
#ifdef HAVE_DNOTIFY
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#undef _GNU_SOURCE
#endif
#include "headers.h"
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include "tty.h"
#include "clserv.h"

static short tosend=0;
static cls_cl_t *cl=NULL;
static cls_ln_t *ln=NULL;
static int t_dial=0,c_delay,rnum;
#ifdef HAVE_DNOTIFY
static int dnot;
#endif

static RETSIGTYPE sigchild(int sig)
{
	int rc,wr;
	signal(sig,sigchild);
	if((wr=wait(&rc))<0)write_log("wait() returned %d (%d)",wr,rc);
	rc=WEXITSTATUS(rc)&S_MASK;
	if(rc==S_OK||rc==S_REDIAL)do_rescan=1;
}

static RETSIGTYPE sighup(int sig)
{
	signal(sig,sighup);
	write_log("got SIGHUP, trying to reread configs...");
	killsubsts(&psubsts);
	killconfig();
	if(!readconfig(configname)) {
		write_log("there was some errors parsing config, exiting");
		stopit(0);
	}
	psubsts=parsesubsts(cfgfasl(CFG_SUBST));
#ifdef NEED_DEBUG
	parse_log_levels();
#endif
	do_rescan=1;
}

#ifdef HAVE_DNOTIFY
static RETSIGTYPE sigrt(int sig)
{
	signal(sig,sigrt);
	if(do_rescan==2)return;
	DEBUG(('Q',3,"got SIGRT"));
	do_rescan=2;rnum=0;
}
#endif

static char *sts_str(int flags)
{
	static char s[Q_MAXBIT+1];
	char qchars[]=Q_CHARS;int i;
	for(i=0;i<Q_MAXBIT;i++)s[i]=(flags&(1<<i))?qchars[i]:'.';
	s[Q_MAXBIT]=0;
	return s;
}

static void sendrpkt(char what,int sock,char *fmt,...)
{
	char buf[MSG_BUFFER];
	int rc;
	va_list args;
	STORE16(buf,0);
	buf[2]=what;
	va_start(args,fmt);
	rc=vsnprintf(buf+3,MSG_BUFFER-3,fmt,args);
	va_end(args);
	if(xsend(sock,buf,rc+4)<0)DEBUG(('I',1,"can't send (fd=%d): %s",sock,strerror(errno)));
}

int daemon_xsend(int sock,char *buf,size_t len)
{
	cls_cl_t *uis=cl;
	if(!uis)return len;
	while(uis) {
		if(uis->type=='e'&&(!tosend||tosend==uis->sock))
		    if(xsend(uis->sock,buf,len)<0)DEBUG(('I',1,"can't send to client %d (fd=%d): %s",uis->id,uis->sock,strerror(errno)));
		uis=uis->next;
	}
	return len;
}

static void daemon_evt(int chld,char *buf,int rc,int mode)
{
	int res,set;
	char *p;
	sts_t sts;
	qitem_t *sinfo;
	FTNADDR_T(fa);
	slist_t *sl;
	cls_cl_t *uis;
	cls_ln_t *lins;
	if(!mode) {
		if(buf[2]==QC_EMSID||buf[2]==QC_LIDLE) {
			for(lins=ln;lins&&lins->id!=FETCH16(buf);lins=lins->next);
			if(lins) {
				if(rc>4&&buf[2]==QC_EMSID)lins->emsi=xrealloc(lins->emsi,rc);
				    else xfree(lins->emsi);
				DEBUG(('I',4,"got emsi (%d)",lins->emsi?rc:0));
				if(lins->emsi)memcpy(lins->emsi,buf,rc);
				lins->emsilen=rc;
			} else DEBUG(('I',1,"got emsi from unknown line %d",FETCH16(buf)));
		}
		if(rc>2)daemon_xsend(chld,buf,rc);
		return;
	}
	for(uis=cl;uis&&uis->sock!=chld;uis=uis->next);
	if(!uis) {
		DEBUG(('I',1,"got message from unknown client (fd=%d), skipped",chld));
		return;
	}
	if(rc>0)buf[rc]=0;
	if(buf[2]==QR_STYPE) {
		DEBUG(('I',1,"client %d: type '%c' (%s)",uis->id,buf[3],buf+20));
		if(!strchr("euic",buf[3])) {
			DEBUG(('I',1,"is unknown type"));
			sendrpkt(1,chld,"unknown client type '%c'",buf[3]);
			return;
		}
		if(uis->auth&&cfgs(CFG_SERVERPWD)) {
			unsigned char digest[16];
			md5_cram_get((unsigned char*)ccs,uis->auth,10,digest);
			if(memcmp(buf+4,digest,16)) {
				DEBUG(('I',1,"client %d: auth failed",uis->id));
				sendrpkt(1,chld,"authentication failed");
				return;
			}
			xfree(uis->auth);
		}
		uis->type=buf[3];
		sendrpkt(0,chld,"");
		if(uis->type=='e') {
			tosend=chld;
			qpmydata();
			qsendqueue();
			for(lins=ln;lins;lins=lins->next)
			    if(lins->emsi)daemon_xsend(chld,lins->emsi,lins->emsilen);
			tosend=0;
		}
		return;
	}
	if(uis->type=='u') {
		DEBUG(('I',1,"client %d: msg before auth",uis->id));
		sendrpkt(1,chld,"need basic auth!");
		return;
	}
	if(FETCH16(buf)) {
		DEBUG(('I',2,"client %d: msg for pid %d",uis->id,FETCH16(buf)));
		for(lins=ln;lins&&lins->id!=FETCH16(buf);lins=lins->next);
		if(lins) {
			if(xsendto(lins_sock,buf,rc,lins->addr)<0)DEBUG(('I',1,"can't send to line %d: %s",lins->id,strerror(errno)));
		} else DEBUG(('I',2,"line not found"));
		return;
	}
	if(buf[2]==QR_POLL||buf[2]==QR_REQ||buf[2]==QR_INFO||
	    buf[2]==QR_SEND||buf[2]==QR_STS||buf[2]==QR_KILL) {
		if(!parseftnaddr(buf+3, &fa, &DEFADDR, 0)) {
			sendrpkt(1,chld,"can't parse address '%s'!", buf+3);
			write_log("can't parse address '%s'!", buf+3);
			return;
		}
	}
	switch(buf[2]) {
	    case QR_QUIT:
		DEBUG(('I',2,"client %d: request quit",uis->id));
		if(uis->type!='c') {
			sendrpkt(1,chld,"wrong client type, ignored");
			break;
		}
		sendrpkt(0,chld,"");
#ifdef HAVE_DNOTIFY
		if(dnot>0)close(dnot);
#endif
		if(BSO)bso_done();
		if(ASO)aso_done();
		write_log("exiting by request");
		if(cfgs(CFG_PIDFILE))
		    if(getpid()==islocked(ccs))lunlink(ccs);
		log_done();
		qqreset();sline("");title("");
		qsendpkt(QC_QUIT,"master","",1);
		cls_shutd(lins_sock);
		cls_shutd(uis_sock);
		exit(0);
	    case QR_CONF:
		DEBUG(('I',2,"client %d: request reread config",uis->id));
		write_log("trying to reread configs by request...");
		killsubsts(&psubsts);
		killconfig();
		if(!readconfig(configname)) {
			sendrpkt(1,chld,"bad config, terminated");
			write_log("there was some errors parsing config, exiting");
			stopit(0);
		}
		sendrpkt(0,chld,"");
		psubsts=parsesubsts(cfgfasl(CFG_SUBST));
#ifdef NEED_DEBUG
		parse_log_levels();
#endif
		do_rescan=1;
		break;
	    case QR_SCAN:
		DEBUG(('I',2,"client %d: request rescan",uis->id));
		sendrpkt(0,chld,"");
		do_rescan=1;rnum=0;
		break;
	    case QR_RESTMR:
		sendrpkt(0,chld,"");
		c_delay=1;
		t_dial=0;
		break;
	    case QR_HANGUP: {
		FILE *f;int pid;
		char fname[MAX_PATH];
		DEBUG(('I',2,"client %d: request hangup line '%s'",uis->id,buf+3));
		sendrpkt(0,chld,"");
		snprintf(fname,MAX_PATH,"%s/LCK..%s",cfgs(CFG_LOCKDIR),buf+3);
		if((f=fopen(fname,"r"))) {
			fscanf(f,"%d",&pid);
			fclose(f);
			if(kill(pid,SIGHUP)<0&&errno==ESRCH)lunlink(fname);
		}
		} break;
	    case QR_POLL: {
		int locked=0;
		DEBUG(('I',2,"client %d: request poll addr %s",uis->id,buf+3));
		if(BSO)locked|=bso_locknode(&fa,LCK_t);
		if(ASO)locked|=aso_locknode(&fa,LCK_t);
		if(locked) {
			p=buf+strlen(buf+3)+4;
			if(*p=='?')*p=*cfgs(CFG_POLLFLAVOR);
			rc=bso_flavor(*p);
			if(rc==F_ERR) {
				sendrpkt(1,chld,"unknown flavour %c",C0(*p));
				write_log("unknown flavour - '%c'",C0(*p));
				break;
			}
			sendrpkt(0,chld,"");
			write_log("poll for %s, flavor %c", ftnaddrtoa(&fa),*p);
			if(BSO) {
				rc=bso_poll(&fa,rc);
				bso_unlocknode(&fa,LCK_t);
				bso_getstatus(&fa,&sts);
				if(sts.flags&Q_IMM) {
					sts.flags&=~Q_IMM;
					write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
					bso_setstatus(&fa,&sts);
				}
			}
			if(ASO) {
				if(!BSO)rc=aso_poll(&fa,rc);
				aso_unlocknode(&fa,LCK_t);
				aso_getstatus(&fa,&sts);
				if(sts.flags&Q_IMM) {
					sts.flags&=~Q_IMM;
					if(!BSO)write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
					aso_setstatus(&fa,&sts);
				}
			}
			do_rescan=1;
		    } else {
			sendrpkt(1,chld,"can't create poll for %s",ftnaddrtoa(&fa));
			write_log("can't create poll for %s!",ftnaddrtoa(&fa));
		}
		} break;
	    case QR_KILL: {
		int locked=0;
		DEBUG(('I',2,"client %d: request kill addr %s",uis->id,buf+3));
		if(BSO)locked|=bso_locknode(&fa,LCK_t);
		if(ASO)locked|=aso_locknode(&fa,LCK_t);
		if(locked) {
			sendrpkt(0,chld,"");
			write_log("kill %s",ftnaddrtoa(&fa));
			simulate_send(&fa);
			if(BSO)bso_unlocknode(&fa,LCK_t);
			if(ASO)aso_unlocknode(&fa,LCK_t);
			do_rescan=1;
		    } else {
			sendrpkt(1,chld,"can't kill %s",ftnaddrtoa(&fa));
			write_log("can't kill %s!",ftnaddrtoa(&fa));
			}
		} break;
	    case QR_STS:
		DEBUG(('I',2,"client %d: request flags change addr %s",uis->id,buf+3));
		p=buf+strlen(buf+3)+4;
		rc=1;res=0;set=0;
		while(*p&&rc) {
			switch(*p) {
			case 'W': set|=Q_WAITA;break;
			case 'I': set|=Q_IMM;break;
			case 'U': set|=Q_UNDIAL;break;
			case 'w': res|=Q_ANYWAIT;break;
			case 'i': res|=Q_IMM;break;
			case 'u': res|=Q_UNDIAL;break;
			case 'h': set|=Q_WAITA;rc=2;break;
			default:
				sendrpkt(1,chld,"unknown status action: %c",*p);
				write_log("unknown status action: %c",*p);
				rc=0;
			}
			p++;
		}
		if(rc) {
			sendrpkt(0,chld,"");
			if(BSO) {
				bso_getstatus(&fa,&sts);
				sts.flags|=set;sts.flags&=~res;p++;
				if(rc!=2)write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
				    else write_log("hold %s for %d min (new status: [%s])",ftnaddrtoa(&fa),FETCH16(p),sts_str(sts.flags));
				if(set&Q_WAITA&&!(res&Q_ANYWAIT))sts.htime=t_set((rc==2?(FETCH16(p)):cfgi(CFG_WAITHRQ))*60);
				if(set&Q_UNDIAL)sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
				if(res&Q_UNDIAL)sts.try=0;
				bso_setstatus(&fa,&sts);
			}
			if(ASO) {
				aso_getstatus(&fa,&sts);
				sts.flags|=set;sts.flags&=~res;p++;
				if(!BSO){if(rc!=2)write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
				    else write_log("hold %s for %d min (new status: [%s])",ftnaddrtoa(&fa),FETCH16(p),sts_str(sts.flags));}
				if(set&Q_WAITA&&!(res&Q_ANYWAIT))sts.htime=t_set((rc==2?(FETCH16(p)):cfgi(CFG_WAITHRQ))*60);
				if(set&Q_UNDIAL)sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
				if(res&Q_UNDIAL)sts.try=0;
				aso_setstatus(&fa,&sts);
			}
			do_rescan=1;
		}
		break;
	    case QR_REQ: {
		int locked=0,l;
		DEBUG(('I',2,"client %d: request filereq addr %s",uis->id,buf+3));
		if(BSO)locked|=bso_locknode(&fa,LCK_t);
		if(ASO)locked|=aso_locknode(&fa,LCK_t);
		if(locked) {
			sendrpkt(0,chld,"");
			sl=NULL;p=buf+strlen(buf+3)+4;
			while((l=strlen(p))){
				if(*p=='"'&&p[l-1]=='"') {
					l--;p[l]=0;p++;
					if(l<2)goto skiprq;
				}
				write_log("requested '%s' from %s",p,ftnaddrtoa(&fa));
				if(strchr(cfgs(CFG_MAPOUT),'r'))recode_to_remote(p);
				slist_add(&sl,p);
skiprq:				p+=l+1;
			}
			if(BSO)rc=bso_request(&fa,sl);
			else if(ASO)rc=aso_request(&fa,sl);
			slist_kill(&sl);
			if(BSO) {
				bso_unlocknode(&fa,LCK_t);
				bso_getstatus(&fa,&sts);
				if(sts.flags&Q_IMM) {
					sts.flags&=~Q_IMM;
					write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
					bso_setstatus(&fa,&sts);
				}
			}
			if(ASO) {
				aso_unlocknode(&fa,LCK_t);
				aso_getstatus(&fa,&sts);
				if(sts.flags&Q_IMM) {
					sts.flags&=~Q_IMM;
					if(!BSO)write_log("changing status of %s to [%s]",ftnaddrtoa(&fa),sts_str(sts.flags));
					aso_setstatus(&fa,&sts);
				}
			}
			do_rescan=1;
		    } else {
			sendrpkt(1,chld,"can't lock node %s",ftnaddrtoa(&fa));
			write_log("can't lock node %s!",ftnaddrtoa(&fa));
		}
		} break;
	    case QR_SEND: {
		int locked=0;
		DEBUG(('I',2,"client %d: request send addr %s",uis->id,buf+3));
		p=buf+strlen(buf+3)+4;
		if(*p=='?')*p=*cfgs(CFG_POLLFLAVOR);
		rc=bso_flavor(*p);
		if(rc==F_ERR) {
			sendrpkt(1,chld,"unknown flavour %c",C0(*p));
			write_log("unknown flavour - '%c'",C0(*p));
			break;
		}
		p+=strlen(p)+1;
		if(BSO)locked|=bso_locknode(&fa,LCK_t);
		if(ASO)locked|=aso_locknode(&fa,LCK_t);
		if(locked) {
			sendrpkt(0,chld,"");
			sl=NULL;
			while(strlen(p)){
				write_log("attaching '%s' to %s%s",
					(p[0]=='^')?p+1:p,ftnaddrtoa(&fa),
					(p[0]=='^')?" (k/s)":"");
				slist_add(&sl,p);
				p+=strlen(p)+1;
			}
			if(BSO)rc=bso_attach(&fa,rc,sl);
			    else if(ASO)rc=aso_attach(&fa,rc,sl);
			slist_kill(&sl);
			if(BSO)bso_unlocknode(&fa,LCK_t);
			if(ASO)aso_unlocknode(&fa,LCK_t);
			do_rescan=1;
		    } else {
			sendrpkt(1,chld,"can't lock node %s",ftnaddrtoa(&fa));
			write_log("can't lock node %s!",ftnaddrtoa(&fa));
		}
		} break;
	    case QR_INFO:
		DEBUG(('I',2,"client %d: request nlinfo addr %s",uis->id,buf+3));
		rc=query_nodelist(&fa,cfgs(CFG_NLPATH),&rnode);
		switch(rc) {
		    case 0:
			if(rnode) {
				write_log("returned info about %s (%s)",rnode->name,ftnaddrtoa(&fa));
				sendrpkt(0,chld,"%s%c%s%c%s%c%s%c%s%c%s%c%d%c",
						 ftnaddrtoa(&fa),0,
						 rnode->name,0,rnode->place,0,
						 rnode->sysop,0,rnode->phone,0,
						 rnode->flags,0,rnode->speed,0
					);
				nlkill(&rnode);
			} else {
				sendrpkt(1,chld,"%s not found in nodelist",ftnaddrtoa(&fa));
				write_log("%s not found in nodelist",ftnaddrtoa(&fa));
			}
			break;
		    case 1:
			sendrpkt(1,chld,"can't query nodelist, index error");
			write_log("can't query nodelist, index error");
			break;
		    case 2:
			sendrpkt(1,chld,"can't query nodelist, nodelist error");
			write_log("can't query nodelist, nodelist error");
			break;
		    case 3:
			sendrpkt(1,chld,"index is older than the list, need recompile");
			write_log("index is older than the list, need recompile");
			break;
		    default:
			sendrpkt(1,chld,"nodelist query error");
			write_log("nodelist query error");
			break;
		}
		break;
	    case QR_QUEUE:
		DEBUG(('I',2,"client %d: request queue",uis->id));
		sinfo=q_queue;
		if(sinfo) do {
			sendrpkt(0,chld,"%c%s%c%lu%c%lu%c%lu%c%lu",
				(char)1,
				ftnaddrtoa(&sinfo->addr),(char)0,
				(unsigned long)sinfo->pkts,(char)0,
				(unsigned long)(q_sum(sinfo)+sinfo->reqs),(char)0,
				(unsigned long)sinfo->try,(char)0,
				(unsigned long)sinfo->flv);
		} while ((sinfo=sinfo->next));
		sendrpkt(0,chld,"%c",0);
		break;
	    default:
		DEBUG(('I',2,"client %d: bad request",uis->id));
		sendrpkt(1,chld,"bad request");
		write_log("got unsupported packet type: %c",C0(buf[2]));
	}
}

void daemon_mode()
{
	int t_rescan=0,rc=1,dable;
	int f,w,hld,mailonly,rescanperiod;
	char *port=NULL,buf[MSG_BUFFER];
	static short curr_id=1;
	sts_t sts;
	pid_t chld;
	qitem_t *current=q_queue,*i;
	qitem_t *uncurr=q_queue;
	time_t t;
	fd_set rfds;
	cls_cl_t *uis,*uit;
	cls_ln_t *lins,*lnt;
	struct timeval tv;
	struct sockaddr sa;
	if(cfgs(CFG_ROOTDIR)&&*ccs)chdir(ccs);
	if(getppid()!=1) {
		signal(SIGTTOU,SIG_IGN);
		signal(SIGTTIN,SIG_IGN);
		signal(SIGTSTP,SIG_IGN);
		if((rc=fork())>0)exit(0);
		if(rc<0) {
			write_log("can't spawn daemon!");
			fprintf(stderr,"can't spawn daemon!\n");
			exit(1);
		}
	}
	signal(SIGINT,sigerr);
	signal(SIGTERM,sigerr);
	signal(SIGFPE,sigerr);
	signal(SIGCHLD,sigchild);
	signal(SIGHUP,sighup);
	signal(SIGPIPE,SIG_IGN);
	if(cfgs(CFG_PIDFILE))if(!lockpid(ccs)) {
		write_log("another daemon exists or can't create pid file!");
		fprintf(stderr,"another daemon exists or can't create pid file!\n");
		exit(1);
	}
	lins_sock=cls_conn(CLS_SERV_L,cfgs(CFG_SERVER),NULL);
	if(lins_sock<0) {
		write_log("can't create server_udp: %s",strerror(errno));
		exit(1);
	}
	uis_sock=cls_conn(CLS_SERV_U,cfgs(CFG_SERVER),NULL);
	if(uis_sock<0) {
		write_log("can't create server_tcp: %s",strerror(errno));
		cls_shutd(lins_sock);
		exit(1);
	}
	log_callback=vlogs;xsend_cb=daemon_xsend;tosend=0;
	if(!bso_init(cfgs(CFG_BSOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init BSO");
	if(!aso_init(cfgs(CFG_ASOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init ASO");
	if(!BSO&&!ASO) {
		write_log("No outbound defined");
		cls_shutd(lins_sock);cls_shutd(uis_sock);
		exit(1);
	}
	to_dev_null();setsid();
	write_log("%s-%s/%s daemon started",progname,version,osname);
#ifdef HAVE_DNOTIFY
	if(ASO) {
		dnot=open(cfgs(CFG_ASOOUTBOUND),O_RDONLY);
		if(dnot<0)DEBUG(('Q',1,"can't open %s: %s",ccs,strerror(errno)));
		    else {
			signal(SIGRTMIN,sigrt);
			fcntl(dnot,F_SETSIG,SIGRTMIN);
			fcntl(dnot,F_NOTIFY,DN_MODIFY|DN_CREATE|DN_DELETE|DN_RENAME|DN_MULTISHOT);
		}
	} else dnot=-1;
#endif
	t_rescan=cfgi(CFG_RESCANPERIOD);
	srand(time(NULL));rnum=-1;
	c_delay=randper(cfgi(CFG_DIALDELAY),cfgi(CFG_DIALDELTA));
	while(1) {
		rescanperiod=cfgi(CFG_RESCANPERIOD);
		if(do_rescan>1){t_rescan=rescanperiod-do_rescan;do_rescan=0;}
		title("Queue manager [%d]",rescanperiod-t_rescan);
		if(t_rescan>=rescanperiod||do_rescan) {
			/*sline("Rescanning outbound...");*/
			if(rnum<0)rnum=cfgi(CFG_LONGRESCAN)-1;
			do_rescan=0;
			if(!q_rescan(&current,!rnum--))write_log("can't rescan outbound!");
			t_rescan=0;
		}
		sline("Waiting %d...",c_delay-t_dial);
		if(t_dial>=c_delay) {
			c_delay=randper(cfgi(CFG_DIALDELAY),cfgi(CFG_DIALDELTA));
			t_dial=dable=0;
			mailonly=checktimegaps(cfgs(CFG_MAILONLY))||checktimegaps(cfgs(CFG_ZMH));
			if(!q_queue)dable=1;
			i=q_queue;
			DEBUG(('Q',1,"dabl"));
			while(!dable&&i) {
				f=current->flv;
				w=current->what;
				if(f&Q_ANYWAIT)
					if(t_exp(current->onhold)) {
						current->onhold=0;
						f&=~Q_ANYWAIT;
					}
				if(f&Q_UNDIAL&&cfgi(CFG_CLEARUNDIAL)!=0) {
					if(BSO) {
						bso_getstatus(&current->addr,&sts);
						if (t_exp(sts.utime)) {
							sts.flags&=~Q_UNDIAL;
							sts.try=0;
							sts.utime=0;
							bso_setstatus(&current->addr,&sts);
							f&=~Q_UNDIAL;
							write_log("changing status of %s to [%s]",ftnaddrtoa(&current->addr),sts_str(sts.flags));
						}
					}
					if(ASO) {
						aso_getstatus(&current->addr,&sts);
						if (t_exp(sts.utime)) {
							sts.flags&=~Q_UNDIAL;
							sts.try=0;
							sts.utime=0;
							aso_setstatus(&current->addr,&sts);
							f&=~Q_UNDIAL;
							write_log("changing status of %s to [%s]",ftnaddrtoa(&current->addr),sts_str(sts.flags));
						}
					}
				}
 				if(falist_find(cfgal(CFG_ADDRESS),&current->addr)||
					f&Q_UNDIAL||!havestatus(f,CFG_CALLONFLAVORS)||
					needhold(f,w)||(mailonly&&!current->pkts)) {
					    current=current->next;
					    if(!current)current=q_queue;
					    i=i->next;
					    continue;
				}
				rc=query_nodelist(&current->addr,cfgs(CFG_NLPATH),&rnode);
				DEBUG(('Q',1,"querynl"));
				switch(rc) {
					case 1:write_log("can't query nodelist, index error");break;
					case 2:write_log("can't query nodelist, nodelist error");break;
					case 3:write_log("index is older than the list, need recompile");break;
				}
				if(!rnode) {
					rnode=xcalloc(1,sizeof(ninfo_t));
					falist_add(&rnode->addrs,&current->addr);
					rnode->name=xstrdup("Unknown");
					rnode->phone=xstrdup("");
				}
				if(!(rnode->opt&(MO_BINKP|MO_IFC)))xfree(rnode->host);
				DEBUG(('Q',1,"ndl: %s %s %s [%d]",ftnaddrtoa(&current->addr),rnode?(rnode->host?rnode->host:rnode->phone):"$",rnode->haswtime?rnode->wtime:"$",rnode->hidnum));
				if(!cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
				if(checktimegaps(cfgs(CFG_CANCALL))&&find_dialable_subst(rnode,havestatus(f,CFG_IMMONFLAVORS),psubsts)) {
					xfree(rnode->tty);
					if(rnode->host) {
						static struct stat s;
						static char lckname[MAX_PATH];
						snprintf(lckname,MAX_PATH,"%s.tcpip",cfgs(CFG_NODIAL));
						if(!stat(lckname,&s))goto nlkil;
						is_ip=1;
						if(rnode->opt&MO_BINKP)bink=1;
						xstrcpy(ip_id,"ipline",10);
						rnode->tty=xstrdup(bink?"binkp":"tcpip");
					} else {
						port=tty_findport(cfgsl(CFG_PORT),cfgs(CFG_NODIAL));
						if(!port){DEBUG(('Q',3,"nottyport"));goto nlkil;}
						rnode->tty=xstrdup(baseport(port));
					}
					dable=1;current->flv|=Q_DIAL;
					DEBUG(('Q',1,"forking %s",ftnaddrtoa(&current->addr)));
					chld=fork();
					if(!chld) {
						setsid();
#ifdef HAVE_DNOTIFY
						signal(SIGRTMIN,SIG_IGN);
#endif
						if(BSO)if(!bso_locknode(&current->addr,LCK_c))exit(S_BUSY);
						if(ASO)if(!aso_locknode(&current->addr,LCK_c))exit(S_BUSY);
						if(!is_ip&&cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
						log_done();ssock=uis_sock=lins_sock=-1;
						log_callback=NULL;xsend_cb=NULL;
						if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
							fprintf(stderr,"can't init log %s!\n",ccs);
							exit(S_BUSY);
						}
						DEBUG(('I',4,"connecting to daemon"));
						ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
						if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
						    else log_callback=vlogs;
						if(cfgs(CFG_RUNONCALL)) {
							snprintf(buf,MSG_BUFFER,"%s %s %s",ccs,ftnaddrtoa(&current->addr),is_ip?rnode->host:rnode->phone);
							if((rc=execsh(buf)))write_log("exec '%s' returned rc=%d",buf,rc);
						}
						if(is_ip)rc=tcp_call(rnode->host,&current->addr);
						    else {
							if(rnode->hidnum) {
								title("Calling %s #%d, %s",rnode->name,rnode->hidnum,ftnaddrtoa(&current->addr));
								write_log("calling %s #%d, %s (%s)",rnode->name,rnode->hidnum,ftnaddrtoa(&current->addr),rnode->phone);
							    } else {
								title("Calling %s, %s",rnode->name,ftnaddrtoa(&current->addr));
								write_log("calling %s, %s (%s)",rnode->name,ftnaddrtoa(&current->addr),rnode->phone);
							}
							rc=do_call(&current->addr,rnode->phone,port);
						}
						log_done();hld=0;
						if(!log_init(cfgs(CFG_MASTERLOG),NULL))fprintf(stderr,"can't init log %s.%s!\n",ccs,port);
						if(rc&S_ANYHOLD&&(rc&S_MASK)==S_OK) {
							if(BSO) {
								bso_getstatus(&current->addr,&sts);
								if(rc&S_HOLDA)sts.flags|=Q_WAITA;
								if(rc&S_HOLDR)sts.flags|=Q_WAITR;
								if(rc&S_HOLDX)sts.flags|=Q_WAITX;
								hld=cfgi(CFG_WAITHRQ);
								write_log("calls to %s delayed for %d min [%s]",ftnaddrtoa(&current->addr),hld,sts_str(sts.flags));
								sts.htime=t_set(hld*60);
								bso_setstatus(&current->addr,&sts);
							}
							if(ASO) {
								aso_getstatus(&current->addr,&sts);
								if(rc&S_HOLDA)sts.flags|=Q_WAITA;
								if(rc&S_HOLDR)sts.flags|=Q_WAITR;
								if(rc&S_HOLDX)sts.flags|=Q_WAITX;
								hld=cfgi(CFG_WAITHRQ);
								if(!BSO)write_log("calls to %s delayed for %d min [%s]",ftnaddrtoa(&current->addr),hld,sts_str(sts.flags));
								sts.htime=t_set(hld*60);
								aso_setstatus(&current->addr,&sts);
							}
						}
						if(rc!=S_BUSY)t_rescan=cfgi(CFG_RESCANPERIOD)-1;
						switch(rc&S_MASK) {
							case S_BUSY: break;
							case S_OK:
								if(BSO) {
									bso_getstatus(&current->addr,&sts);
									if (cfgi(CFG_HOLDONSUCCESS)) {
										hld=MAX(hld,cci);
										sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
										sts.htime=t_set(hld*60);
										write_log("calls to %s delayed for %d min after successful session",ftnaddrtoa(&current->addr),hld);
									}
									sts.try=0;
									bso_setstatus(&current->addr,&sts);
								}
								if(ASO) {
									aso_getstatus(&current->addr,&sts);
									if (cfgi(CFG_HOLDONSUCCESS)) {
										hld=MAX(hld,cci);
										sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
										sts.htime=t_set(hld*60);
										if(!BSO)write_log("ASO calls to %s delayed for %d min after successful session",ftnaddrtoa(&current->addr),hld);
									}
									sts.try=0;
									aso_setstatus(&current->addr,&sts);
								}
								break;
							case S_NODIAL:
								if((hld=cfgi(CFG_HOLDONNODIAL))!=0) {
									if(BSO) {
										uncurr=q_queue;
										while(!(!uncurr)) {
											bso_getstatus(&uncurr->addr,&sts);
											sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
											sts.htime=t_set(hld*60);
		    									bso_setstatus(&uncurr->addr,&sts);
											uncurr=uncurr->next;
										}
									}
									if(ASO) {
										uncurr=q_queue;
										while(!(!uncurr)) {
											aso_getstatus(&uncurr->addr,&sts);
											sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
											sts.htime=t_set(hld*60);
		    									aso_setstatus(&uncurr->addr,&sts);
											uncurr=uncurr->next;
										}
									}
									write_log("outgoing calls delayed for %d min after nodial",hld);
								}
								break;
							case S_FAILURE:
								if(BSO) {
									bso_getstatus(&current->addr,&sts);
									sts.flags|=Q_UNDIAL;
									sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
									write_log("fatal handshake error, %s undialable",ftnaddrtoa(&current->addr));
									bso_setstatus(&current->addr,&sts);
								}
								if(ASO) {
									aso_getstatus(&current->addr,&sts);
									sts.flags|=Q_UNDIAL;
									sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
									if(!BSO)write_log("fatal handshake error, %s undialable",ftnaddrtoa(&current->addr));
									aso_setstatus(&current->addr,&sts);
								}
								break;
							case S_REDIAL:
								if(!(rc&S_ADDTRY))break;
								if(BSO) {
									bso_getstatus(&current->addr,&sts);
									if(++sts.try>=cfgi(CFG_MAX_FAILS)) {
										sts.flags|=Q_UNDIAL;
										sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
										write_log("maximum tries count reached, %s undialable",
											ftnaddrtoa(&current->addr));
									} else if(cfgi(CFG_FAILS_HOLD_DIV))if(sts.try>=cci&&!(sts.try%cci)) {
										sts.flags|=Q_WAITA;
										sts.htime=t_set(cfgi(CFG_FAILS_HOLD_TIME)*60);
									}
									bso_setstatus(&current->addr,&sts);
								}
								if(ASO) {
									aso_getstatus(&current->addr,&sts);
									if(++sts.try>=cfgi(CFG_MAX_FAILS)) {
										sts.flags|=Q_UNDIAL;
										sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
										if(!BSO)write_log("maximum tries count reached, %s undialable",
											ftnaddrtoa(&current->addr));
									} else if(cfgi(CFG_FAILS_HOLD_DIV))if(sts.try>=cci&&!(sts.try%cci)) {
										sts.flags|=Q_WAITA;
										sts.htime=t_set(cfgi(CFG_FAILS_HOLD_TIME)*60);
									}
									aso_setstatus(&current->addr,&sts);
								}
								break;
						}
						if(BSO)bso_unlocknode(&current->addr,LCK_x);
						if(ASO)aso_unlocknode(&current->addr,LCK_x);
						vidle();log_done();
						cls_close(ssock);
						exit(rc);
					}
					if(chld<0) write_log("can't fork() caller!");
					c_delay=randper(cfgi(CFG_DIALDELAY),cfgi(CFG_DIALDELTA));
				} else current->flv&=~Q_DIAL;
nlkil:				is_ip=0;bink=0;
				nlkill(&rnode);
				DEBUG(('Q',1,"nlkill"));
				current=current->next;
				if(!current)current=q_queue;
				i=i->next;
			}
		}
		t=time(NULL);
		while(do_rescan!=1&&(time(NULL)-t)<1) {
			if((time(NULL)-t)<0)t=time(NULL);
			FD_ZERO(&rfds);
			FD_SET(lins_sock,&rfds);
			FD_SET(uis_sock,&rfds);
			rc=MAX(lins_sock,uis_sock);
			for(uis=cl;uis;uis=uis->next) {
				rc=MAX(rc,uis->sock);
				FD_SET(uis->sock,&rfds);
			}
			tv.tv_sec=0;tv.tv_usec=500000;
			rc=select(rc+1,&rfds,NULL,NULL,&tv);
			if(rc<0&&errno!=EINTR) {
				DEBUG(('I',1,"select: error: %s",strerror(errno)));
				usleep(1);
			}
			if(rc>0) {
				if(FD_ISSET(lins_sock,&rfds)) {
				  socklen_t salen=sizeof(sa);
/* nmap -sU >>> */		  if(recvfrom(lins_sock,buf,2,MSG_PEEK,&sa,&salen)<2)DEBUG(('I',1,"recvfrom: error: %s",strerror(errno)));
				    else {
					rc=xrecv(lins_sock,buf,MSG_BUFFER-1,1);
					if(rc<0)DEBUG(('I',3,"xrecv_l: %s",strerror(errno)));
					if(rc>0) {
						sa.sa_family=AF_INET;
						for(lins=ln,lnt=NULL;lins&&(memcmp(lins->addr,&sa,salen)||FETCH16(buf)!=lins->id);lnt=lins,lins=lins->next);
						if(!lins) {
							cls_ln_t *lt=NULL;lins=ln;
							while(lins) {
								if(kill(lins->id,0)<0) {
									char bf[MAX_STRING];
									cls_ln_t *lntt=lins;
									DEBUG(('I',2,"remove dead line %d",lins->id));
									STORE16(bf,lins->id);
									bf[2]=QC_ERASE;
									xstrcpy(bf+3,"ipline",8);
									daemon_xsend(ssock,bf,10);
									lins=lins->next;
									if(lt)lt->next=lins;
									if(lntt==ln)ln=lins;
									xfree(lntt->addr);
									xfree(lntt->emsi);
									xfree(lntt);
									continue;
								}
								lt=lins;
								lins=lins->next;
							}
							DEBUG(('I',2,"new line: pid=%d",FETCH16(buf)));
							lins=xmalloc(sizeof(cls_ln_t));
							lins->id=FETCH16(buf);
							lins->addr=xmalloc(salen);
							memcpy(lins->addr,&sa,salen);
							lins->emsi=NULL;
							lins->emsilen=0;
							lins->next=NULL;
							if(ln)lnt->next=lins;
							    else ln=lins;
						}
						DEBUG(('I',9,"lines_cl: recv %d bytes from %d",rc,lins->id));
					}
					if(rc>1)daemon_evt(lins_sock,buf,rc,0);
				    }
				}
				uis=cl;uit=NULL;
				while(uis) {
					if(FD_ISSET(uis->sock,&rfds)) {
						rc=xrecv(uis->sock,buf,MSG_BUFFER-1,1);
						if(!rc||(rc<0&&errno==ECONNRESET)) {
							cls_cl_t *uitt=uis;
							DEBUG(('I',1,"client %d: removed",uis->id));
							cls_close(uis->sock);
							if(uis->id+1==curr_id)curr_id--;
							xfree(uis->auth);
							uis=uis->next;
							if(uit)uit->next=uis;
							if(uitt==cl)cl=uis;
							if(!cl)curr_id=1;
							xfree(uitt);
							continue;
						}
						if(rc<0&&errno!=EAGAIN)DEBUG(('I',3,"xrecv_u %d: %s",uis->id,strerror(errno)));
						if(rc>0)DEBUG(('I',8,"client %d: recv %d bytes",uis->id,rc));
						if(rc>1)daemon_evt(uis->sock,buf,rc,1);
					}
					uit=uis;
					uis=uis->next;
				}
				if(FD_ISSET(uis_sock,&rfds)) {
					for(uis=cl;uis&&uis->next;uis=uis->next);
					uit=xmalloc(sizeof(cls_cl_t));
					uit->sock=accept(uis_sock,NULL,NULL);
					if(uit->sock<0) {
						DEBUG(('I',1,"client %d: accept err: %s",uit->id,strerror(errno)));
						xfree(uit);
					} else {
						if(curr_id<1)curr_id=33;
						uit->next=NULL;
						uit->id=curr_id++;
						uit->type='u';
						uit->auth=NULL;
						if(cl)uis->next=uit;
						    else cl=uit;
						DEBUG(('I',1,"new client %d: accepted (fd=%d)",uit->id,uit->sock));
						if(cfgs(CFG_SERVERPWD)) {
							long rnd=(long)random(),utm=time(NULL);
							int pid=((int)getpid())^((int)random());
							uit->auth=(unsigned char*)malloc(10);
							STORE32(uit->auth,rnd);
							STORE16(uit->auth+4,pid)
							STORE32(uit->auth+6,utm);
							xsend(uit->sock,(char*)uit->auth,10);
						} else xsend(uit->sock,"qs-noauth",10);
					}
				}
			}
		}
		t_dial++;t_rescan++;
		if(do_rescan)t_dial=0;
	}
}
