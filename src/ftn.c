/**********************************************************
 * ftn tools
 * $Id: ftn.c,v 1.20 2004/03/24 17:50:04 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <fnmatch.h>
#include "crc.h"

/* domain name for translate ftn addr to inet host. */
#define FTNDOMAIN "fidonet.net"

void addr_cpy(ftnaddr_t *a,ftnaddr_t *b)
{
	if(!a||!b)return;
	a->z=b->z;a->n=b->n;
	a->f=b->f;a->p=b->p;
	if(b->d&&*b->d)a->d=xstrdup(b->d);
	    else a->d=NULL;
}

int addr_cmp(ftnaddr_t *a,ftnaddr_t *b)
{
	if(!a||!b)return 0;
	if(a->z==b->z&&a->n==b->n&&a->f==b->f&&a->p==b->p)return 1;
	return 0;
}

int parseftnaddr(char *s,ftnaddr_t *a,ftnaddr_t *b,int wc)
{
	char *p=s,*pn;
	int n=-1,wn=0,pq=1;
	if(!s||!*s)return 0;
	if(b) {
		addr_cpy(a,b);
		a->p=wc?-1:0;
	} else a->z=a->n=a->f=a->p=wc?-1:0,a->d=NULL;
	for(pn=p;*p&&pq;p++) {
		if(isdigit((int)*p))wn=1;
		else if(wc&&*p=='*')wn=2;
		else switch(*p) {
			case ':':
				if(!wn||n>0)return 0;
				a->z=(wn<2)?atoi(pn):-1;
				pn=p+1;wn=0;n=1;
				break;
			case '/':
				if(!wn||n>1)return 0;
				a->n=(wn<2)?atoi(pn):-1;
				n=2;pn=p+1;wn=0;
				break;
			case '.':
				if(n>2)return 0;
				if(wn)a->f=(wn<2)?atoi(pn):-1;
				n=3;pn=p+1;
				if(!wn&&n<0)break;
				if(!wn)return 0;
				wn=0;
				break;
			case '@':
				if(p[1]) {
					xfree(a->d);
					a->d=xstrdup(p+1);
				}
			case '\n': case '\r':
			case ' ': case '\t':
			case '\0':
				pq=0;
				break;
			default:return 0;
		}
	}
	switch(n) {
	    case 2:
		if(!wn)return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	    case 3:
		if(!wn)return 0;
		a->p=(wn<2)?atoi(pn):-1;
		break;
	    case -1:
		if(!wn)return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	    default:
		return 0;
	}
	return 1;
}

ftnaddr_t *akamatch(ftnaddr_t *a,falist_t *akas)
{
	int m,bm=0;
	falist_t *i;
	ftnaddr_t *best=&akas->addr;
	for(i=akas;i;i=i->next) {
		m=0;
		if(i->addr.z==a->z) {
			m=1;
			if(i->addr.n==a->n) {
				m=2;
				if(i->addr.f==a->f) {
					m=3;
					if(i->addr.p==a->p)m=4;
				}
			}
		}
		if(m>bm) {
			bm=m;
			best=&i->addr;
		}
	}
	return best;
}

char *ftnaddrtoa(ftnaddr_t *a)
{
	static char s[30];
	if(a->p)snprintf(s,30,"%d:%d/%d.%d",a->z,a->n,a->f,a->p);
	    else snprintf(s,30,"%d:%d/%d",a->z,a->n,a->f);
	return s;
}

char *ftnaddrtoda(ftnaddr_t *a)
{
	char *d=cfgs(CFG_DOMAIN);
	static char s[64];
	if(!*d)d="fidonet.org";
	if(*d=='@')d++;
	if(a->p)snprintf(s,64,"%d:%d/%d.%d@%s",a->z,a->n,a->f,a->p,a->d?a->d:d);
	    else snprintf(s,64,"%d:%d/%d@%s",a->z,a->n,a->f,a->d?a->d:d);
	return s;
}

char *ftnaddrtoia(ftnaddr_t *a)
{
	static char s[64];
	if(a->p)snprintf(s,64,"p%d.f%d.n%d.z%d." FTNDOMAIN,a->p,a->f,a->n,a->z);
	    else snprintf(s,64,"f%d.n%d.z%d." FTNDOMAIN,a->f,a->n,a->z);
	return s;
}

void falist_add(falist_t **l,ftnaddr_t *a)
{
	falist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(falist_t *)xmalloc(sizeof(falist_t));
	(*t)->next=NULL;
	addr_cpy(&(*t)->addr,a);
}

void falist_kill(falist_t **l)
{
	falist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->addr.d);
		xfree(*l);
		*l=t;
	}
}

falist_t *falist_find(falist_t *l,ftnaddr_t *a)
{
	for(;l;l=l->next)if(addr_cmp(&l->addr,a))return l;
	return NULL;
}

slist_t *slist_add(slist_t **l,char *s)
{
	slist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(slist_t *)xmalloc(sizeof(slist_t));
	(*t)->next=NULL;
	(*t)->str=xstrdup(s);
	return *t;
}

void slist_kill(slist_t **l)
{
	slist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->str);
		xfree(*l);
		*l=t;
	}
}

void faslist_add(faslist_t **l,char *s,ftnaddr_t *a)
{
	faslist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(faslist_t *)xmalloc(sizeof(faslist_t));
	(*t)->next=NULL;
	(*t)->str=xstrdup(s);
	addr_cpy(&(*t)->addr,a);
}

void faslist_kill(faslist_t **l)
{
	faslist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->addr.d);
		xfree((*l)->str);
		xfree(*l);
		*l=t;
	}
}

char *strip8(char *s)
{
	int i=0;
	unsigned char t;
	char buf[MAX_STRING+1],*ss=s;
	if(*s)recode_to_remote(s);
	while(*s&&i<MAX_STRING) {
		t=*s;
		if(t>127) {
			buf[i++]='\\';
			buf[i++]=t/16+((t/16)>9?'a'-10:'0');
			buf[i]=t%16+((t%16)>9?'a'-10:'0');
		} else buf[i]=t;
		if(buf[i]=='}'||buf[i]==']')buf[i]=')';
		i++;s++;
	}
	buf[i]=0;
	restrcpy(&ss,buf);
	return ss;
}

int has_addr(ftnaddr_t *a,falist_t *l)
{
	while(l) {
		if(addr_cmp(a,&l->addr))return 1;
		l=l->next;
	}
	return 0;
}

char *engms[13]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","Any"};

int showpkt(char *fn)
{
	FILE *f;
	int i,n=1;
	pkthdr_t ph;
	pktmhdr_t mh;
	char from[36],to[36],a;
	f=fopen(fn,"r");
	if(!f){write_log("can't open '%s' for reading: %s",fn,strerror(errno));return 0;}
	if(fread(&ph,sizeof(ph),1,f)!=1)write_log("packet read error");
	    else if(I2H16(ph.phType)!=2)write_log("packet isn't 2+ format");
		else {
		    while(fread(&mh,sizeof(mh),1,f)==1) {
			i=0;while(fgetc(f)>0&&i<30)i++;i=0;
			if(i>=30)break;
			while((a=fgetc(f))>0&&i<36)to[i++]=a;
			if(i>=36)break;
			to[i]=0;i=0;
			while((a=fgetc(f))>0&&i<36)from[i++]=a;
			if(i>=32)break;
			from[i]=0;i=0;
			while(fgetc(f)>0&&i<72)i++;
			if(i>=72)break;
			while(fgetc(f)>0);
			write_log(" *msg:%d from: \"%s\", to: \"%s\"",n++,from,to);
		    }
	}
	fclose(f);
	return 0;
}

FILE *openpktmsg(ftnaddr_t *fa,ftnaddr_t *ta,char *from,char *to,char *subj,char *pwd,char *fn,unsigned attr)
{
	FILE *f;
	pkthdr_t ph;
	pktmhdr_t mh;
	time_t tim=time(NULL);
	struct tm *t=localtime(&tim);
	if(!(f=fopen(fn,"w")))return NULL;
	memset(&ph,0,sizeof(ph));
	memset(&mh,0,sizeof(mh));
	ph.phONode=H2I16(fa->f);
	ph.phDNode=H2I16(ta->f);
	ph.phONet=H2I16(fa->n);
	ph.phDNet=H2I16(ta->n);
	ph.phYear=H2I16(t->tm_year+1900);
	ph.phMonth=H2I16(t->tm_mon);
	ph.phDay=H2I16(t->tm_mday);
	ph.phHour=H2I16(t->tm_hour);
	ph.phMinute=H2I16(t->tm_min);
	ph.phSecond=H2I16(t->tm_sec);
	ph.phBaud=H2I16(0);
	ph.phPCode=0xFE;
	ph.phType=H2I16(2);
	ph.phAuxNet=H2I16(0);
	ph.phCWValidate=H2I16(0x100);
	ph.phPCodeHi=1;
	ph.phPRevMinor=2;
	ph.phCaps=H2I16(1);
	if(pwd)memcpy(ph.phPass,pwd,MAX(strlen(pwd),8));
	ph.phQOZone=H2I16(fa->z);
	ph.phQDZone=H2I16(ta->z);
	ph.phOZone=H2I16(fa->z);
	ph.phDZone=H2I16(ta->z);
	ph.phOPoint=H2I16(fa->p);
	ph.phDPoint=H2I16(ta->p);
	fwrite(&ph,sizeof(ph),1,f);
	mh.pmONode=H2I16(fa->f);
	mh.pmDNode=H2I16(ta->f);
	mh.pmONet=H2I16(fa->n);
	mh.pmDNet=H2I16(ta->n);
	mh.pmAttr=H2I16(attr);
	mh.pmType=H2I16(2);
	fwrite(&mh,sizeof(mh),1,f);
	fprintf(f,"%02d %3s %02d  %02d:%02d:%02d%c",t->tm_mday,engms[t->tm_mon],t->tm_year%100,t->tm_hour,t->tm_min,t->tm_sec,0);
	if(cfgi(CFG_RECODEPKTS)) {
		recode_to_remote(to);
		if(attr<128) {
			recode_to_remote(subj);
			recode_to_remote(from);
		}
	}
	fwrite(to,strlen(to)+1,1,f);
	fwrite(from,strlen(from)+1,1,f);
	fwrite(subj,strlen(subj)+1,1,f);
	fprintf(f,"\001INTL %d:%d/%d %d:%d/%d\r",ta->z,ta->n,ta->f,fa->z,fa->n,fa->f);
	if(fa->p)fprintf(f,"\001FMPT %d\r",fa->p);
	if(ta->p)fprintf(f,"\001TOPT %d\r",ta->p);
	fprintf(f,"\001MSGID: %s %08lx\r",ftnaddrtoa(fa),sequencer());
	fprintf(f,"\001PID: %s %s/%s\r",progname,version,osname);
	return f;
}

void closepkt(FILE *f,ftnaddr_t *fa,char *tear,char *orig)
{
	if(cfgi(CFG_RECODEPKTS))recode_to_remote(orig);
	fprintf(f,"--- %s\r * Origin: %s (%s)\r%c%c%c",tear,orig,ftnaddrtoa(fa),0,0,0);
	fclose(f);
}

char *qver(int w)
{
	cfgs(CFG_PROGNAME);
	if(!w) {
		if(ccs)if(strncasecmp(ccs,progname,4))return ccs;
		return progname;
	} else if(w==1) {
		if(ccs)if(strncasecmp(ccs,progname,4))
		    if(cfgs(CFG_VERSION))return ccs;
		return version;
	} else return(cfgs(CFG_OSNAME)?ccs:osname);
}

void closeqpkt(FILE *f,ftnaddr_t *fa)
{
	char str[MAX_STRING];
	snprintf(str,MAX_STRING*4,"%s-%s/%s",qver(0),qver(1),qver(2));
	closepkt(f,fa,str,cfgs(CFG_STATION));
}

int whattype(char *fn)
{
	static char *ext[]={"su","mo","tu","we","th","fr","sa","pkt","req"};
	int i,l;
	char *p,low;
	if(!fn)return IS_ERR;
	p=strrchr(fn,'.');
	if(!p)return IS_FILE;
	l=strlen(++p);
	if(l!=3)return IS_FILE;
	for(i=0;i<9;i++)if(!strncasecmp(p,ext[i],strlen(ext[i])))
	    switch(i) {
		case 7: return IS_PKT;
		case 8: return IS_REQ;
		default:
			low=tolower(p[2]);
			if((low>='0'&&low<='9')||(low>='a'&&low<='z'))return IS_ARC;
			break;
	}
	return IS_FILE;
}

int istic(char *fn)
{
	char *p;
	if(!fn)return 0;
	p=strrchr(fn,'.');
	if(!p)return 0;
	if(!strncasecmp(p+1,"tic",3))return 1;
	    else return 0;
}

char *mapname(char *fn,char *map,size_t size)
{
	int t;
	char *l;
	if(!map)return fn;
	DEBUG(('S',3,"map : '%s', infname: '%s'",map,fn));
	if(strchr(map,'c'))recode_to_remote(fn);
	if(strchr(map,'k'))recode_to_local(fn);
	if(strchr(map,'d')) {
		if((l=strrchr(fn,'.'))) {
			strtr(fn,'.','_');
			*l='.';
		}
	}
	t=whattype(fn);
	if(strchr(map,'b')&&t!=IS_FILE) {
		l=strrchr(fn,'.');
		snprintf(fn,14,"%08lx%s",crc32s(fn),l);
	}
	if(strchr(map,'u'))strupr(fn);
	if(strchr(map,'l'))strlwr(fn);
	if(strchr(map,'f'))xstrcpy(fn,fnc(fn),size);
	switch(t) {
	    case IS_PKT:
		if(strchr(map,'p'))strlwr(fn);
		else if(strchr(map,'P'))strupr(fn);
		break;
	    case IS_ARC:
		if(strchr(map,'a'))strlwr(fn);
		else if(strchr(map,'A'))strupr(fn);
		break;
	    case IS_FILE:
		if(istic(fn)) {
			if(strchr(map,'t'))strlwr(fn);
			else if(strchr(map,'T'))strupr(fn);
		} else {
			if(isdos83name(fn)) {
				if(strchr(map,'o'))strlwr(fn);
				else if(strchr(map,'O'))strupr(fn);
			}
		}
		break;
	}
	if(bink)strtr(fn,' ','_');
	DEBUG(('S',3,"mapex: '%s'",fn));
	return fn;
}

int havestatus(int status,int cfgkey)
{
	static int stc[]={Q_NORM,Q_HOLD,Q_DIR,Q_CRASH,Q_IMM,Q_REQ};
	static char stl[]=Q_CHARS;
	int i;
	char *callon=cfgs(cfgkey);
	for(i=0;i<F_MAX;i++)if((status&stc[i])&&(strchr(callon,stl[i])))return 1;
	return 0;
}

int needhold(int status,int what)
{
	status&=Q_ANYWAIT;
	if(status&Q_WAITA)return 1;
	if((status&Q_WAITR)&&!(what&(~T_REQ)))return 1;
	if((status&Q_WAITX)&&!(what&(~T_ARCMAIL)))return 1;
	return 0;
}

int xfnmatch(char *pat,char *name,int flags)
{
	int type,rc=0,q=0;
	if(!name||!pat)return FNM_NOMATCH;
	type=whattype(name);
	if(*pat=='!'){pat++;rc=1;}
	if(*pat=='%') {
		switch(pat[1]) {
		    case 'F':
			if(type==IS_FILE)q=1;
			break;
		    case 'N':
			if(type==IS_PKT)q=1;
			break;
		    case 'E':
			if(type==IS_ARC)q=1;
			break;
		    default:
			write_log("error: mask '%s': unknown macro '%c'",pat-rc,pat[1]);
			return FNM_NOMATCH;
		}
	} else q=!fnmatch(pat,name,flags);
	return((rc^q)?0:FNM_NOMATCH);
}

char *findpwd(ftnaddr_t *a)
{
	faslist_t *cf;
	for(cf=cfgfasl(CFG_PASSWORD);cf;cf=cf->next)
		if(addr_cmp(&cf->addr,a))return cf->str;
	return NULL;
}
