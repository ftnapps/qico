/**********************************************************
 * work with config
 * $Id: config.c,v 1.1.1.1 2003/07/12 21:26:29 sisoft Exp $
 **********************************************************/
#include "headers.h"

extern int flagexp(char *);

extern ftnaddr_t DEFADDR;
slist_t *condlist=NULL;
char *curcond=NULL;

int cci;
char *ccs;
slist_t *ccsl;
faslist_t *ccfasl;
falist_t *ccal;

int getstr(char **to, char *from)
{
	*to=xstrdup(from);
	return 1;
}

int getpath(char **to, char *from)
{
	if(strcspn(from, "*[]?<>|")!=strlen(from)) return 0;
	if(from[strlen(from)-1]=='/') chop(from,1);
   	*to=xstrdup(from);
	return 1;
}

int getlong(int *to, char *from)
{
	if(strspn(from, "0123456789 \t")!=strlen(from)) return 0;
	*to=atol(from);return 1;
}

int getoct(int *to, char *from)
{
	if(strspn(from, "01234567 \t")!=strlen(from)) return 0;
	*to=strtol(from, (char **)NULL, 8);return 1;
}	 

int getaddrl(falist_t **to, char *from)
{
	ftnaddr_t ta;
	if(!parseftnaddr(from, &ta, &DEFADDR, 0)) return 0;
	if(!DEFADDR.z) {
		DEFADDR.z=ta.z;DEFADDR.n=ta.n;
	}
	falist_add(to, &ta);
	return 1;
}

int getfasl(faslist_t **to, char *from)
{
	ftnaddr_t ta;char *p;
	if(!parseftnaddr(from, &ta, &DEFADDR, 0)) return 0;
	if(!DEFADDR.z) {
		DEFADDR.z=ta.z;DEFADDR.n=ta.n;
	}
	p=strchr(from, ' ');if(!p) return 0;
	while(*p==' ') p++;
	faslist_add(to, p, &ta);
	return 1;
}

int getyesno(int *to, char *from)
{
	if(tolower(*from)=='y' || *from=='1' || *from=='t') *to=1;
	else *to=0;
	return 1;
}

int getstrl(slist_t **to, char *from)
{
	slist_add(to, from);
	return 1;
}

int setvalue(cfgitem_t *ci, char *t, int type)
{
	switch(type) {
	case C_STR:     return getstr(&ci->value.v_char, t);
	case C_PATH:    return getpath(&ci->value.v_char, t);
	case C_STRL:    return getstrl(&ci->value.v_sl, t);
	case C_ADRSTRL: return getfasl(&ci->value.v_fasl, t);
	case C_ADDRL:   return getaddrl(&ci->value.v_al, t);
	case C_INT:     return getlong(&ci->value.v_int, t);
	case C_OCT:     return getoct(&ci->value.v_int, t);
	case C_YESNO:   return getyesno(&ci->value.v_int, t);
	}
	return 0;
}

int cfgi(int i)
{
	cfgitem_t *ci, *cn=((void *)0);
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition &&	flagexp(ci->condition)==1)
			return  cci=ci->value.v_int;
		if(!ci->condition) cn=ci;
	}
	return  cci =cn->value. v_int;
}

char *cfgs(int i)
{
	cfgitem_t *ci, *cn=((void *)0);
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition && flagexp(ci->condition))
			return  ccs=ci->value.v_char;
		if(!ci->condition) cn=ci;
	}
	return  ccs=cn->value.v_char;
} 
slist_t *cfgsl(int i)
{
	cfgitem_t *ci, *cn=((void *)0);
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition && flagexp(ci->condition))
			return ccsl =ci->value.v_sl;
		if(!ci->condition) cn=ci;
	}
	return ccsl=cn->value.v_sl;
}

faslist_t *cfgfasl(int i)
{
	cfgitem_t *ci, *cn=((void *)0);
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition && flagexp(ci->condition))
			return ccfasl=ci->value.v_fasl;
		if(!ci->condition) cn=ci;
	}
	return ccfasl=cn->value.v_fasl;
}  

falist_t *cfgal(int i)
{
	cfgitem_t *ci, *cn=((void *)0);
	for(ci=configtab[i].items;ci;ci=ci->next) {
		if(ci->condition && flagexp(ci->condition))
			return ccal =ci->value.v_al;
		if(!ci->condition) cn=ci;
	}
	return ccal =cn->value.v_al;
}  

int readconfig(char *cfgname)
{
	int rc, i;
	cfgitem_t *ci;
	rc=parseconfig(cfgname);
	if(!rc) return 0;
	for(i=0;i<CFG_NNN;i++) {
		if(!configtab[i].found) {
			if(configtab[i].required) {
				write_log("required value '%s' not defined!",
						configtab[i].keyword);
				rc=0;
			} else {
				ci=xcalloc(1, sizeof(cfgitem_t));
				if(configtab[i].def_val)
					setvalue(ci,configtab[i].def_val ,
							 configtab[i].type);
				else
					memset(&ci->value, 0, sizeof(ci->value));
				ci->condition=NULL;configtab[i].found=1;
				ci->next=NULL;
				configtab[i].items=ci;
			}
		}
	}
	return rc;
}
	
int parseconfig(char *cfgname)
{
	FILE *f;
	char s[MAX_STRING], *p, *t, *k;
	int i,line=0,rc=1;
	slist_t *cc;
	cfgitem_t *ci;

	f=fopen(cfgname, "rt");
	if(!f) {
		fprintf(stderr,"can't open config '%s'\n",cfgname);
		return 0;
	}
	while(fgets(s, MAX_STRING, f)) {
		line++;
		p=s;strtr(p, '\t', ' ');
		while(*p==' ') p++;
		if(*p && *p!='#' && *p!='\n' && *p!=';') {
			t=strchr(p, ' ');
			if(!t) t=strchr(p, '\n');
			if(!t) t=strchr(p, '\r');
			if(!t) t=strchr(p, 0);
			*t=0;t++;
			while(*t==' ') t++;
			for(k=t+strlen(t)-1;*k=='\n'||*k=='\r'||*k==' ';k--) *k=0;
			i=0;
			if(!strcasecmp(p, "include")) {
				if(!parseconfig(t)) {
					write_log("%d: was errors parsing included file %s",
							line, p);
					rc=0;					
				}
			} else if(!strcasecmp(p, "if"))	{
				if(flagexp(t)<0) {
					write_log("%d: can't parse expression '%s'",
							line, t);
					rc=0;
				} else {
					cc=slist_add(&condlist, t);
					curcond=cc->str;
				}
			} else if(!strcasecmp(p, "endif"))	{
				if(!curcond) {
					write_log("%d: misplaced 'endif' without 'if'!", line);
					rc=0;
				} else curcond=NULL;
			} else {
				while(configtab[i].keyword && strcasecmp(configtab[i].keyword, p))
					i++;
				if(configtab[i].keyword) {
					for(ci=configtab[i].items;ci;ci=ci->next)
						if(ci->condition==curcond) break;
					if(!ci) {
						ci=xcalloc(1, sizeof(cfgitem_t));
						ci->condition=curcond;
						ci->next=configtab[i].items;
						configtab[i].items=ci;
					}
					if(setvalue(ci, t, configtab[i].type)) {
						if(!curcond) configtab[i].found=1;
					} else {
						xfree(ci);
						write_log("%d: can't parse '%s %s'",  line, p, t);
						rc=0;
					}
				} else {
					write_log("%d: unknown keyword '%s'", line, p);
					rc=0;
				}
			}
					
		}
	}
	fclose(f);
	return rc;
}

#ifdef NEED_DEBUG
void dumpconfig()
{
	char buf[MAX_STRING];
	cfgitem_t *c;
	int i;
	slist_t *sl;
	falist_t *al;
	faslist_t *fasl;

	for(i=0;i<CFG_NNN;i++) {
		write_log("*** %s, type %d, required %d, found %d",
			   configtab[i].keyword,configtab[i].type,
			   configtab[i].required,configtab[i].found);
		for(c=configtab[i].items;c;c=c->next) {
			snprintf(buf,MAX_STRING,"  when '%s': ", c->condition);
			switch(configtab[i].type) {
			case C_PATH:    
			case C_STR:
				snprintf(buf,MAX_STRING,"%s'%s'", buf,c->value.v_char);break;
			case C_STRL:
				for(sl=c->value.v_sl;sl;sl=sl->next)
					snprintf(buf,MAX_STRING,"%s'%s',",buf,sl->str);
				snprintf(buf,MAX_STRING,"%s%%",buf);
				break;
			case C_ADRSTRL: 
				for(fasl=c->value.v_fasl;fasl;fasl=fasl->next)
					snprintf(buf,MAX_STRING,"%s%s '%s',",buf,ftnaddrtoa(&fasl->addr), fasl->str);
				snprintf(buf,MAX_STRING,"%s%%",buf);
				break;
			case C_ADDRL:
				for(al=c->value.v_al;al;al=al->next)
					snprintf(buf,MAX_STRING,"%s%s,",buf,ftnaddrtoa(&al->addr));
				snprintf(buf,MAX_STRING,"%s%%",buf);
				break;
			case C_INT:     snprintf(buf,MAX_STRING,"%s%d", buf, c->value.v_int);break;
			case C_OCT:     snprintf(buf,MAX_STRING,"%s%o", buf, c->value.v_int);break;
			case C_YESNO:   snprintf(buf,MAX_STRING,"%s%s", buf, c->value.v_int?"yes":"no");break;
			}
		}
		write_log("%s",buf);
	}
}
#endif

void killconfig()
{
	int i;
	cfgitem_t *c, *t;
	slist_kill(&condlist);
	for(i=0;i<CFG_NNN;i++) {
		c=configtab[i].items;
		while(c) {
			t=c->next;
			switch(configtab[i].type) {
			case C_PATH:    
			case C_STR:	
				if(c->value.v_char) xfree(c->value.v_char);
				else c->value.v_char=NULL;
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
			xfree(c);
			c=t;
		}
		configtab[i].items=NULL;
		configtab[i].found=0;
	}
}
