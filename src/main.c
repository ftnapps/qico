/**********************************************************
 * qico main
 * $Id: main.c,v 1.28 2004/05/17 22:29:04 sisoft Exp $
 **********************************************************/
#include "headers.h"
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <sys/utsname.h>
#include "clserv.h"
#include "tty.h"

char *sigs[]={"","HUP","INT","QUIT","ILL","TRAP","IOT","BUS","FPE","KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM"};

static void usage(char *ex)
{
	printf("usage: %s [<options>] [<node>]\n"
		"<node>       must be in ftn-style (i.e. zone:net/node[.point])!\n",ex);
	puts(	"-h           this help screen\n"
		"-I<config>   override default config\n"
		"-d           start in daemon (originate) mode\n"
 		"-a<type>     start in answer mode with <type> session, type can be:\n"
		"                       auto - autodetect\n"
		"             **EMSI_INQC816 - EMSI session without init phase\n"
#ifdef WITH_BINKP
		"                      binkp - Binkp session\n"
#endif
		"                      tsync - FTS-0001 session (unsuppported)\n"
		"                     yoohoo - YOOHOO session (unsuppported)");
	puts(	"-i<host>     start TCP/IP connection to <host> (node must be specified!)\n"
		"-c[N|I|A]    force call to <node>\n"
		"             N - normal call\n"
		"             I - call <i>mmediately (don't check node worktime)\n"
		"             A - call on <a>ny free port (don't check cancall setting)\n"
		"             You could specify line after <node>, lines are numbered from 1\n"
#ifdef WITH_BINKP
		"-b           call with Binkp (default call ifcico)\n"
#endif
		"-n           compile nodelists\n"
		"-t           check config file for errors\n"
                "-v           show version\n");
	exit(0);
}

void stopit(int rc)
{
	vidle();qqreset();
	write_log("exiting with rc=%d",rc);
	log_done();
	cls_close(ssock);
	cls_shutd(lins_sock);
	cls_shutd(uis_sock);
	exit(rc);
}

RETSIGTYPE sigerr(int sig)
{
	signal(sig,SIG_DFL);
	if(BSO)bso_done();
	if(ASO)aso_done();
	write_log("got SIG%s signal",sigs[sig]);
	if(cfgs(CFG_PIDFILE))if(getpid()==islocked(ccs))lunlink(ccs);
	log_done();
	tty_close();
	qqreset();sline("");title("");
	cls_close(ssock);
	cls_shutd(lins_sock);
	cls_shutd(uis_sock);
	switch(sig) {
	    case SIGSEGV:
	    case SIGFPE:
	    case SIGBUS:
	    case SIGABRT:
		abort();
	    default:
		exit(1);
	}
}

static void getsysinfo()
{
	struct utsname uts;
	char tmp[MAX_STRING+1];
	if(uname(&uts)) return;
	snprintf(tmp,MAX_STRING,"%s-%s (%s)",uts.sysname,uts.release,uts.machine);
	osname=xstrdup(tmp);
}

static void answer_mode(int type)
{
	int rc, spd;char *cs;
	struct sockaddr_in sa;
	socklen_t ss=sizeof(sa);
	sts_t sts;
	if(cfgs(CFG_ROOTDIR)&&ccs[0])chdir(ccs);
	rnode=xcalloc(1,sizeof(ninfo_t));
	is_ip=!isatty(0);
	xstrcpy(ip_id,"ipline",10);
	rnode->tty=xstrdup(is_ip?(bink?"binkp":"tcpip"):basename(ttyname(0)));
	rnode->options|=O_INB;
	if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
		printf("can't open log %s!\n",ccs);
		exit(0);
	}
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,sigerr);
	signal(SIGSEGV,sigerr);
	signal(SIGFPE,sigerr);
	signal(SIGPIPE,SIG_IGN);

	log_callback=NULL;xsend_cb=NULL;
	ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
	if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
	    else log_callback=vlogs;

	if(!bso_init(cfgs(CFG_BSOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init BSO");
	if(!aso_init(cfgs(CFG_ASOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init ASO");
	if(!BSO&&!ASO) {
		write_log("No outbound defined");
		stopit(1);
	}

	write_log("answering incoming call");vidle();
	if(is_ip&&!getpeername(0,(struct sockaddr*)&sa,&ss)) {
		write_log("remote is %s",inet_ntoa(sa.sin_addr));
		spd=TCP_SPEED;
	} else {
		cs=getenv("CONNECT");spd=cs?atoi(cs):0;
		xfree(connstr);connstr=xstrdup(cs);
		if(cs&&spd)write_log("*** CONNECT %s",cs);
		    else {
			write_log("*** CONNECT Unknown");
			spd=DEFAULT_SPEED;
		}
	}
	if((cs=getenv("CALLER_ID"))&&strcasecmp(cs,"none")&&strlen(cs)>3)write_log("caller-id: %s",cs);
	tty_setattr(0);
	tty_nolocal();
	rc=session(0,type,NULL,spd);
	tty_local();
	if(!is_ip&&!bink) {
		hangup();
		stat_collect();
	}
	tty_cooked();
	if((S_OK==(rc&S_MASK))&&cfgi(CFG_HOLDONSUCCESS)) {
		log_done();
		log_init(cfgs(CFG_MASTERLOG),NULL);
		if(BSO) {
			bso_getstatus(&rnode->addrs->addr, &sts);
			sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
			sts.htime=MAX(t_set(cci*60),sts.htime);
			write_log("calls to %s delayed for %d min after successful incoming session",
					ftnaddrtoa(&rnode->addrs->addr),cci);
			bso_setstatus(&rnode->addrs->addr,&sts);
		}
		if(ASO) {
			aso_getstatus(&rnode->addrs->addr,&sts);
			sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
			sts.htime=MAX(t_set(cci*60),sts.htime);
			if(!BSO)write_log("calls to %s delayed for %d min after successful incoming session",
					ftnaddrtoa(&rnode->addrs->addr),cci);
			aso_setstatus(&rnode->addrs->addr,&sts);
		}
		log_done();
		log_init(cfgs(CFG_LOG),rnode->tty);
	}

	title("Waiting...");
	vidle();sline("");
	if(BSO)bso_done();
	if(ASO)aso_done();
	stopit(rc);
}

static int force_call(ftnaddr_t *fa,int line,int flags)
{
	int rc;
	char *port=NULL;
	slist_t *ports=NULL;
	rc=query_nodelist(fa,cfgs(CFG_NLPATH),&rnode);
	switch(rc) {
	    case 1:write_log("can't query nodelist, index error");break;
	    case 2:write_log("can't query nodelist, nodelist error");break;
	    case 3:write_log("index is older than the list, need recompile");break;
	}
	if(!rnode) {
		rnode=xcalloc(1,sizeof(ninfo_t));
		falist_add(&rnode->addrs,fa);
		rnode->name=xstrdup("Unknown");
		rnode->phone=xstrdup("");
	}
	rnode->tty=NULL;
	ports=cfgsl(CFG_PORT);
	if((flags&2)!=2) {
		do {
			if(!ports)exit(33);
			port=tty_findport(ports,cfgs(CFG_NODIAL));
			if(!port)exit(33);
			if(rnode->tty)xfree(rnode->tty);
			rnode->tty=xstrdup(baseport(port));
			ports=ports->next;
		} while(!checktimegaps(cfgs(CFG_CANCALL)));
		if(!checktimegaps(cfgs(CFG_CANCALL)))exit(33);
	} else {
		if((port=tty_findport(ports,cfgs(CFG_NODIAL)))) {
			rnode->tty=xstrdup(baseport(port));
		} else {
			cls_close(ssock);
			exit(33);
		}
	}
	if(!cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
	if(line) {
		applysubst(rnode,psubsts);
		while(rnode->hidnum&&line!=rnode->hidnum&&applysubst(rnode,psubsts)&&rnode->hidnum!=1);
		if(line!=rnode->hidnum) {
			fprintf(stderr,"%s doesn't have line #%d\n",ftnaddrtoa(fa),line);
			cls_close(ssock);
			exit(0);
		}
		if(!can_dial(rnode,(flags&1)==1)) {
			fprintf(stderr,"We should not call to %s #%d at this time\n",ftnaddrtoa(fa),line);
			cls_close(ssock);
			exit(0);
		}
	} else {
		if (!find_dialable_subst(rnode,((flags&1)==1),psubsts)) {
			fprintf(stderr,"We should not call to %s at this time\n",ftnaddrtoa(fa));
			cls_close(ssock);
			exit(0);
		}
	}
	if(cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
	if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
		printf("can't open log %s!\n",ccs);
		cls_close(ssock);
		exit(0);
	}
	if(cfgs(CFG_RUNONCALL)) {
		char buf[MAX_PATH];
		snprintf(buf,MAX_PATH,"%s %s %s",ccs,ftnaddrtoa(fa),rnode->phone);
		if((rc=execsh(buf)))write_log("exec '%s' returned rc=%d",buf,rc);
	}
	if(rnode->hidnum)
	    write_log("calling %s #%d, %s (%s)",rnode->name,rnode->hidnum,ftnaddrtoa(fa),rnode->phone);
		else write_log("calling %s, %s (%s)",rnode->name,ftnaddrtoa(fa),rnode->phone);
	rc=do_call(fa,rnode->phone,port);
	return rc;
}

int main(int argc,char *argv[],char *envp[])
{
	int c,daemon=-1,rc,sesstype=SESSION_AUTO,line=0,call_flags=0;
	char *hostname=NULL,*str=NULL;
	FTNADDR_T(fa);
#ifndef HAVE_SETPROCTITLE
	setargspace(argc,argv,envp);
#endif
#ifdef HAVE_SETLOCALE
 	setlocale(LC_ALL, "");
#endif
	while((c=getopt(argc, argv, "hI:da:ni:c:tbv"))!=EOF) {
		switch(c) {
		    case 'c':
			daemon=12;
			str=optarg;
			while(str&&*str) {
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
		    case 'I':
			configname=optarg;
			break;
		    case 'd':
			daemon=1;
			break;
		    case 'a':
			daemon=0;
			sesstype=SESSION_AUTO;
			if(!strncasecmp(optarg,"**emsi",6))sesstype=SESSION_EMSI;
			if(!strncasecmp(optarg,"tsync",5))sesstype=SESSION_FTS0001;
			if(!strncasecmp(optarg,"yoohoo",6))sesstype=SESSION_YOOHOO;
#ifdef WITH_BINKP
			if(!strncasecmp(optarg,"binkp",5)){sesstype=SESSION_BINKP;bink=1;}
#endif
			break;
		    case 'n':
			daemon=2;
			break;
		    case 't':
			daemon=3;
			break;
#ifdef WITH_BINKP
		    case 'b':
			bink=1;
			break;
#endif
		    case 'v':
			u_vers(progname);
		    default:
			usage(argv[0]);
		}
	}
	if(!hostname&&daemon<0)usage(argv[0]);
	getsysinfo();ssock=lins_sock=uis_sock=-1;
	if(!readconfig(configname)) {
		write_log("there was some errors parsing '%s', aborting",configname);
		exit(EXC_BADCONFIG);
	}
	if(!log_init(cfgs(CFG_MASTERLOG),NULL)) {
		write_log("can't open master log '%s'!",ccs);
		exit(1);
	}
#ifdef NEED_DEBUG
	parse_log_levels();
	if(facilities_levels['C']>=1)dumpconfig();
#endif
	psubsts=parsesubsts(cfgfasl(CFG_SUBST));
#ifdef NEED_DEBUG
	if(facilities_levels['C']>=1) {
		subst_t *s;
		dialine_t *l;
		for(s=psubsts;s;s=s->next) {
			write_log("subst for %s [%d]",ftnaddrtoa(&s->addr),s->nhids);
			for(l=s->hiddens;l;l=l->next)
				write_log(" * %s,%s,%s,%d,%d",l->phone,l->host,l->timegaps,l->flags,l->num);
		}
	}
#endif
	if(daemon==3) {
		log_done();
		exit(EXC_OK);
	}
	if(hostname||daemon==12) {
		if(!parseftnaddr(argv[optind],&fa,&DEFADDR,0)) {
			write_log("can't parse address '%s'!",argv[optind]);
			log_done();
			exit(1);
		}
		optind++;
	}
	if(hostname) {
		is_ip=1;
		rnode=xcalloc(1,sizeof(ninfo_t));
		xstrcpy(ip_id,"ipline",10);
		rnode->tty=bink?"binkp":"tcpip";
		if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
			write_log("can't open log %s!",ccs);
			exit(1);
		}
		signal(SIGINT,sigerr);
		signal(SIGTERM,sigerr);
		signal(SIGSEGV,sigerr);
		signal(SIGPIPE,SIG_IGN);

		log_callback=NULL;xsend_cb=NULL;
		ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
		if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
		    else log_callback=vlogs;

		if(!bso_init(cfgs(CFG_BSOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init BSO");
		if(!aso_init(cfgs(CFG_ASOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init ASO");
		if(!BSO&&!ASO) {
			write_log("No outbound defined");
			stopit(1);
		}
		if(cfgs(CFG_RUNONCALL)) {
			char buf[MAX_PATH];
			snprintf(buf,MAX_PATH,"%s %s %s",ccs,ftnaddrtoa(&fa),hostname);
			if((rc=execsh(buf)))write_log("exec '%s' returned rc=%d",buf,rc);
		}
		tcp_call(hostname,&fa);
		if(BSO)bso_done();
		if(ASO)aso_done();
		stopit(0);
	}
	if(daemon==12) {
		int locked=0;
		if(optind<argc) {
			if(1!=sscanf(argv[optind],"%d",&line)) {
				write_log("can't parse line number '%s'!\n",argv[optind]);
				exit(1);
			}
		} else line = 0;

		log_callback=NULL;xsend_cb=NULL;
		ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
		if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
		    else log_callback=vlogs;

		if(!bso_init(cfgs(CFG_BSOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init BSO");
		if(!aso_init(cfgs(CFG_ASOOUTBOUND),cfgal(CFG_ADDRESS)->addr.z)&&ccs)write_log("can't init ASO");
		if(!BSO&&!ASO) {
			write_log("No outbound defined");
			cls_close(ssock);
			exit(1);
		}
		if(BSO)locked|=bso_locknode(&fa,LCK_c);
		if(ASO)locked|=aso_locknode(&fa,LCK_c);
		if(locked) {
			signal(SIGINT,sigerr);
			signal(SIGTERM,sigerr);
			signal(SIGSEGV,sigerr);
			signal(SIGPIPE,SIG_IGN);
			rc=force_call(&fa,line,call_flags);
			if(BSO)bso_unlocknode(&fa,LCK_x);
			if(ASO)aso_unlocknode(&fa,LCK_x);
		} else rc=0;
		if(rc&S_MASK)write_log("can't call to %s",ftnaddrtoa(&fa));
		if(BSO)bso_done();
		if(ASO)aso_done();
		stopit(rc);
	}
	switch(daemon) {
	    case 1: daemon_mode(); break;
	    case 0: answer_mode(sesstype); break;
	    case 2: compile_nodelists(); break;
	}
	cls_close(ssock);
	return 0;
}
