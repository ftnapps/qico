/**********************************************************
 * qico main
 * $Id: main.c,v 1.34 2004/06/19 22:31:57 sisoft Exp $
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
	IFPerl(perl_done(0));
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
	aso_done();
	write_log("got SIG%s signal",sigs[sig]);
	if(cfgs(CFG_PIDFILE))if(getpid()==islocked(ccs))lunlink(ccs);
	IFPerl(perl_done(1));
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
		exit(S_FAILURE);
	}
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,sigerr);
	signal(SIGSEGV,sigerr);
	signal(SIGFPE,sigerr);
	signal(SIGPIPE,SIG_IGN);
	IFPerl(perl_init(cfgs(CFG_PERLFILE),0));
	log_callback=NULL;xsend_cb=NULL;
	ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
	if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
	    else log_callback=vlogs;

	rc=aso_init(cfgs(CFG_ASOOUTBOUND),cfgs(CFG_BSOOUTBOUND),cfgs(CFG_QSTOUTBOUND),cfgal(CFG_ADDRESS)->addr.z);
	if(!rc) {
		write_log("No outbound defined");
		stopit(S_FAILURE);
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
	tty_local(0);
	rc=session(0,type,NULL,spd);
	tty_local(1);
	if(!is_ip) {
		hangup();
		stat_collect();
	}
	tty_cooked();
	if((S_OK==(rc&S_MASK))&&cfgi(CFG_HOLDONSUCCESS)) {
		log_done();
		log_init(cfgs(CFG_MASTERLOG),NULL);
		aso_getstatus(&rnode->addrs->addr, &sts);
		sts.flags|=(Q_WAITA|Q_WAITR|Q_WAITX);
		sts.htime=MAX(t_set(cci*60),sts.htime);
		write_log("calls to %s delayed for %d min after successful incoming session",
				ftnaddrtoa(&rnode->addrs->addr),cci);
		aso_setstatus(&rnode->addrs->addr,&sts);
		log_done();
		log_init(cfgs(CFG_LOG),rnode->tty);
	}
	title("Waiting...");
	vidle();sline("");
	aso_done();
	stopit(rc);
}

int main(int argc,char **argv,char **envp)
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
				    default:  write_log("unknown call option: %c", *optarg);
					      exit(S_FAILURE);
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
			if(!strncasecmp(optarg,"binkp",5))sesstype=SESSION_BINKP,bink=1;
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
		exit(S_FAILURE);
	}
	if(!log_init(cfgs(CFG_MASTERLOG),NULL)) {
		write_log("can't open master log '%s'",ccs);
		exit(S_FAILURE);
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
	log_done();
	if(daemon==3)exit(S_OK);
	if(hostname||daemon==12) {
		if(!parseftnaddr(argv[optind],&fa,&DEFADDR,0)) {
			write_log("can't parse address '%s'",argv[optind]);
			exit(S_FAILURE);
		}
		optind++;
	}
	if(hostname) {
		is_ip=1;
		rnode=xcalloc(1,sizeof(ninfo_t));
		xstrcpy(ip_id,"ipline",10);
		rnode->tty=bink?"binkp":"tcpip";
		if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
			write_log("can't open log %s",ccs);
			exit(S_FAILURE);
		}
		signal(SIGINT,sigerr);
		signal(SIGTERM,sigerr);
		signal(SIGSEGV,sigerr);
		signal(SIGPIPE,SIG_IGN);
		IFPerl(perl_init(cfgs(CFG_PERLFILE),0));
		log_callback=NULL;xsend_cb=NULL;
		ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
		if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
		    else log_callback=vlogs;

		rc=aso_init(cfgs(CFG_ASOOUTBOUND),cfgs(CFG_BSOOUTBOUND),cfgs(CFG_QSTOUTBOUND),cfgal(CFG_ADDRESS)->addr.z);
		if(!rc) {
			write_log("No outbound defined");
			stopit(S_FAILURE);
		}
		rc=do_call(&fa,hostname,NULL);
		aso_done();
		stopit(rc);
	}
	if(daemon==12) {
		if(optind<argc) {
			if(1!=sscanf(argv[optind],"%d",&line)) {
				write_log("can't parse line number '%s'!\n",argv[optind]);
				exit(S_FAILURE);
			}
		} else line = 0;

		log_callback=NULL;xsend_cb=NULL;
		ssock=cls_conn(CLS_LINE,cfgs(CFG_SERVER),NULL);
		if(ssock<0)write_log("can't connect to server: %s",strerror(errno));
		    else log_callback=vlogs;

		rc=aso_init(cfgs(CFG_ASOOUTBOUND),cfgs(CFG_BSOOUTBOUND),cfgs(CFG_QSTOUTBOUND),cfgal(CFG_ADDRESS)->addr.z);
		if(!rc) {
			write_log("No outbound defined");
			cls_close(ssock);
			exit(S_FAILURE);
		}
		if(aso_locknode(&fa,LCK_c)) {
			signal(SIGINT,sigerr);
			signal(SIGTERM,sigerr);
			signal(SIGSEGV,sigerr);
			signal(SIGPIPE,SIG_IGN);
			IFPerl(perl_init(cfgs(CFG_PERLFILE),0));
			rc=force_call(&fa,line,call_flags);
			aso_unlocknode(&fa,LCK_x);
		} else rc=S_FAILURE;
		if(rc&S_MASK)write_log("can't call to %s",ftnaddrtoa(&fa));
		aso_done();
		stopit(rc);
	}
	switch(daemon) {
	    case 1: daemon_mode(); break;
	    case 0: answer_mode(sesstype); break;
	    case 2: compile_nodelists(); break;
	}
	return S_OK;
}
