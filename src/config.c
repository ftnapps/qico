/**********************************************************
 * work with config
 * $Id: config.c,v 1.23 2004/06/22 08:28:30 sisoft Exp $
 **********************************************************/
#include "headers.h"

extern int flagexp(slist_t *expr,int strict);

static slist_t *condlist=NULL,*curcond;

static int getstr(char **to,char *from)
{
	*to=xstrdup(from);
	return 1;
}

static int getpath(char **to,char *from)
{
	if(strcspn(from,"*[]?<>|")!=strlen(from))return 0;
	if(from[strlen(from)-1]=='/')chop(from,1);
   	*to=xstrdup(from);
	return 1;
}

static int getlong(int *to,char *from)
{
	if(strspn(from,"0123456789 \t")!=strlen(from))return 0;
	*to=atol(from);
	return 1;
}

static int getoct(int *to,char *from)
{
	if(strspn(from,"01234567 \t")!=strlen(from))return 0;
	*to=strtol(from,(char**)NULL,8);
	return 1;
}

static int getaddrl(falist_t **to,char *from)
{
	FTNADDR_T(ta);
	if(!parseftnaddr(from,&ta,&DEFADDR,0))return 0;
	if(!DEFADDR.z) {DEFADDR.z=ta.z;DEFADDR.n=ta.n;}
	falist_add(to,&ta);
	return 1;
}

static int getfasl(faslist_t **to,char *from)
{
	char *p;
	FTNADDR_T(ta);
	if(!parseftnaddr(from,&ta,&DEFADDR,0)) return 0;
	if(!DEFADDR.z) {DEFADDR.z=ta.z;DEFADDR.n=ta.n;}
	p=strchr(from,' ');if(!p)return 0;
	while(*p==' ')p++;
	faslist_add(to,p,&ta);
	return 1;
}

static int getyesno(int *to,char *from)
{
	if(tolower(*from)=='y'||*from=='1'||tolower(*from)=='t')*to=1;
	    else *to=0;
	return 1;
}

static int getstrl(slist_t **to,char *from)
{
	slist_add(to,from);
	return 1;
}

static int setvalue(cfgitem_t *ci,char *t,int type)
{
	switch(type) {
	case C_STR:	return getstr(&ci->value.v_char,t);
	case C_PATH:	return getpath(&ci->value.v_char,t);
	case C_STRL:	return getstrl(&ci->value.v_sl,t);
	case C_ADRSTRL:	return getfasl(&ci->value.v_fasl,t);
	case C_ADDRL:	return getaddrl(&ci->value.v_al,t);
	case C_INT:	return getlong(&ci->value.v_int,t);
	case C_OCT:	return getoct(&ci->value.v_int,t);
	case C_YESNO:	return getyesno(&ci->value.v_int,t);
	}
	return 0;
}

int cfgi(int i)
{
	cfgitem_t *ci,*cn=NULL;
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition&&flagexp(ci->condition,0)==1)
			return cci=ci->value.v_int;
		if(!ci->condition)cn=ci;
	}
	return cci=cn->value.v_int;
}

char *cfgs(int i)
{
	cfgitem_t *ci,*cn=NULL;
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition&&flagexp(ci->condition,0))
			return ccs=ci->value.v_char;
		if(!ci->condition)cn=ci;
	}
	return ccs=cn->value.v_char;
}

slist_t *cfgsl(int i)
{
	cfgitem_t *ci,*cn=NULL;
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition&&flagexp(ci->condition,0))
			return ccsl=ci->value.v_sl;
		if(!ci->condition)cn=ci;
	}
	return ccsl=cn->value.v_sl;
}

faslist_t *cfgfasl(int i)
{
	cfgitem_t *ci,*cn=NULL;
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition&&flagexp(ci->condition,0))
			return ccfasl=ci->value.v_fasl;
		if(!ci->condition)cn=ci;
	}
	return ccfasl=cn->value.v_fasl;
}

falist_t *cfgal(int i)
{
	cfgitem_t *ci,*cn=NULL;
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition&&flagexp(ci->condition,0))
			return ccal=ci->value.v_al;
		if(!ci->condition)cn=ci;
	}
	return ccal=cn->value.v_al;
}

int readconfig(char *cfgname)
{
	int rc,i;
	cfgitem_t *ci;
	curcond=NULL;
	rc=parseconfig(cfgname);
	if(curcond) {
		write_log("%s: not all <if>-expressions closed",cfgname);
		while(curcond)slist_dell(&curcond);
		rc=0;
	}
	if(!rc)return 0;
	for(i=0;i<CFG_NNN;i++)
	    if(configtab[i].found<2) {
		if(configtab[i].required&1) {
			write_log("required keyword '%s' not defined",configtab[i].keyword);
			rc=0;
		}
		ci=xcalloc(1,sizeof(cfgitem_t));
		if(configtab[i].def_val)
		    setvalue(ci,configtab[i].def_val,configtab[i].type);
		else memset(&ci->value,0,sizeof(ci->value));
		ci->condition=NULL;ci->next=NULL;
		if(configtab[i].found) {
			cfgitem_t *cia=configtab[i].items;
			for(;cia&&cia->next;cia=cia->next);
			if(cia)cia->next=ci;
		}
		if(!configtab[i].items)configtab[i].items=ci;
	}
	if(rc) { /* read tables */
		recode_to_local(NULL);
		recode_to_remote(NULL);
	}
	return rc;
}

int parsekeyword(char *kw,char *arg,char *cfgname,int line)
{
	int i=0,rc=1;
	slist_t *cc;
	cfgitem_t *ci;
	while(configtab[i].keyword&&strcasecmp(configtab[i].keyword,kw))i++;
	DEBUG(('C',2,"parse: '%s', '%s' [%d] on %s:%d%s",kw,arg,i,cfgname,line,curcond?" (c)":""));
	if(configtab[i].keyword) {
		if(curcond&&(configtab[i].required&2)) {
			write_log("%s:%d: keyword '%s' can't be defined inside if-expression's",cfgname,line,kw);
			return 0;
		}
		for(ci=configtab[i].items;ci;ci=ci->next) {
			slist_t *a=ci->condition,*b=curcond;
			for(;a&&b&&a->str==b->str;a=a->next,b=b->next);
			if(!a&&!b)break;
		}
		if(!ci) {
			ci=xcalloc(1,sizeof(cfgitem_t));
			for(cc=curcond;cc;cc=cc->next)
			slist_addl(&ci->condition,cc->str);
			ci->next=configtab[i].items;
			configtab[i].items=ci;
		}
		if(setvalue(ci,arg,configtab[i].type)) {
			if(configtab[i].found<2) {
			    if(!curcond)configtab[i].found=2;
				else configtab[i].found=1;
			}
		} else {
			xfree(ci);
			write_log("%s:%d: can't parse '%s %s'",cfgname,line,kw,arg);
			rc=0;
		}
	} else {
		write_log("%s:%d: unknown keyword '%s'",cfgname,line,kw);
		rc=0;
	}
	return rc;
}

int parseconfig(char *cfgname)
{
	FILE *f;
	char s[MAX_STRING*2],*p,*t,*k;
	int line=0,rc=1;
	slist_t *cc;
	if(!(f=fopen(cfgname,"rt"))) {
		write_log("can't open config file '%s': %s",cfgname,strerror(errno));
		return 0;
	}
	while(fgets(s,MAX_STRING*2,f)) {
		line++;
contl:		p=s;
		strtr(p,'\t',' ');
		while(*p==' ')p++;
		if(*p&&*p!='#'&&*p!='\n'&&*p!=';'&&(*p!='/'||p[1]!='/')) {
			for(t=p+strlen(p)-1;*t==' '||*t=='\r'||*t=='\n';t--);
			if(*t=='\\'&&t>p&&*(t-1)==' ') {
				do {
					line++;
					if(!fgets(t,MAX_STRING*2-(t-p),f))*t=0;
				} while(*t&&(*t==';'||*t=='#'||(*t=='/'&&t[1]=='/')));
				for(k=t;*k==' ';k++);
				if(k>t)xstrcpy(t,k,strlen(k));
				goto contl;
			}
			if(*p!='{') {
				t=strchr(p,' ');
				if(!t)t=strchr(p,'\n');
				if(!t)t=strchr(p,'\r');
				if(!t)t=strchr(p,0);
				*t++=0;
			} else t=p+1;
			while(*t==' ')t++;
			for(k=t+strlen(t)-1;*k=='\n'||*k=='\r'||*k==' ';k--)*k=0;
			if(!strcasecmp(p,"include")) {
				if(!strncmp(cfgname,t,MAX_STRING)) {
					write_log("%s:%d: <include> including itself -> infinity loop",cfgname,line);
					rc=0;
				} else if(!parseconfig(t)) {
					write_log("%s:%d: was errors parsing included file '%s'",cfgname,line,t);
					rc=0;
				}
			} else if(*p=='{'||!strcasecmp(p,"if")) {
				for(k=t;*k&&(*k!=':'||k[1]!=' ');k++);
				if(*k==':'&&k[1]==' ') {
					*k++=0;
					if(*(k-2)==' ')*(k-2)=0;
					while(*k==' ')k++;
					for(p=k;*p&&*p!=' ';p++);
					*p++=0;
					while(*p==' ')p++;
					if(!*p) {
						write_log("%s:%d: inline <if>-expression witout arguments",cfgname,line);
						rc=0;k=NULL;
					}
					if(*(p+strlen(p)-1)=='}')*(p+strlen(p)-1)=0;
				} else k=NULL;
				cc=slist_add(&condlist,t);
				if(flagexp(cc,1)<0) {
					write_log("%s:%d: can't parse expression '%s'",cfgname,line,t);
					rc=0;
				} else slist_addl(&curcond,cc->str);
				if(k&&curcond) {
					if(!parsekeyword(k,p,cfgname,line))rc=0;
					slist_dell(&curcond);
				}
			} else if(!strcasecmp(p,"else")) {
				if(!curcond) {
					write_log("%s:%d: misplaced <else> without <if>",cfgname,line);
					rc=0;
				} else {
					k=slist_dell(&curcond);
					snprintf(s,MAX_STRING*2,"not(%s)",k);
					cc=slist_add(&condlist,s);
					slist_addl(&curcond,cc->str);
				}
			} else if(*p=='}'||!strcasecmp(p,"endif")) {
				if(!curcond) {
					write_log("%s:%d: misplaced <endif> without <if>",cfgname,line);
					rc=0;
				} else slist_dell(&curcond);
			} else if(!parsekeyword(p,t,cfgname,line))rc=0;
		}
	}
	fclose(f);
	return rc;
}

#ifdef NEED_DEBUG
void dumpconfig()
{
	int i;
	char buf[LARGE_STRING];
	cfgitem_t *c;
	slist_t *sl;
	falist_t *al;
	faslist_t *fasl;
	for(i=0;i<CFG_NNN;i++) {
		write_log("conf: %s. (type=%d, need=%d, found=%d)",
			   configtab[i].keyword,configtab[i].type,
			   configtab[i].required,configtab[i].found);
		for(c=configtab[i].items;c;c=c->next) {
			xstrcpy(buf,"conf:   ",LARGE_STRING);
			if(c->condition) {
				xstrcpy(buf+8,"if (",LARGE_STRING);
				for(sl=c->condition;sl;sl=sl->next) {
					xstrcat(buf+12,sl->str,LARGE_STRING);
					if(sl->next)xstrcat(buf+12,") and (",LARGE_STRING);
				}
				xstrcat(buf+12,"): ",LARGE_STRING);
			} else xstrcat(buf,"default: ",LARGE_STRING);
			switch(configtab[i].type) {
			    case C_PATH:
			    case C_STR:	  snprintf(buf+strlen(buf),LARGE_STRING,"'%s'",c->value.v_char);break;
			    case C_INT:   snprintf(buf+strlen(buf),LARGE_STRING,"%d",c->value.v_int);break;
			    case C_OCT:   snprintf(buf+strlen(buf),LARGE_STRING,"%o",c->value.v_int);break;
			    case C_YESNO: snprintf(buf+strlen(buf),LARGE_STRING,"%s",c->value.v_int?"yes":"no");break;
			    case C_STRL:
				for(sl=c->value.v_sl;sl;sl=sl->next)
					snprintf(buf+strlen(buf),LARGE_STRING,"'%s', ",sl->str);
				xstrcat(buf,"%",LARGE_STRING);
				break;
			    case C_ADRSTRL:
				for(fasl=c->value.v_fasl;fasl;fasl=fasl->next)
					snprintf(buf+strlen(buf),LARGE_STRING,"%s '%s', ",fasl->addr.d?ftnaddrtoda(&fasl->addr):ftnaddrtoa(&fasl->addr),fasl->str);
				xstrcat(buf,"%",LARGE_STRING);
				break;
			    case C_ADDRL:
				for(al=c->value.v_al;al;al=al->next)
					snprintf(buf+strlen(buf),LARGE_STRING,"%s, ",al->addr.d?ftnaddrtoda(&al->addr):ftnaddrtoa(&al->addr));
				xstrcat(buf,"%",LARGE_STRING);
				break;
			}
			write_log("%s",buf);
		}
	}
}
#endif

void killconfig()
{
	int i;
	cfgitem_t *c,*t;
	slist_kill(&condlist);
	for(i=0;i<CFG_NNN;i++) {
		for(c=configtab[i].items;c;c=t) {
			switch(configtab[i].type) {
			    case C_PATH:
			    case C_STR:
				xfree(c->value.v_char);
				break;
			    case C_STRL:
				slist_kill(&c->value.v_sl);
				break;
			    case C_ADRSTRL:
				faslist_kill(&c->value.v_fasl);
				break;
			    case C_ADDRL:
				falist_kill(&c->value.v_al);
				break;
			    case C_OCT:
			    case C_INT:
			    case C_YESNO:
				c->value.v_int=0;
				break;
			}
			slist_killn(&c->condition);
			t=c->next;
			xfree(c);
		}
		configtab[i].items=NULL;
		configtab[i].found=0;
	}
}
