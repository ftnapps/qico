/**********************************************************
 * File: ftn.c
 * Created at Thu Jul 15 16:11:27 1999 by pk // aaz@ruxy.org.ru
 * ftn tools
 * $Id: ftn.c,v 1.1 2000/07/18 12:37:19 lev Exp $
 **********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "ftn.h"
#include "qconf.h"

unsigned long seq=0xFFFFFFFF;

int parseftnaddr(char *s, ftnaddr_t *a, ftnaddr_t *b, int wc)
{
	char *p=s, *pn;
	int n=-1,wn=0, pq=1;

	if(!s || !*s) return 0;
	
	if(b) {	a->z=b->z;a->n=b->n;a->f=b->f;a->p=(wc?-1:0); }
	else { a->z=a->n=a->f=a->p=(wc?-1:0); }
	pn=p;
	while(*p && pq) {
		if(isdigit(*p))
			wn=1;
		else if(wc && *p=='*') {
			wn=2;
		} else
			switch(*p) {
			case ':':
				if(!wn || n>0) return 0;
				a->z=(wn<2)?atoi(pn):-1;
				pn=p+1;wn=0;n=1;
				break;
			case '/':
				if(!wn || n>1) return 0;
				a->n=(wn<2)?atoi(pn):-1;
				n=2;pn=p+1;wn=0;
				break;
			case '.':
				if(n>2) return 0;
				if(wn) a->f=(wn<2)?atoi(pn):-1;
				n=3;pn=p+1;
				if(!wn && n<0) break;
				if(!wn) return 0;
				wn=0;
				break;
			case '\n':
			case '\r':
			case '\0':	
			case ' ':
			case '\t':
			case '@':
				pq=0;
				break; 
			default:
				return 0;
			}
		p++;
	}
	switch(n) {
	case 2:
		if(!wn) return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	case 3:
		if(!wn) return 0;
		a->p=(wn<2)?atoi(pn):-1;
		break;
	case -1:
		if(!wn) return 0;
		a->f=(wn<2)?atoi(pn):-1;
		break;
	default:
		return 0;
	} 
	return 1;
}

ftnaddr_t *akamatch(ftnaddr_t *a, falist_t *akas)
{
	ftnaddr_t *best;int m, bm;falist_t *i;
	bm=0;best=&akas->addr;
	for(i=akas;i;i=i->next) {
		m=0;
		if(i->addr.z==a->z) {
			m=1;
			if(i->addr.n==a->n) {
				m=2;
				if(i->addr.f==a->f) {
					m=3;
					if(i->addr.p==a->p)
						m=4;
				}
			}
		}
		if(m>bm) {
			bm=m;best=&i->addr;
		}
	}
	return best;
}

char *ftnaddrtoa(ftnaddr_t *a)
{
	static char s[50];
	if(a->p)
		sprintf(s, "%d:%d/%d.%d", a->z, a->n, a->f, a->p);
	else
		sprintf(s, "%d:%d/%d", a->z, a->n, a->f);
	return s;
}

char *ftnaddrtoia(ftnaddr_t *a)
{
	static char s[50];
	if(a->p)
		sprintf(s, "p%d.f%d.n%d.z%d", a->p, a->f, a->n, a->z);
	else
		sprintf(s, "f%d.n%d.z%d", a->f, a->n, a->z);
	return s;
}


void falist_add(falist_t **l, ftnaddr_t *a)
{
	falist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(falist_t *)malloc(sizeof(falist_t));
	(*t)->next=NULL;
	ADDRCPY((*t)->addr, (*a));
}

void falist_kill(falist_t **l)
{
	falist_t *t;
	while(*l) {
		t=(*l)->next;
		free(*l);
		*l=t;
	}
}

falist_t *falist_find(falist_t *l, ftnaddr_t *a)
{
	for(;l;l=l->next) if(ADDRCMP(l->addr, (*a))) return l;
	return NULL;
}

slist_t *slist_add(slist_t **l, char *s)
{
	slist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(slist_t *)malloc(sizeof(slist_t));
	(*t)->next=NULL;
	(*t)->str=strdup(s);
	return *t;
}

void slist_kill(slist_t **l)
{
	slist_t *t;
	while(*l) {
		t=(*l)->next;
		free((*l)->str);
		free(*l);
		*l=t;
	}
}


void faslist_add(faslist_t **l, char *s, ftnaddr_t *a)
{
	faslist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(faslist_t *)malloc(sizeof(faslist_t));
	(*t)->next=NULL;
	(*t)->str=strdup(s);
	ADDRCPY((*t)->addr, (*a));
}

void faslist_kill(faslist_t **l)
{
	faslist_t *t;
	while(*l) {
		t=(*l)->next;
		free((*l)->str);
		free(*l);
		*l=t;
	}
}

void strlwr(char *s)
{
	while(s && *s) { *s=tolower(*s);s++; }
}

void strupr(char *s)
{
	while(s && *s) { *s=toupper(*s);s++; };
}

void strtr(char *s, char a, char b)
{
	while(*s) {
		if(*s==a) *s=b;
		s++;
	}
}
	
char *chop(char *s,int n)
{
	int i=0;char *p=strchr(s,0);
	for(i=0;i<n;i++) *--p=0;
	return s;
}

unsigned long filesize(char *fname)
{
	int s;
	FILE *f=fopen(fname, "r");
	if(!f) return 0;
	fseek(f, 0L, SEEK_END);s=ftell(f);fclose(f);
	return s;
}

int lockpid(char *pidfn)
{
	FILE *f;pid_t pid;
	char tmpname[MAX_PATH], *p;
	int rc;

	f=fopen(pidfn, "rt");
	if(f) {
		fscanf(f, "%d", &pid);
		fclose(f);
		if(kill(pid, 0)&&(errno==ESRCH)) unlink(pidfn);
		else return 0;
	}

	strcpy(tmpname, pidfn);
	p=strrchr(tmpname, '/');if(!p) p=tmpname;
	sprintf(tmpname+(p-tmpname), "/QTEMP.%d", getpid());
	if ((f=fopen(tmpname,"w")) == NULL) return 0;
	fprintf(f,"%10d\n",getpid());
	fclose(f);
/* 	chmod(tmpname,0444); */
	unlink(pidfn);
	rc=link(tmpname,pidfn);
	if(rc) return 0;
	unlink(tmpname);
	return 1;
}
	
int islocked(char *pidfn)
{
	FILE *f;pid_t pid;

	f=fopen(pidfn, "rt");
	if(f) {
		fscanf(f, "%d", &pid);
		fclose(f);
		if(kill(pid, 0)&&(errno==ESRCH)) unlink(pidfn);
		else return pid;
	}
	return 0;
}
	
char *strip8(char *s)
{
	char *p=s;
	while (*s) {
	  if (s[0] == '}' || s[0] == ']') s[0] = 'x';
	  *s++&=0x7f;
	}
	return p;
}

unsigned long sequencer()
{
	if(seq==0xFFFFFFFF)
		seq=time(NULL);
	else
		if(time(NULL)>seq)
			seq=time(NULL);
		else seq++;
	return seq;
}


int has_addr(ftnaddr_t *a, falist_t *l)
{
	while(l) {
		if(ADDRCMP((*a), (l->addr))) return 1;
		l=l->next;
	}
	return 0;
}


/* int touch(char *fn) */
/* { */
/* 	FILE *f=fopen(fn, "a"); */
/* 	if(f) { */
/* 		fclose(f); */
/* 		return 1; */
/* 	} */
/* 	return 0; */
/* } */

int mkdirs(char *name)
{
	char *p,*q;int rc=0;
	p=name+1;
	while ((q=strchr(p,'/'))) {
		*q=0;rc=mkdir(name,cfgi(CFG_DIRPERM));*q='/';
		p=q+1;
	}
	return rc;
}

void rmdirs(char *name)
{
	int rc=0;
	char *q;
	q=strrchr(name,'/');
	while(q && q!=name && !rc) {
		*q=0;rc=rmdir(name);/* log("rmdir %s", name); */
		q=strrchr(name,'/');*q='/';
	}
}

FILE *mdfopen(char *fn, char *pr)
{
	FILE *f=fopen(fn,pr);
	if(f) return f;
	if(errno==ENOENT) {
		mkdirs(fn);
		return fopen(fn,pr);
	}
	return NULL;
}

char *engms[]={"Jan","Feb","Mar","Apr","May","Jun",
			   "Jul","Aug","Sep","Oct","Nov","Dec"};

FILE *openpktmsg(ftnaddr_t *fa, ftnaddr_t *ta, char *from, char *to,
				 char *subj, char *pwd, char *fn)
{
	FILE *f;
	pkthdr_t ph;pktmhdr_t mh;
	time_t tim=time(NULL);
	struct tm *t=localtime(&tim);
	
	f=fopen(fn,"w");
	if(!f) return NULL;
	bzero(&ph, sizeof(ph));bzero(&mh, sizeof(mh));
	ph.phONode=fa->f;ph.phDNode=ta->f;ph.phONet=fa->n;ph.phDNet=ta->n;
	ph.phYear=t->tm_year+1900;ph.phMonth=t->tm_mon;ph.phDay=t->tm_mday;
	ph.phHour=t->tm_hour;ph.phMinute=t->tm_min;ph.phSecond=t->tm_sec;
	ph.phBaud=0;ph.phPCode=0xFE;ph.phType=2;
	ph.phAuxNet=0;ph.phCWValidate=0x100;ph.phPCodeHi=1;ph.phPRevMinor=2;
	ph.phCaps=1;		
	memset(ph.phPass,' ',8);if(pwd) memcpy(ph.phPass, pwd, strlen(pwd));
	ph.phQOZone=fa->z;ph.phQDZone=ta->z;
	ph.phOZone=fa->z;ph.phDZone=ta->z;
	ph.phOPoint=fa->p;ph.phDPoint=ta->p;
	fwrite(&ph, sizeof(ph), 1, f);
	mh.pmONode=fa->f;mh.pmDNode=ta->f;mh.pmONet=fa->n;mh.pmDNet=ta->n;
	mh.pmAttr=1;mh.pmType=2;
	fwrite(&mh, sizeof(mh), 1, f);
	fprintf(f, "%02d %3s %02d  %02d:%02d:%02d", t->tm_mday,
			engms[t->tm_mon], t->tm_year%100, t->tm_hour, t->tm_min,
			t->tm_sec);fputc(0,f);
	fwrite(from, strlen(from)+1, 1, f);fwrite(to, strlen(to)+1, 1, f);
	fwrite(subj, strlen(subj)+1, 1, f);
	if(fa->p) fprintf(f, "\001FMPT %d\r", fa->p);
	if(ta->p) fprintf(f, "\001TOPT %d\r", ta->p);
	fprintf(f, "\001INTL %d:%d/%d %d:%d/%d\r",
			ta->z,ta->n,ta->f,fa->z,fa->n,fa->f);
	fprintf(f, "\001MSGID: %s %08lx\r", ftnaddrtoa(fa), sequencer());
	return f;
}

void closepkt(FILE *f, ftnaddr_t *fa, char *tear, char *orig)
{
	fprintf(f,"--- %s\r * Origin: %s (%s)\r", tear, orig, ftnaddrtoa(fa));
	fputc(0,f);fputc(0,f);fputc(0,f);fclose(f);
}

#ifndef FREE_BSD
#ifdef PROCTITLE
/*
 * clobber argv so ps will show what we're doing.
 * (stolen from BSD ftpd where it was stolen from sendmail)
 * warning, since this is usually started from inetd.conf, it
 * often doesn't have much of an environment or arglist to overwrite.
 */

static char *cmdstr=NULL;
static char *cmdstrend=NULL;

void setargspace(char **argv, char **envp)
{
	cmdstr=argv[1];
	while (*envp) envp++;
	envp--;
	cmdstrend=(*envp)+strlen(*envp);
}

void setproctitle(char *str)
{
	char *p;
	/* make ps print our process name */
	if(!cmdstr) return;
	for (p=cmdstr;(p < cmdstrend) && (*str);p++,str++) *p=*str;
	while (p < cmdstrend) *p++ = ' ';
}

#else

void setargspace(char **argv, char **envp){}
void setproctitle(char *str){}

#endif
#endif /* FREE_BSD */

char *xstrcpy(char **to, char *from)
{
	if(*to) free(*to);
	return *to=strdup(from?from:"");
}

char *xstrncpy(char **to, char *from, int n)
{
	if(*to) free(*to);
	*to=malloc(MIN(n,strlen(from))+1);
	strncpy(*to, from, n);
	return *to;
}

char *xstrcat(char **to, char *from)
{
	if(!from) return *to;
	if(!*to) return xstrcpy(to, from);
	*to=realloc(*to, strlen(*to)+strlen(from)+1);
	strcat(*to, from);
	return *to;
}

#ifndef HAS_BASENAME
char *basename (const char *filename)
{
  char *p = strrchr (filename, '/');
  return p ? p + 1 : (char *) filename;
}                                                                               
#endif

int fexist(char *s)
{
	struct stat sb;
	return !stat(s, &sb) &&	S_ISREG(sb.st_mode);
}

char dos_allowed[]="-!~$()_";
char *fnc(char *s)
{
	static char s8[13];
	char *p,*q, *t, i;
	t=strdup(s);
	s=strrchr(t, '.');if(!s) s=t+strlen(t);
	for(p=t,q=t;p<s;p++) 
		if((tolower(*p)<='z' && tolower(*p)>='a') ||
		   (*p<='9' && *p>='0') || strchr(dos_allowed, *p)) 
			*q++=tolower(*p);
	*q=0;
	if(strlen(t)<=8) strcpy(s8, t);else {
		memcpy(s8, t, 4);memcpy(s8+4, q-4, 4);s8[8]=0;
	}
	s++;
	if(*s) {
	    strcat(s8,".");q=s8+strlen(s8);i=0;
	    for(;*s && i<3;s++) 
		    if((tolower(*s)<='z' && tolower(*s)>='a') ||
		       (*s<='9' && *s>='0') || strchr(dos_allowed, *s)) 
			    q[i++]=tolower(*s);
	    q[i]=0;
	}
	free(t);
	return s8;
}
	
int whattype(char *fn)
{
	char *ext[] = {"su","mo","tu","we","th","fr","sa","pkt","req"};
	int i;char *p;
	if(!fn) return IS_ERR;
	p=strrchr(fn,'.');
	if(!p) return IS_FILE;
	for(i=0;i<9;i++)
		if(!strncasecmp(p+1,ext[i],strlen(ext[i])))
			switch(i) {
			case 7: return IS_PKT;
			case 8: return IS_REQ;
			default: return IS_ARC;
			}
	return IS_FILE;
}                                           
		
int lunlink(char *s)
{
	int rc=unlink(s);
	if(rc<0 && errno!=ENOENT)
		log("can't delete file %s: %s", s, strerror(errno));
	return rc;
}
			
char *mapname(char *fn, char *map)
{
	char *l;
	if(!map) return fn;
	if(strchr(map, 'd')) {
		l=strrchr(fn, '.');if(l) {
			strtr(fn,'.','_');*l='.';
		}
	}
	if(strchr(map, 'b') && whattype(fn)!=IS_FILE) 
		sprintf(fn, "%08lx%s", crc32s(fn), strrchr(fn,'.'));
	if(strchr(map, 'u')) strupr(fn);
	if(strchr(map, 'l')) strlwr(fn);
	if(strchr(map, 'f')) strcpy(fn, fnc(fn));
	return fn;
}	
