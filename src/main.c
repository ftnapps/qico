/**********************************************************
 * File: main.c
 * Created at Thu Jul 15 16:14:17 1999 by pk // aaz@ruxy.org.ru
 * qico main
 * $Id: main.c,v 1.62.4.1 2003/01/24 08:59:22 cyrilm Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#include <locale.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "tty.h"
#include "qipc.h"
#include "byteop.h"

#define IP_D 0

extern int hangup();

char *configname=CONFIG;
subst_t *psubsts;
int qipcr_msg;

void usage(char *ex)
{
	printf("%s-%s copyright (c) pavel kurnosoff, 1999-2000\n"
		   "usage: %s [<options>] [<node>] [<files>]\n"
 		   "<node>       must be in ftn-style (i.e. zone:net/node[.point])!\n" 
		   "-h           this help screen\n"
		   "-I<config>   override default config\n\n"  
		   "-d           start in daemon (originate) mode\n"
 		   "-a<type>     start in answer mode with <type> session, type can be:\n"
		   "                       auto - autodetect\n"
		   "             **EMSI_INQC816 - EMSI session without init phase\n"
		   "                      tsync - FTS-0001 session (unsuppported)\n"
		   "                     yoohoo - YOOHOO session (unsuppported)\n"
 		   "-i<host>     start TCP/IP connection to <host> (node must be specified!)\n"
		   "-c[N|IA]     force call to <node>\n"
		   "             N - normal call\n"
		   "             I - call <i>mmidiatly (don't check node worktime)\n"
		   "             A - call on <a>ny free port (don't check cancall setting)\n"
		   "             You could specify line after <node>, lines are numbered from 1\n"
		   "-n           compile nodelists\n"
		   "\n", progname, version, ex);
	exit(0);
}

void stopit(int rc)
{
	vidle();qqreset();
	write_log("exiting with rc=%d", rc);log_done();
#if IP_D	
	if(is_ip) qlerase();
#endif	
	qipc_done();
	exit(rc);
}

void sigerr(int sig)
{
	char *sigs[]={"","HUP","INT","QUIT","ILL","TRAP","IOT","BUS","FPE",
				  "KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM"};
	signal(sig, SIG_DFL);
	msgctl(qipcr_msg, IPC_RMID, 0);
	bso_done();
	write_log("got SIG%s signal",sigs[sig]);
#if IP_D	
	if(is_ip) qlerase();
#endif
	if(getpid()==islocked(cfgs(CFG_PIDFILE))) lunlink(ccs);
	log_done();
	tty_close();
	qqreset();sline("");title("");
	qipc_done();
	switch(sig) {
	case SIGSEGV:
	case SIGFPE:
	case SIGBUS:
		abort();
	default:
		exit(1);
	}
}

void sigchild(int sig)
{
	int rc, wr;
	signal(SIGCHLD, sigchild);
	wr=wait(&rc);
	if(wr<0) write_log("wait() returned %d (%d)", wr, rc);
	rc=WEXITSTATUS(rc)&S_MASK;
	if(rc==S_OK || rc==S_REDIAL) {
		do_rescan=1;
	}
}

void sighup(int sig)
{
	signal(SIGHUP, sighup);
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

void sendipcpkt(int wait, char what, pid_t pid, char *fmt, va_list args)
{
	char buf[MSG_BUFFER];
	int rc;
	STORE32(buf,pid);
	buf[4]=what;
#ifdef HAVE_VSNPRINTF
	rc=vsnprintf(buf+5, MSG_BUFFER-1, fmt, args);
#else
	/* to be replaced with some emulation vsnprintf!!! */
	rc=vsprintf(buf+5, fmt, args);
#endif	
	msgsnd(qipcr_msg, buf, rc+6, wait?0:IPC_NOWAIT);
}

void sendrpkt(char what, pid_t pid, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	sendipcpkt(0,what,pid,fmt,args);
	va_end(args);
}

void sendrpktwait(char what, pid_t pid, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	sendipcpkt(1,what,pid,fmt,args);
	va_end(args);
}


char qchars[]=Q_CHARS;
char *sts_str(int flags)
{
	static char s[Q_MAXBIT+1];int i;
	for(i=0;i<Q_MAXBIT;i++) s[i]=(flags&(1<<i))?qchars[i]:'.';
	s[Q_MAXBIT]=0;
	return s;
}

int randper(int base, int diff)
{
	return base-diff+(int)(diff*2.0*rand()/(RAND_MAX+1.0));
}

void to_dev_null() 
{
	int fd;

	close(STDIN_FILENO);close(STDOUT_FILENO);close(STDERR_FILENO);

	fd=open(devnull,O_RDONLY);
	if(dup2(fd,STDIN_FILENO)!=STDIN_FILENO) { write_log("reopening of stdin failed");exit(-1); }
	if(fd!=STDIN_FILENO) close(fd);

	fd=open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600);
	if(dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO) { write_log("reopening of stdout failed");exit(-1); }
	if(fd!=STDOUT_FILENO) close(fd);

	fd=open(devnull,O_WRONLY | O_APPEND | O_CREAT,0600);
	if(dup2(fd,STDERR_FILENO)!=STDERR_FILENO) { write_log("reopening of stderr failed");exit(-1); }
	if(fd!=STDERR_FILENO) close(fd);
}


void daemon_mode()
{
	int t_dial=0, c_delay, t_rescan=0; 
	int rc=1, dable, f, w, res, set;
	char *port, buf[MSG_BUFFER], *p;
	sts_t sts;
	pid_t chld;
	qitem_t *current=q_queue, *i;
	qitem_t *sinfo;
	key_t qipcr_key;
	time_t t;
	ftnaddr_t fa;
	slist_t *sl;
	int mailonly, hld;


	/* Change our root, if we are asked for */ 
	if(cfgs(CFG_ROOTDIR) && ccs[0]) chdir(ccs);

	if(getppid()!=1) {
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		if((rc=fork())>0) 
			exit(0);
		if(rc<0) {
			write_log("can't spawn daemon!");
			exit(1);
		}
	}

	signal(SIGINT, sigerr);	
	signal(SIGTERM, sigerr);
	signal(SIGFPE, sigerr);
	signal(SIGCHLD, sigchild);
	signal(SIGHUP, sighup);

	if(cfgs(CFG_PIDFILE)) {
		if(!lockpid(ccs)) {
			write_log("another daemon exists or can't create pid file!");
			exit(1);
		}
	}
	if((qipcr_key=ftok(QIPC_KEY,QR_MSGQ))<0) {
		write_log("can't get key");
		exit(1);
	}
	if((qipcr_msg=msgget(qipcr_key, cfgi(CFG_IPCPERM) | IPC_CREAT | IPC_EXCL))<0) {
		write_log("can't create message queue (may be another daemon is running?)");
		exit(1);
	}

	if(!bso_init(cfgs(CFG_OUTBOUND), cfgal(CFG_ADDRESS)->addr.z)) {
		write_log("can't init BSO");
		exit(1);
	}

	if(!log_init(cfgs(CFG_MASTERLOG),NULL)) {
		write_log("can't open master log %s!", ccs);
		exit(1);
	}
	/* we could detach from terminal -- log system is Ok */
	to_dev_null();
	setsid();
	/* No terninal below this line */
	write_log("%s-%s/%s daemon started",progname,version,osname);
	t_rescan=cfgi(CFG_RESCANPERIOD);
	srand(time(NULL));
	c_delay=randper(cfgi(CFG_DIALDELAY),cfgi(CFG_DIALDELTA));
	while(1) {
		title("Queue manager [%d]", cfgi(CFG_RESCANPERIOD)-t_rescan);
		if(t_rescan>=cci || do_rescan) {
			do_rescan=0;
			sline("Rescanning outbound...");
			if(!q_rescan(&current)) 
				write_log("can't rescan outbound %s!", cfgs(CFG_OUTBOUND));
			t_rescan=0;
		}
		sline("Waiting %d...", c_delay-t_dial);
		if(t_dial>=c_delay) {
			c_delay=randper(cfgi(CFG_DIALDELAY),cfgi(CFG_DIALDELTA));
			t_dial=0;
			dable=0;
			mailonly=checktimegaps(cfgs(CFG_MAILONLY))||checktimegaps(cfgs(CFG_ZMH));

			port=tty_findport(cfgsl(CFG_PORT),cfgs(CFG_NODIAL));			
			if(!port || !q_queue) dable=1;
			i=q_queue;
			DEBUG(('Q',1,"dabl"));
			while(!dable && i) {
				f=current->flv;
				w=current->what;
				if(f&Q_ANYWAIT) 
					if(t_exp(current->onhold)) {
						current->onhold=0;
						f&=~Q_ANYWAIT;
					}
				if(f&Q_UNDIAL && cfgi(CFG_CLEARUNDIAL)!=0) {
					bso_getstatus(&current->addr, &sts);
					if (t_exp(sts.utime)) {
						sts.flags&=~Q_UNDIAL;
						sts.try=0;
						sts.utime=0;
						bso_setstatus(&current->addr, &sts);
						f&=~Q_UNDIAL;
						write_log("changing status of %s to [%s]",
							ftnaddrtoa(&current->addr), sts_str(sts.flags));
					}
				}
 
				if(falist_find(cfgal(CFG_ADDRESS), &current->addr) ||
					f&Q_UNDIAL ||
					!havestatus(f,CFG_CALLONFLAVORS) ||
					needhold(f,w) ||
					(mailonly && !current->pkts)) {
					current=current->next;
					if(!current) current=q_queue;
					i=i->next;
					continue;
				}
				DEBUG(('Q',1,"quering"));
				rc=query_nodelist(&current->addr,cfgs(CFG_NLPATH),&rnode);
				DEBUG(('Q',1,"querynl"));
				switch(rc) {
				case 1:write_log("can't query nodelist, index error");break;
				case 2:write_log("can't query nodelist, nodelist error");break;
				case 3:write_log("index is older than the list, need recompile");break;
				}
				if(!rnode) {
					rnode=xcalloc(1,sizeof(ninfo_t));
					falist_add(&rnode->addrs, &current->addr);
					rnode->name=xstrdup("Unknown");
					rnode->phone=xstrdup("");
				}
				phonetrans(&rnode->phone, cfgsl(CFG_PHONETR));
				DEBUG(('Q',1,"%s %s %s [%d]", ftnaddrtoa(&current->addr),
					rnode?rnode->phone:"$",rnode->haswtime?rnode->wtime:"$",rnode->hidnum));
				rnode->tty=xstrdup(baseport(port));
				if(checktimegaps(cfgs(CFG_CANCALL)) &&
					find_dialable_subst(rnode, havestatus(f,CFG_IMMONFLAVORS), psubsts)) {
					dable=1;current->flv|=Q_DIAL;
					chld=fork();
					DEBUG(('Q',1,"forking %s",ftnaddrtoa(&current->addr)));
					
					if(chld==0) {
						setsid();
						if(!bso_locknode(&current->addr)) exit(S_BUSY);
						log_done();
						if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
							fprintf(stderr, "can't init log %s!",ccs);
							exit(S_BUSY);
						}
						if(rnode->hidnum) {
							title("Calling %s #%d, %s",
								  rnode->name, rnode->hidnum,
								  ftnaddrtoa(&current->addr));
							write_log("calling %s #%d, %s (%s)", rnode->name, rnode->hidnum,	
								ftnaddrtoa(&current->addr),
								rnode->phone);
						} else {								
							title("Calling %s, %s",
								  rnode->name, ftnaddrtoa(&current->addr));
							write_log("calling %s, %s (%s)", rnode->name,
								ftnaddrtoa(&current->addr),
								rnode->phone);
						}								
						rc=do_call(&current->addr, rnode->phone,
								   port);
						log_done();
							
						if(!log_init(cfgs(CFG_MASTERLOG),NULL)) {
							fprintf(stderr, "can't init log %s.%s!",
									ccs, port);
						}
							
						hld=0;
						if(rc&S_ANYHOLD) {
							bso_getstatus(&current->addr, &sts);
							if(rc&S_HOLDA) sts.flags|=Q_WAITA; 
							if(rc&S_HOLDR) sts.flags|=Q_WAITR;
							if(rc&S_HOLDX) sts.flags|=Q_WAITX;
							hld=cfgi(CFG_WAITHRQ);
							write_log("calls to %s delayed for %d min %s",
								ftnaddrtoa(&current->addr), hld, sts_str(sts.flags));
							sts.htime=t_set(hld*60);
							bso_setstatus(&current->addr, &sts);
						}
						if(rc!=S_BUSY) t_rescan=cfgi(CFG_RESCANPERIOD)-1;
						switch(rc&S_MASK) {
						case S_BUSY: break;
						case S_OK:
							bso_getstatus(&current->addr, &sts);
							if (cfgi(CFG_HOLDONSUCCESS)) {
								hld=MAX(hld,cci);
								sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
								sts.htime=t_set(hld*60);
								write_log("calls to %s delayed for %d min after successuful session",
									ftnaddrtoa(&current->addr), hld);
							}
							sts.try=0;
							bso_setstatus(&current->addr, &sts);
							break;
						case S_UNDIAL:
							bso_getstatus(&current->addr, &sts);
							sts.flags|=Q_UNDIAL;
							sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
							bso_setstatus(&current->addr, &sts);
							break;
						case S_REDIAL:
							bso_getstatus(&current->addr, &sts);
							if(++sts.try>=cfgi(CFG_MAX_FAILS)) {
								sts.flags|=Q_UNDIAL;
								sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
								write_log("maximum tries count reached, %s undialable",
									ftnaddrtoa(&current->addr));
							}
							bso_setstatus(&current->addr, &sts);
							break;
						}
						bso_unlocknode(&current->addr);
						vidle();log_done();
/* 						qipc_done(); */
						exit(rc);
					}
					if(chld<0) write_log("can't fork() caller!");
				} else current->flv&=~Q_DIAL;
				nlkill(&rnode);
				DEBUG(('Q',1,"nlkill"));
				current=current->next;
				if(!current) current=q_queue;
				i=i->next;
			} 
		}
		t=time(NULL);
		while((time(NULL)-t)<1) {
			rc=msgrcv(qipcr_msg, buf, MSG_BUFFER-1, 1, IPC_NOWAIT);
			if(rc<=0) {
				struct timeval tv = {0,200};
				select(0,NULL,NULL,NULL,&tv);
			} else
			if(rc>=5) {
				chld=*((int *)buf+1);
				rc=1;
				if(buf[8]==QR_POLL || buf[8]==QR_REQ ||
				   buf[8]==QR_INFO || buf[8]==QR_SEND ||
				   buf[8]==QR_STS || buf[8]==QR_KILL) {
					if(!parseftnaddr(buf+9, &fa, &DEFADDR, 0)) {
						write_log("can't parse address '%s'!", buf+9);
						sendrpkt(1,chld,"can't parse address '%s'!", buf+9);
						rc=0;
					}
				}
				if(rc) switch(buf[8]) {
				case QR_QUIT:
					sendrpkt(0,chld,"");
					msgctl(qipcr_msg, IPC_RMID, 0);
					bso_done();
					write_log("exiting by request");
#if IP_D	
					if(is_ip) qlerase();
#endif
					if(cfgs(CFG_PIDFILE)) {
						if(getpid()==islocked(ccs)) lunlink(ccs);
					}
					log_done();
					qqreset();sline("");title("");
					qipc_done();
					exit(0);
				case QR_CONF:
					write_log("trying to reread configs by request...");
					killsubsts(&psubsts);
					killconfig();
					if(!readconfig(configname)) {
						write_log("there was some errors parsing config, exiting");
						sendrpkt(1,chld,"bad config, terminated");
						stopit(0);
					}
					psubsts=parsesubsts(cfgfasl(CFG_SUBST));
#ifdef NEED_DEBUG
					parse_log_levels();
#endif
					do_rescan=1;					
					sendrpkt(0,chld,"");
					break;
				case QR_SCAN:
					do_rescan=1;					
					sendrpkt(0,chld,"");
					break;
				case QR_POLL:
					if(bso_locknode(&fa)) {
						p=buf+9+strlen(buf+9)+1;
						if('?'==*p) *p=*cfgs(CFG_POLLFLAVOR);
						rc=bso_flavor(*p);
						if(rc==F_ERR) {
							write_log("unknown flavour - '%c'",C0(*p));
							sendrpkt(1,chld,"unknown flavour %c",C0(*p));
							break;
						}

						write_log("poll for %s, flavor %c", ftnaddrtoa(&fa),*p);
						sendrpkt(0,chld,"");
						rc=bso_poll(&fa,rc);
						bso_unlocknode(&fa);
						do_rescan=1;
					} else {
						write_log("can't create poll for %s!",
							ftnaddrtoa(&fa));
						sendrpkt(1,chld,"can't create poll for %s",
								 ftnaddrtoa(&fa));
					}
					break;
				case QR_KILL:
					if(bso_locknode(&fa)) {
						write_log("kill %s", ftnaddrtoa(&fa));
						sendrpkt(0,chld,"");
						simulate_send(&fa);
						bso_unlocknode(&fa);
						do_rescan=1;
					} else {
						write_log("can't kill %s!",
							ftnaddrtoa(&fa));
						sendrpkt(1,chld,"can't kill %s",
								 ftnaddrtoa(&fa));
					}
					break;
				case QR_STS:
					p=buf+9+strlen(buf+9)+1;
					rc=1;res=0;set=0;
					while(*p && rc) {
						switch(*p) {
						case 'W': set|=Q_WAITA;break;
						case 'I': set|=Q_IMM;break;
						case 'U': set|=Q_UNDIAL;break;
						case 'w': res|=Q_ANYWAIT;break;
						case 'i': res|=Q_IMM;break;
						case 'u': res|=Q_UNDIAL;break;
						default:
							write_log("unknown status action: %c", *p);
							sendrpkt(1,chld,
									 "unknown status action: %c", *p);
							rc=0;
						}
						p++;
					}
					if(rc) {
						bso_getstatus(&fa, &sts);
						sts.flags|=set;sts.flags&=~res;
						write_log("changing status of %s to [%s]",
							ftnaddrtoa(&fa), sts_str(sts.flags));
						if(set&Q_WAITA && !(res&Q_ANYWAIT))
							sts.htime=t_set(cfgi(CFG_WAITHRQ)*60);
						if(set&Q_UNDIAL) sts.utime=t_set(cfgi(CFG_CLEARUNDIAL)*60);
						if(res&Q_UNDIAL) sts.try=0;
						bso_setstatus(&fa, &sts);
						sendrpkt(0,chld,"");
						do_rescan=1;
					}
					break;
				case QR_REQ:
					if(bso_locknode(&fa)) {
						sl=NULL;p=buf+9+strlen(buf+9)+1;
						while(strlen(p)){
							write_log("requested '%s' from %s",p,
								ftnaddrtoa(&fa));
							slist_add(&sl, p);
							p+=strlen(p)+1;
						}
						rc=bso_request(&fa, sl);
						slist_kill(&sl);
						bso_unlocknode(&fa);
						sendrpkt(0,chld,"");
						do_rescan=1;
					} else {
						write_log("can't lock node %s!",
							ftnaddrtoa(&fa));
						sendrpkt(1,chld,"can't lock node %s",
								 ftnaddrtoa(&fa));
					}
					break;
				case QR_SEND:
					p=buf+9+strlen(buf+9)+1;
					if('?'==*p) *p=*cfgs(CFG_POLLFLAVOR);
					rc=bso_flavor(*p);
					if(rc==F_ERR) {
						write_log("unknown flavour - '%c'",C0(*p));
						sendrpkt(1,chld,"unknown flavour %c",C0(*p));
						break;
					}
					p+=strlen(p)+1;
					if(bso_locknode(&fa)) {
						sl=NULL;
						while(strlen(p)){
							write_log("attaching '%s' to %s%s",
								(p[0]=='^')?p+1:p,
								ftnaddrtoa(&fa),
								(p[0]=='^')?" (k/s)":"");
							slist_add(&sl, p);
							p+=strlen(p)+1;
						}
						rc=bso_attach(&fa, rc, sl);
						slist_kill(&sl);
						bso_unlocknode(&fa);
						sendrpkt(0,chld,"");
						do_rescan=1;
					} else {
						write_log("can't lock node %s!",
							ftnaddrtoa(&fa));
						sendrpkt(1,chld,"can't lock node %s",
								 ftnaddrtoa(&fa));
					}
					break;
				case QR_INFO:
					rc=query_nodelist(&fa,cfgs(CFG_NLPATH),&rnode);
					switch(rc) {
					case 0:
						if(rnode) {
							write_log("returned info about %s (%s)",rnode->name, ftnaddrtoa(&fa));
							sendrpkt(0, chld, "%s%c%s%c%s%c%s%c%s%c%s%c%d%c",
									 ftnaddrtoa(&fa), 0,
									 rnode->name, 0, rnode->place, 0,
									 rnode->sysop, 0, rnode->phone, 0,
									 rnode->flags, 0, rnode->speed, 0
								);
							nlkill(&rnode);
						} else {
							write_log("%s not found in nodelist!",ftnaddrtoa(&fa));
							sendrpkt(1, chld, "%s not found in nodelist!",ftnaddrtoa(&fa));
						}
						break;
					case 1:
						write_log("can't query nodelist, index error");
						sendrpkt(1, chld, "can't query nodelist, index error");
						break;
					case 2:
						write_log("can't query nodelist, nodelist error");
						sendrpkt(1, chld, "can't query nodelist, nodelist error");
						break;
					case 3:
						write_log("index is older than the list, need recompile");
						sendrpkt(1, chld, "index is older than the list, need recompile");
						break;
					default:
						write_log("nodelist query error!");
						sendrpkt(1, chld, "nodelist query error!");
						break;
					}
					break;
				case QR_QUEUE:
					sinfo = q_queue;
					if (sinfo) do {
						sendrpktwait(0,chld,"%c%s%c%lu%c%lu%c%lu%c%lu",
							(char)1,
							ftnaddrtoa(&sinfo->addr),(char)0,
							(unsigned long)sinfo->pkts,(char)0,
							(unsigned long)(q_sum(sinfo)+sinfo->reqs),(char)0,
							(unsigned long)sinfo->try,(char)0,
							(unsigned long)sinfo->flv);
					} while ((sinfo = sinfo->next));
					sendrpktwait(0,chld,"%c",0);
					break;
				default:
					write_log("got unsupported packet type: %c", C0(buf[8]));
				}
			}
		}
		t_dial++;t_rescan++;
		if(do_rescan) t_dial=0;
	}
}

void killdaemon(int sig)
{
	FILE *f;
	long pid;
	if(!cfgs(CFG_PIDFILE)) {
		fprintf(stderr, "no pidfile defined\n");
		return;
	}
	f=fopen(cfgs(CFG_PIDFILE), "rt");
	if(!f) {
		fprintf(stderr, "can't open pid file - no daemon killed!\n");
		return;
	}		
	fscanf(f, "%ld", &pid);fclose(f);
	if(kill(pid, sig))
		fprintf(stderr, "can't send signal!\n");
	else
		fprintf(stderr, "ok!\n");
}

void getsysinfo()
{
	struct utsname uts;
	char tmp[MAX_STRING];
	if(uname(&uts)) return;
	snprintf(tmp, MAX_STRING, "%s-%s (%s)", uts.sysname, uts.release, uts.machine);
	osname=xstrdup(tmp);
}


void answer_mode(int type)
{
	int rc, spd;char *cs;
	struct sockaddr_in sa;int ss=sizeof(sa);
	sts_t sts;

	/* Change our root, if we are asked for */ 
	if(cfgs(CFG_ROOTDIR) && ccs[0]) chdir(ccs);

	rnode=xcalloc(1, sizeof(ninfo_t));
	is_ip=!isatty(0);
#if IP_D	
	snprintf(ip_id, 10, "ip%d", getpid());
#else
	xstrcpy(ip_id, "ipd", 10);
#endif
	rnode->tty=xstrdup(is_ip?"tcpip":basename(ttyname(0)));
	rnode->options|=O_INB;
	if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
		printf("can't open log %s!\n", ccs);
		exit(0);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, sigerr);
	signal(SIGSEGV, sigerr);
	signal(SIGFPE, sigerr);
	
	if(!bso_init(cfgs(CFG_OUTBOUND), cfgal(CFG_ADDRESS)->addr.z)) {
		write_log("can't init BSO");stopit(1);
	}

	write_log("answering incoming call");vidle();
	if(is_ip && !getpeername(0,(struct sockaddr *)&sa,&ss)) {
		write_log("remote is %s", inet_ntoa(sa.sin_addr));
		spd=TCP_SPEED;
	} else {	
		cs=getenv("CONNECT");spd=cs?atoi(cs):0;
		if(cs && spd) {
			write_log("*** CONNECT %s", cs);
		} else {
			write_log("*** CONNECT Unknown");spd=300;
		}
	}
	if((cs=getenv("CALLER_ID")) && strcasecmp(cs,"none")) write_log("caller-id: %s", cs);
	tty_setattr(0);
	tty_nolocal();
	rc=session(0, type, NULL, spd);
	if(!is_ip) hangup();
	tty_cooked();

	if ((S_OK == (rc&S_MASK)) && cfgi(CFG_HOLDONSUCCESS)) {
		log_done();
		log_init(cfgs(CFG_MASTERLOG),NULL);

		bso_getstatus(&rnode->addrs->addr, &sts);
		sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
		sts.htime=MAX(t_set(cci*60),sts.htime);
		write_log("calls to %s delayed for %d min after successuful incoming session",
					ftnaddrtoa(&rnode->addrs->addr), cci);
		bso_setstatus(&rnode->addrs->addr, &sts);

		log_done();
		log_init(cfgs(CFG_LOG),rnode->tty);
	}

	title("Waiting...");
	vidle();
	sline("");
	bso_done();
	stopit(rc);
}	

int force_call(ftnaddr_t *fa, int line, int flags)
{
	char *port=NULL;
	slist_t *ports=NULL;
	int rc;

	rc=query_nodelist(fa,cfgs(CFG_NLPATH),&rnode);
	switch(rc) {
	case 1:write_log("can't query nodelist, index error");break;
	case 2:write_log("can't query nodelist, nodelist error");break;
	case 3:write_log("index is older than the list, need recompile");break;
	}
	if(!rnode) {
		rnode=xcalloc(1,sizeof(ninfo_t));
		falist_add(&rnode->addrs, fa);
		rnode->name=xstrdup("Unknown");
		rnode->phone=xstrdup("");
	}
	phonetrans(&rnode->phone, cfgsl(CFG_PHONETR));
	rnode->tty=NULL;

	if((flags & 2) != 2) {
		ports=cfgsl(CFG_PORT);
		do {
			if(!ports) exit(33);
			port=tty_findport(ports,cfgs(CFG_NODIAL));
			if(!port) exit(33);
			if(rnode->tty) xfree(rnode->tty);
			rnode->tty=xstrdup(baseport(port));
			ports=ports->next;
		} while(!checktimegaps(cfgs(CFG_CANCALL)));
		if(!checktimegaps(cfgs(CFG_CANCALL))) exit(33);
	} else {
		if((port=tty_findport(cfgsl(CFG_PORT),cfgs(CFG_NODIAL)))) {
			rnode->tty=xstrdup(baseport(port));
		} else {
			exit(33);
		}
	}

	if(line) {
		applysubst(rnode, psubsts);
		while(rnode->hidnum && line != rnode->hidnum && applysubst(rnode, psubsts) && rnode->hidnum != 1);
		if(line != rnode->hidnum) {
			fprintf(stderr,"%s doesn't have line #%d\n",ftnaddrtoa(fa),line);
			exit(0);
		}
		if(!can_dial(rnode,(flags & 1) == 1)) {
			fprintf(stderr,"We should not call to %s #%d at this time\n",ftnaddrtoa(fa),line);
			exit(0);
		}
	} else {
		if (!find_dialable_subst(rnode, ((flags & 1) == 1), psubsts)) {
			fprintf(stderr,"We should not call to %s at this time\n",ftnaddrtoa(fa));
			exit(0);
		}
	}

	if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
		printf("can't open log %s!\n", ccs);
		exit(0);
	}

	if(rnode->hidnum) {
		write_log("calling %s #%d, %s (%s)", rnode->name, rnode->hidnum,ftnaddrtoa(fa),rnode->phone);
	} else {
		write_log("calling %s, %s (%s)", rnode->name,ftnaddrtoa(fa),rnode->phone);
	}

	rc=do_call(fa, rnode->phone,port);
	return rc;
}
                                                                              

int main(int argc, char *argv[], char *envp[])
{
	int c, daemon=-1, rc, sesstype=SESSION_AUTO, line = 0;
	char *hostname=NULL;
	ftnaddr_t fa;
	int call_flags = 0;
	char *str=NULL;

#ifndef HAVE_SETPROCTITLE
	setargspace(argc,argv,envp);
#endif
 	setlocale(LC_ALL, "");	 

	while((c=getopt(argc, argv, "hI:da:ni:c:"))!=EOF) {
		switch(c) {
		case 'c':                                                       
			daemon=12;
			str=optarg;
			while(str && *str) {
				switch(toupper(*str)) {
				case 'N': call_flags=0; break;
				case 'I': call_flags|=1; break;
				case 'A': call_flags|=2; break;
				default: write_log("unknown call option: %c", *optarg);exit(0);
				}
				str++;
			}
			break;
		case 'i':
			hostname=optarg;
			break;
		case 'h':
			usage(argv[0]);
		case 'I':
			configname=optarg;
			break;
		case 'd':
			daemon=1;
			break;
		case 'a':
			daemon=0;
			sesstype=SESSION_AUTO;
			if(!strncasecmp(optarg, "**emsi", 6)) sesstype=SESSION_EMSI;
			if(!strncasecmp(optarg, "tsync", 5)) sesstype=SESSION_FTS0001;
			if(!strncasecmp(optarg, "yoohoo", 6)) sesstype=SESSION_YOOHOO;
			break;
		case 'n':
			daemon=2;
			break;
		}
	}

	if(!hostname && daemon<0) usage(argv[0]);

	getsysinfo();
	if(!readconfig(configname)) {
		write_log("there was some errors parsing %s, aborting",
				configname);
		exit(EXC_BADCONFIG);
	}

#ifdef NEED_DEBUG
	parse_log_levels();
	if (facilities_levels['C'] >= 1) dumpconfig();
#endif	
    psubsts=parsesubsts(cfgfasl(CFG_SUBST));
#ifdef NEED_DEBUG
	if (facilities_levels['C'] >= 1) {
		subst_t *s;
		dialine_t *l;
		for(s=psubsts;s;s=s->next) {
			printf("subst for %s [%d]\n", ftnaddrtoa(&s->addr), s->nhids);
			for(l=s->hiddens;l;l=l->next)
				printf(" * %s,%s,%d\n",l->phone,l->timegaps,l->num);
		}
		printf("...press any key...\n");getchar();
	}
#endif	

	if(!qipc_init()) write_log("can't create ipc key!");

	if(hostname || daemon==12) {
		if(!parseftnaddr(argv[optind], &fa, &DEFADDR, 0)) {
			write_log("%s: can't parse address '%s'!", argv[0], argv[optind]);
			exit(1);
		}
		optind++;
	}

	if(hostname) {
		is_ip=1;
		rnode=xcalloc(1,sizeof(ninfo_t));
#if IP_D	
		snprintf(ip_id, 10, "ip%d", getpid());
#else
		xstrcpy(ip_id, "ipd", 10);
#endif
		rnode->tty="tcpip";
		if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
			write_log("can't open log %s!", ccs);
			exit(1);
		}
		signal(SIGINT, sigerr);
		signal(SIGTERM, sigerr);
		signal(SIGSEGV, sigerr);
		
		if(!bso_init(cfgs(CFG_OUTBOUND), cfgal(CFG_ADDRESS)->addr.z)) {
			write_log("can't init BSO");stopit(1);
		}
		tcp_call(hostname, &fa);
		
		bso_done();
		stopit(0);
	}

	if(daemon==12) {
		if(optind<argc) {
			if(1!=sscanf(argv[optind],"%d",&line)) {
				write_log("%s: can't parse line number '%s'!\n", argv[0], argv[optind]);
				exit(1);
			}
		} else {
			line = 0;
		}
		if(!bso_init(cfgs(CFG_OUTBOUND), cfgal(CFG_ADDRESS)->addr.z)) {
			write_log("%s: can't init bso!", argv[0]);
			exit(1);
		}
		if (bso_locknode(&fa)) {
			signal(SIGINT, sigerr);
			signal(SIGTERM, sigerr);
			signal(SIGSEGV, sigerr);
			rc=force_call(&fa,line,call_flags);
			bso_unlocknode(&fa);
		} else rc=0;
		if(rc&S_MASK) write_log("%s: can't call to %s", argv[0],ftnaddrtoa(&fa));
		bso_done();
		stopit(rc);
	}
	switch(daemon) {
	case 1:
		daemon_mode();break;
	case 0:
		answer_mode(sesstype);break;
	case 2:
		compile_nodelists();break;
	}
	
	return 0;
}
