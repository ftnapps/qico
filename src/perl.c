/**********************************************************
 * perl support
 * $Id: perl.c,v 1.7 2004/06/20 18:50:50 sisoft Exp $
 **********************************************************/
#include "headers.h"
#ifdef WITH_PERL
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#ifndef sv_undef
#	define sv_undef PL_sv_undef
#endif
#define pladd_int(_sv,_name,_v) if((_sv=perl_get_sv(_name,TRUE))) { \
	sv_setiv(_sv,(_v));SvREADONLY_on(_sv);}
#define pladd_str(_sv,_name,_v) if((_sv=perl_get_sv(_name,TRUE))) { \
	if(_v)sv_setpv(_sv,(_v));else sv_setsv(_sv,&sv_undef);SvREADONLY_on(_sv);}
#define pladd_strz(_sv,_name,_v) if((_sv=perl_get_sv(_name,TRUE))) { \
	sv_setpv(_sv,SS(_v));SvREADONLY_on(_sv);}
#define plhadd_sv(_hv,_sv,_name) if(_sv) { \
	SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0);}
#define plhadd_str(_hv,_sv,_name,_v) if((_v)&&(_sv=newSVpv((_v),0))) \
	{ SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0); \
	} else hv_store(_hv,_name,strlen(_name),&sv_undef,0);
#define plhadd_int(_hv,_sv,_name,_v) if(_sv=newSViv((_v))) { \
	SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0);}
#define ploffRO(_sv) if(_sv) {SvREADONLY_off(_sv);}
#define PerlHave(_t) (perl&&(perl_nc&(1<<(_t))))

typedef enum {
	PERL_ON_INIT,
	PERL_ON_EXIT,
	PERL_ON_LOG,
	PERL_ON_CALL,
	PERL_ON_SESSION,
	PERL_END_SESSION,
	PERL_ON_RECV,
	PERL_END_RECV,
	PERL_ON_SEND,
	PERL_END_SEND
} perl_subs;

static char *perl_subnames[]={
	"on_init",
	"on_exit",
	"on_log",
	"on_call",
	"on_session",
	"end_session",
	"on_recv",
	"end_recv",
	"on_send",
	"end_send"
};

static struct {char *name;int val;} perl_const[]={
	{"S_OK",S_OK},
	{"S_NODIAL",S_NODIAL},
	{"S_REDIAL",S_REDIAL},
	{"S_BUSY",S_BUSY},
	{"S_FAILURE",S_FAILURE},
	{"S_HOLDR",S_HOLDR},
	{"S_HOLDX",S_HOLDX},
	{"S_HOLDA",S_HOLDA},
	{"S_ADDTRY",S_ADDTRY},
	{"F_OK",FOP_OK},
	{"F_CONT",FOP_CONT},
	{"F_SKIP",FOP_SKIP},
	{"F_ERR",FOP_ERROR},
	{"F_SUSPEND",FOP_SUSPEND}
};

static PerlInterpreter *perl=NULL;
static unsigned perl_nc=0;
unsigned short perl_flg=0;

static void sub_err(int sub)
{
	STRLEN len;
	char *s=SvPV(ERRSV,len);
	if(!s||!len)s="(null message)";
	write_log("perl %s error:%s%s",perl_subnames[sub],strchr(s,'\n')?"\n":" ",s);
}

static XS(perl_wlog)
{
	dXSARGS;
	int lvl=0;
	char *str;
	STRLEN len;
	if(items==1||items==2) {
		if(items==2)lvl=SvIV(ST(0));
		str=(char*)SvPV(ST((items-1)),len);
		if(!lvl)write_log("%s",len?str:"(empty perl log)");
		    else DEBUG(('P',lvl,"%s",len?str:NULL));
	} else write_log("perl wlog() error: wrong number of args");
	XSRETURN_EMPTY;
}

static XS(perl_setflag)
{
	dXSARGS;
	int num,arg;
	if(items==2) {
		num=SvIV(ST(0));
		arg=SvIV(ST(1));
		DEBUG(('P',3,"perl setflag(%d,%d)",num,arg));
		if(num<0||num>9||arg<0)write_log("perl setflags() error: illegal argument");
		    else {
			if(arg)perl_flg|=1<<num;
			else perl_flg&=~(1<<num);
		}
		
	} else write_log("perl setflags() error: wrong number of args");
	XSRETURN_EMPTY;
}

static void perl_xs_init()
{
	static char *file=__FILE__;
	newXS("wlog",perl_wlog,file);
	newXS("setflag",perl_setflag,file);
}

static void perl_setup(int daemon,int init)
{
	int i;
	SV *sv;
	AV *av,*ava;
	HV *hv,*hva;
	cfgitem_t *c;
	slist_t *sl;
	falist_t *al;
	faslist_t *fasl;
	pladd_int(sv,"init",init);
	pladd_int(sv,"daemon",daemon);
	pladd_str(sv,"conf",configname);
	hv=perl_get_hv("conf",TRUE);
	hv_clear(hv);
	for(i=0;i<CFG_NNN;i++)
	    for(c=configtab[i].items;c;c=c->next) {
		if(c->condition)continue;
		switch(configtab[i].type) {
		    case C_PATH: case C_STR: /* $conf{kw} */
			plhadd_str(hv,sv,configtab[i].keyword,c->value.v_char);
			break;
		    case C_INT: case C_OCT: case C_YESNO: /* $conf{kw} */
			plhadd_int(hv,sv,configtab[i].keyword,c->value.v_int);
			break;
		    case C_STRL: /* @conf{kw} */
			av=newAV();
			for(sl=c->value.v_sl;sl;sl=sl->next) {
				sv=newSVpv(sl->str,0);
				SvREADONLY_on(sv);
				av_push(av,sv);
			}
			sv=newRV_noinc((SV*)av);
			plhadd_sv(hv,sv,configtab[i].keyword);
			break;
		    case C_ADDRL: /* @conf{kw} */
			av=newAV();
			for(al=c->value.v_al;al;al=al->next) {
				sv=newSVpv(al->addr.d?ftnaddrtoda(&al->addr):ftnaddrtoa(&al->addr),0);
				SvREADONLY_on(sv);
				av_push(av,sv);
			}
			sv=newRV_noinc((SV*)av);
			plhadd_sv(hv,sv,configtab[i].keyword);
			break;
		    case C_ADRSTRL: /* @conf{kw}{adr|str} */
			av=newAV();ava=newAV();
			for(fasl=c->value.v_fasl;fasl;fasl=fasl->next) {
				sv=newSVpv(fasl->addr.d?ftnaddrtoda(&fasl->addr):ftnaddrtoa(&fasl->addr),0);
				SvREADONLY_on(sv);
				av_push(ava,sv);
				sv=newSVpv(fasl->str,0);
				SvREADONLY_on(sv);
				av_push(av,sv);
			}
			hva=newHV();
			sv=newRV_noinc((SV*)ava);
			plhadd_sv(hva,sv,"adr");
			sv=newRV_noinc((SV*)av);
			plhadd_sv(hva,sv,"str");
			sv=newRV_noinc((SV*)hva);
			plhadd_sv(hv,sv,configtab[i].keyword);
			break;
		}
	}
	perl_flg=0;
}

int perl_init(char *script,int mode)
{
	int rc;
	SV *sv;
	char *perlargs[]={"",NULL,NULL};
	if(!script||!*script)return 0;
	DEBUG(('P',2,"perl_init(%s, %d)",script,mode));
	if(!fexist(script)||access(script,R_OK)) {
		perl=NULL;
		write_log("can't access perlfile %s: %s",script,strerror(errno));
		return 0;
	}
	perl=perl_alloc();
	if(perl) {
		perlargs[1]=script;
		perl_construct(perl);
		rc=perl_parse(perl,perl_xs_init,2,perlargs,NULL);
	} else {
		write_log("perl allocation error");
		return 0;
	}
	if(rc) {
		perl_destruct(perl);
		perl_free(perl);
		perl=NULL;
		write_log("can't parse perlfile %s",script);
		return 0;
	}
	perl_run(perl);
	for(rc=0,perl_nc=0;rc<sizeof(perl_subnames)/sizeof(*perl_subnames);rc++)
		if(perl_get_cv(perl_subnames[rc],FALSE))perl_nc|=1<<rc;
	if(!perl_nc) {
		perl_done(0);
		return 0;
	}
	pladd_strz(sv,"version",version);
	perl_setup(mode,1);
	for(rc=0;rc<sizeof(perl_const)/sizeof(*perl_const);rc++)
		pladd_int(sv,perl_const[rc].name,perl_const[rc].val);
	perl_on_std(PERL_ON_INIT);
	return 1;
}

void perl_done(int rc)
{
	if(perl) {
		SV *sv;
		DEBUG(('P',2,"perl_done(%d)",rc));
		pladd_int(sv,"emerg",rc);
		perl_on_std(PERL_ON_EXIT);
		perl_destruct(perl);
		perl_free(perl);
		perl=NULL;
		perl_nc=0;
	}
}

void perl_on_reload(int mode)
{
	if(perl) {
		DEBUG(('P',4,"perl_reload(%d)",mode));
		perl_setup(!mode,0);
		perl_on_std(PERL_ON_INIT);
	}
}

void perl_on_std(int sub)
{
	if(PerlHave(sub)) {
		dSP;
		DEBUG(('P',4,"perl_on_std(%s)",perl_subnames[sub]));
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[sub],G_EVAL|G_VOID);
		SPAGAIN;
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV))sub_err(sub);
	}
}

void perl_on_log(char *str)
{
	static int Lock=0;
	if(!Lock&&PerlHave(PERL_ON_LOG)) {
		SV *svret,*sv;
		STRLEN len;
		int rc=0;
		dSP;
		Lock=1;
		DEBUG(('P',4,"perl_on_log(%s)",str));
		pladd_strz(sv,"_",str);ploffRO(sv);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_ON_LOG],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret))rc=SvIV(svret);
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_ON_LOG);
			rc=1;
		} else if(rc&&sv) {
			char *p=SvPV(sv,len);
			if(p&&len)strncpy(str,p,MIN(len+1,LARGE_STRING-1));
		}
		Lock=0;
	}
}

int perl_on_call(ftnaddr_t *fa,char *site,char *port)
{
	if(PerlHave(PERL_ON_CALL)) {
		SV *sv,*svret;
		int rc=S_OK;
		dSP;
		DEBUG(('P',4,"perl_on_call(%s, %s, %s)",ftnaddrtoa(fa),site,port));
		pladd_strz(sv,"addr",ftnaddrtoa(fa));
		pladd_int(sv,"tcpip",port?0:1);
		pladd_int(sv,"binkp",bink);
		pladd_str(sv,"port",port?port:"ip");
		pladd_str(sv,"site",site);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_ON_CALL],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret))rc=SvIV(svret);
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_ON_CALL);
			rc=S_OK;
		}
		DEBUG(('P',5,"perl_on_call() returns %d",rc));
		return rc;
	}
	return S_OK;
}

int perl_on_session(char *sysflags)
{
	if(PerlHave(PERL_ON_SESSION)) {
		SV *sv,*svret;
		falist_t *fa;
		flist_t *lst;
		int rc=S_OK;
		char *p;
		AV *av;
		HV *hv;
		dSP;
		DEBUG(('P',4,"perl_on_session(%s)",sysflags));
		pladd_int(sv,"start",rnode->starttime);
		strtr(sysflags,'/',' ');
		pladd_strz(sv,"flags",sysflags);
		av=perl_get_av("addrs",TRUE);
		av_clear(av);
		for(fa=cfgal(CFG_ADDRESS);fa;fa=fa->next) {
			p=ftnaddrtoa(&fa->addr);
			sv=newSVpv(p,0);
			SvREADONLY_on(sv);
			av_push(av,sv);
		}
		av=perl_get_av("akas",TRUE);
		av_clear(av);
		for(fa=rnode->addrs;fa;fa=fa->next) {
			p=ftnaddrtoa(&fa->addr);
			sv=newSVpv(p,0);
			SvREADONLY_on(sv);
			av_push(av,sv);
		}
		hv=perl_get_hv("info",TRUE);
		hv_clear(hv);
		plhadd_str(hv,sv,"sysop",rnode->sysop);
		plhadd_str(hv,sv,"mailer",rnode->mailer);
		plhadd_str(hv,sv,"station",rnode->name);
		plhadd_str(hv,sv,"place",rnode->place);
		plhadd_str(hv,sv,"flags",rnode->flags);
		plhadd_str(hv,sv,"wtime",rnode->haswtime?rnode->wtime:NULL);
		plhadd_str(hv,sv,"password",rnode->pwd);
		plhadd_int(hv,sv,"time",rnode->time);
		plhadd_int(hv,sv,"speed",rnode->speed);
		plhadd_int(hv,sv,"connect",rnode->realspeed);
		hv=perl_get_hv("flags",TRUE);
		hv_clear(hv);
		plhadd_int(hv,sv,"in",(rnode->options&O_INB)?1:0);
		plhadd_int(hv,sv,"out",(rnode->options&O_INB)?0:1);
		plhadd_int(hv,sv,"tcp",(rnode->options&O_TCP)?1:0);
		plhadd_int(hv,sv,"secure",(rnode->options&O_PWD)?1:0);
		plhadd_int(hv,sv,"listed",(rnode->options&O_LST)?1:0);
		hv=perl_get_hv("queue",TRUE);
		hv_clear(hv);
		plhadd_int(hv,sv,"mail",totalm);
		plhadd_int(hv,sv,"files",totalf);
		plhadd_int(hv,sv,"num",totaln);
		av=perl_get_av("queue",TRUE);
		av_clear(av);
		for(lst=fl;lst;lst=lst->next) {
			sv=newSVpv(lst->tosend,0);
			SvREADONLY_on(sv);
			av_push(av,sv);
		}
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_ON_SESSION],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret))rc=SvIV(svret);
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_ON_SESSION);
			rc=S_OK;
		}
		DEBUG(('P',5,"perl_on_session() returns %d",rc));
		return rc;
	}
	return S_OK;
}

void perl_end_session(long sest,int result)
{
	if(PerlHave(PERL_END_SESSION)) {
		SV *sv;
		dSP;
		DEBUG(('P',4,"perl_end_session(%ld, %d)",sest,result));
		pladd_int(sv,"r_bytes",recvf.toff-recvf.soff);
		pladd_int(sv,"r_files",recvf.nf);
		pladd_int(sv,"s_bytes",sendf.toff-sendf.soff);
		pladd_int(sv,"s_files",sendf.nf);
		pladd_int(sv,"sesstime",sest);
		pladd_int(sv,"result",result);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_END_SESSION],G_EVAL|G_VOID);
		SPAGAIN;
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV))sub_err(PERL_END_SESSION);
	}
}

int perl_on_recv()
{
    if(PerlHave(PERL_ON_RECV)) {
		SV *sv,*svret;
		HV *hv;
		int rc=FOP_OK;
		dSP;
		DEBUG(('P',4,"perl_on_recv()"));
		hv=perl_get_hv("recv",TRUE);
		hv_clear(hv);
		plhadd_str(hv,sv,"name",recvf.fname);
		plhadd_int(hv,sv,"size",recvf.ftot);
		plhadd_int(hv,sv,"time",recvf.mtime);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_ON_RECV],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret))rc=SvIV(svret);
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_ON_RECV);
			rc=FOP_OK;
		}
		DEBUG(('P',5,"perl_on_recv() returns %d",rc));
		return rc;
    }
    return FOP_OK;
}

char *perl_end_recv(int state)
{
    if(PerlHave(PERL_END_RECV)) {
		SV *sv,*svret;
		char *rc=NULL;
		STRLEN len;
		dSP;
		DEBUG(('P',4,"perl_end_recv(%d)",state));
		pladd_int(sv,"state",state);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_END_RECV],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret)) {
			rc=SvPV(svret,len);
			if(!len)rc=NULL;
			if(len==1&&*rc=='!')rc="";
		}
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_END_RECV);
			rc=NULL;
		}
		DEBUG(('P',5,"perl_end_recv() returns '%s'",rc));
		return rc;
    }
    return NULL;
}

char *perl_on_send(char *tosend)
{
    if(PerlHave(PERL_ON_SEND)) {
		SV *sv,*svret;
		HV *hv;
		char *rc=NULL;
		STRLEN len;
		dSP;
		DEBUG(('P',4,"perl_on_send(%s)",tosend));
		hv=perl_get_hv("send",TRUE);
		hv_clear(hv);
		plhadd_str(hv,sv,"file",tosend);
		plhadd_str(hv,sv,"name",sendf.fname);
		plhadd_int(hv,sv,"size",sendf.ftot);
		plhadd_int(hv,sv,"time",sendf.mtime);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_ON_SEND],G_EVAL|G_SCALAR);
		SPAGAIN;
		svret=POPs;
		if(SvOK(svret)) {
			rc=SvPV(svret,len);
			if(!len)rc=NULL;
			if(len==1&&*rc=='!')rc="";
		}
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV)) {
			sub_err(PERL_ON_SEND);
			rc=NULL;
		}
		DEBUG(('P',5,"perl_on_send() returns '%s'",rc));
		return rc;
    }
    return NULL;
}

void perl_end_send(int state)
{
    if(PerlHave(PERL_END_SEND)) {
		SV *sv;
		dSP;
		DEBUG(('P',4,"perl_end_send(%d)",state));
		pladd_int(sv,"state",state);
		ENTER;
		SAVETMPS;
		PUSHMARK(SP);
		PUTBACK;
		perl_call_pv(perl_subnames[PERL_END_SEND],G_EVAL|G_VOID);
		SPAGAIN;
		PUTBACK;
		FREETMPS;
		LEAVE;
		if(SvTRUE(ERRSV))sub_err(PERL_END_SEND);
    }
}

#endif
