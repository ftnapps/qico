/**********************************************************
 * perl support
 * $Id: perl.c,v 1.2 2004/06/09 22:25:50 sisoft Exp $
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
	sv_setiv(_sv,_v);SvREADONLY_on(_sv);}
#define pladd_str(_sv,_name,_v) if((_sv=perl_get_sv(_name,TRUE))) { \
	if(_v)sv_setpv(_sv,_v);else sv_setsv(_sv,&sv_undef);SvREADONLY_on(_sv);}
#define pladd_strz(_sv,_name,_v) if((_sv=perl_get_sv(_name,TRUE))) { \
	sv_setpv(_sv,SS(_v));SvREADONLY_on(_sv);}
#define plhadd_sv(_hv,_sv,_name) if(_sv) { \
	SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0);}
#define plhadd_str(_hv,_sv,_name,_value) if((_value)&&(_sv=newSVpv(_value,0))) \
	{ SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0); \
	} else hv_store(_hv,_name,strlen(_name),&sv_undef,0);
#define plhadd_int(_hv,_sv,_name,_value) if(_sv=newSViv(_value)) { \
	SvREADONLY_on(_sv);hv_store(_hv,_name,strlen(_name),_sv,0);}
#define ploffRO(_sv) if(_sv) {SvREADONLY_off(_sv);}
#define PerlHave(_t) (perl&&(perl_nc&(1<<(_t))))

typedef enum {
	PERL_ON_LOAD,
	PERL_ON_EXIT,
	PERL_ON_LOG,
	PERL_ON_CALL,
	PERL_ON_HS,
	PERL_END_HS,
	PERL_ON_SESSION,
	PERL_END_SESSION
} perl_subs;

static char *perl_subnames[]={
	"on_load",
	"on_exit",
	"on_log",
	"on_call",
	"on_handshake",
	"end_handshake",
	"on_session",
	"end_session"
};

static PerlInterpreter *perl = NULL;
static char *perlargs[]={"",NULL,NULL};
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
	char *str;
	STRLEN len;
	if(items==1) {
		str=(char*)SvPV(ST(0),len);
		write_log("%s",len?str:"(empty perl log)");
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
	int i=1,rc;
	DEBUG(('P',1,"perl_init(%s, %d)",script,mode));
	perlargs[i++]=script;
	if(access(script,R_OK)) {
		perl=NULL;
		write_log("can't access perlfile %s: %s",script,strerror(errno));
		return 0;
	}
	perl=perl_alloc();
	perl_construct(perl);
	rc=perl_parse(perl,perl_xs_init,i,perlargs,NULL);
	if(rc) {
		perl_destruct(perl);
		perl_free(perl);
		perl=NULL;
		write_log("can't parse perlfile %s",script);
		return 0;
	}
	perl_run(perl);
	perl_setup(mode,1);
	for(i=0,perl_nc=0;i<sizeof(perl_subnames)/sizeof(perl_subnames[0]);i++)
		if(perl_get_cv(perl_subnames[i],FALSE))perl_nc|=1<<i;
	perl_on_std(PERL_ON_LOAD);
	return 1;
}

void perl_done(int rc)
{
	if(perl) {
		SV *sv;
		DEBUG(('P',1,"perl_done(%d)",rc));
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
		DEBUG(('P',3,"perl_reload(%d)",mode));
		perl_setup(!mode,0);
		perl_on_std(PERL_ON_LOAD);
	}
}

void perl_on_std(int sub)
{
	if(PerlHave(sub)) {
		dSP;
		DEBUG(('P',3,"perl_on_std(%s)",perl_subnames[sub]));
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

int perl_on_log(char *str)
{
	static int Lock=0;
	if(!Lock&&PerlHave(PERL_ON_LOG)) {
		SV *svret,*sv;
		STRLEN len;
		int rc=0;
		dSP;
		Lock=1;
		DEBUG(('P',3,"perl_on_log(%s)",str));
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
			    else rc=0;
		} else rc=1;
		Lock=0;
		return rc;
	}
	return 1;
}

int perl_on_call()
{


}

#endif
