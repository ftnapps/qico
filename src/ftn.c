/**********************************************************
 * ftn tools
 * $Id: ftn.c,v 1.1.1.1 2003/07/12 21:26:35 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "charset.h"

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
		if(isdigit((int)*p))
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
		snprintf(s, 50, "%d:%d/%d.%d", a->z, a->n, a->f, a->p);
	else
		snprintf(s, 50, "%d:%d/%d", a->z, a->n, a->f);
	return s;
}
/*
char *ftnaddrtoia(ftnaddr_t *a)
{
	static char s[50];
	if(a->p)
		snprintf(s, 50, "p%d.f%d.n%d.z%d", a->p, a->f, a->n, a->z);
	else
		snprintf(s, 50, "f%d.n%d.z%d", a->f, a->n, a->z);
	return s;
}
*/

void falist_add(falist_t **l, ftnaddr_t *a)
{
	falist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(falist_t *)xmalloc(sizeof(falist_t));
	(*t)->next=NULL;
	ADDRCPY((*t)->addr, (*a));
}

void falist_kill(falist_t **l)
{
	falist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree(*l);
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

void faslist_add(faslist_t **l, char *s, ftnaddr_t *a)
{
	faslist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(faslist_t *)xmalloc(sizeof(faslist_t));
	(*t)->next=NULL;
	(*t)->str=xstrdup(s);
	ADDRCPY((*t)->addr, (*a));
}

void faslist_kill(faslist_t **l)
{
	faslist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree((*l)->str);
		xfree(*l);
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
	while(s&&*s) {
		if(*s==a) *s=b;
		s++;
	}
}

unsigned char todos(unsigned char c)
{
	if(cfgi(CFG_REMOTERECODE)&&c>=128)c=tab_koidos[c-128];
	return c;
}

unsigned char tokoi(unsigned char c)
{
	if(cfgi(CFG_REMOTERECODE)&&c>=128)c=tab_doskoi[c-128];
	return c;
}

void stodos(unsigned char *s)
{
	while(s&&*s){*s=todos(*s);s++;}
}

void stokoi(unsigned char *s)
{
	while(s&&*s){*s=tokoi(*s);s++;}
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
	FILE *f;long pid;
	char tmpname[MAX_PATH], *p;
	int rc;

	f=fopen(pidfn, "rt");
	if(f) {
		fscanf(f, "%ld", &pid);
		fclose(f);
		if(kill(pid, 0)&&(errno==ESRCH)) unlink(pidfn);
		else return 0;
	}

#ifndef LOCKSTYLE_OPEN
	xstrcpy(tmpname, pidfn, MAX_PATH);
	p=strrchr(tmpname, '/');if(!p) p=tmpname;
	snprintf(tmpname+(p-tmpname), MAX_PATH-(p-tmpname+1), "/QTEMP.%ld", (long)getpid());
	if ((f=fopen(tmpname,"w")) == NULL) return 0;
	fprintf(f,"%10ld\n",(long)getpid());
	fclose(f);
	rc=link(tmpname,pidfn);
	unlink(tmpname);
	if(rc) return 0;
#else
	rc=open(pidfn,O_WRONLY|O_CREAT|O_EXCL,0644);
	if(rc<0) return 0;
	snprintf(tmpname,MAX_PATH,"%10ld\n",(long)getpid());
	write(rc,tmpname,strlen(tmpname));
	close(rc);
#endif
	return 1;
}
	
int islocked(char *pidfn)
{
	FILE *f;long pid;

	f=fopen(pidfn, "rt");
	if(f) {
		fscanf(f, "%ld", &pid);
		fclose(f);
		if(kill(pid, 0)&&(errno==ESRCH)) unlink(pidfn);
		else return pid;
	}
	return 0;
}
	
char *strip8(char *s)
{
	unsigned char *p=(unsigned char*)s,buf[128],t;
	int i=0;
	while(*p) {
		t=todos(*p);
		if(t>127)
		{
			buf[i++]='\\';
			buf[i++]=t/16+((t/16)>9?'a'-10:'0');
			buf[i]=t%16+((t%16)>9?'a'-10:'0');
		} else buf[i]=t;
		if(buf[i]=='}'||buf[i]==']')buf[i]=')';
		i++;p++;
	}
	*s=0;buf[i]=0;
	strcat(s,(char*)buf);
	return s;
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

/*
int touch(char *fn)
{
	FILE *f=fopen(fn, "a");
 	if(f) {
 		fclose(f);
 		return 1;
 	}
 	return 0;
} */

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
		*q=0;rc=rmdir(name);/* write_log("rmdir %s", name); */
		q=strrchr(name,'/');*q='/';
	}
}

FILE *mdfopen(char *fn, char *pr)
{
	FILE *f;
	struct stat sb;
	int nf=(stat(fn,&sb))?1:0;

 	f=fopen(fn,pr);
	if(f) {
		if(nf) fchmod(fileno(f), cfgi(CFG_DEFPERM));
		return f;
	}
	if(errno==ENOENT) {
		mkdirs(fn);
		f=fopen(fn,pr);
		if(f&&nf) fchmod(fileno(f), cfgi(CFG_DEFPERM));
		return f;
	}
	return NULL;
}

char *engms[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

FILE *openpktmsg(ftnaddr_t *fa, ftnaddr_t *ta, char *from, char *to,char *subj, char *pwd, char *fn,unsigned attr)
{
	FILE *f;
	pkthdr_t ph;pktmhdr_t mh;
	time_t tim=time(NULL);
	struct tm *t=localtime(&tim);

	f=fopen(fn,"w");
	if(!f) return NULL;
	memset(&ph,0, sizeof(ph));
	memset(&mh,0, sizeof(mh));
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
	fwrite(&ph, sizeof(ph), 1, f);
	mh.pmONode=H2I16(fa->f);
	mh.pmDNode=H2I16(ta->f);
	mh.pmONet=H2I16(fa->n);
	mh.pmDNet=H2I16(ta->n);
	mh.pmAttr=H2I16(attr);
	mh.pmType=H2I16(2);
	fwrite(&mh, sizeof(mh), 1, f);
	fprintf(f, "%02d %3s %02d  %02d:%02d:%02d%c", t->tm_mday, engms[t->tm_mon], t->tm_year%100, t->tm_hour, t->tm_min, t->tm_sec, 0);
	stodos((unsigned char*)to);
	if(attr<128){stodos((unsigned char*)subj);stodos((unsigned char*)from);}
	fwrite(to, strlen(to)+1, 1, f);
	fwrite(from, strlen(from)+1, 1, f);
	fwrite(subj, strlen(subj)+1, 1, f);
	fprintf(f, "\001INTL %d:%d/%d %d:%d/%d\r",ta->z,ta->n,ta->f,fa->z,fa->n,fa->f);
	if(fa->p) fprintf(f, "\001FMPT %d\r", fa->p);
	if(ta->p)fprintf(f, "\001TOPT %d\r", ta->p);
	fprintf(f, "\001MSGID: %s %08lx\r", ftnaddrtoa(fa), sequencer());
	fprintf(f, "\001PID: %s %s/%s\r",progname,version,osname);
	return f;
}

void closepkt(FILE *f, ftnaddr_t *fa, char *tear, char *orig)
{
	stodos((unsigned char*)orig);
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
		if(ccs)if(strncasecmp(ccs,progname,4))if(cfgs(CFG_VERSION))return ccs;
		return version;
	} else return(cfgs(CFG_OSNAME)?ccs:osname);
}

void closeqpkt(FILE *f,ftnaddr_t *fa)
{
	char str[MAX_STRING];
	snprintf(str,MAX_STRING*4,"%s-%s/%s",strdup(qver(0)),strdup(qver(1)),strdup(qver(2)));
	closepkt(f,fa,str,cfgs(CFG_STATION));
}

#ifndef HAVE_SETPROCTITLE
/*
 * clobber argv so ps will show what we're doing.
 * (stolen from BSD ftpd where it was stolen from sendmail)
 * warning, since this is usually started from inetd.conf, it
 * often doesn't have much of an environment or arglist to overwrite.
 */

extern char **environ;

static char *cmdstr=NULL;
static char *cmdstrend=NULL;

void setargspace(int argc, char **argv, char **envp)
{
	int i = 0;

	cmdstr=argv[0];

	while(envp[i]) i++;
	environ = xmalloc(sizeof(char*)*(i+1));
	i = 0;
	while(envp[i]) {
		environ[i] = xstrdup(envp[i]);
		i++;
	}
	environ[i] = NULL;

	cmdstrend = argv[0]+strlen(argv[0]);
	for(i=1;i<argc;i++)
		if(cmdstrend+1==argv[i]) cmdstrend = argv[i]+strlen(argv[i]);
	for(i=0;envp[i];i++)
		if(cmdstrend+1==envp[i]) cmdstrend = envp[i]+strlen(envp[i]);
}

void setproctitle(char *str)
{
	char *p;
	/* make ps print our process name */
	if(!cmdstr) return;
	for (p=cmdstr;(p < cmdstrend) && (*str);p++,str++) *p=*str;
	while (p < cmdstrend) *p++ = ' ';
}

#endif

int fexist(char *s)
{
	struct stat sb;
	return !stat(s, &sb) &&	S_ISREG(sb.st_mode);
}

char dos_allowed[]="-!~$()_";

int dosallowin83(int c)
{
	static char dos_allow[] = "!@#$%^&()~`'-_{}";

	if((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		strchr(dos_allow,c)) return 1;
	return 0;
}

char *fnc(char *s)
{
	static char s8[13];
	char *p, *q;
	unsigned int i=0;

	if (!s) return NULL;

	if (NULL == (p=strrchr(s,'/'))) p=s; else s=p;
	while (*p && *p!='.' && i<8) {
		if (dosallowin83(*p)) s8[i++]=tolower(*p);
		p++;
	}
	s8[i]='\0';

	if (strstr(s,".tar.gz")) xstrcat (s8, ".tgz", 14);
	else if (strstr(s,".tar.bz2")) xstrcat (s8, ".tb2", 14);
	else {
		p=strrchr(s, '.');
		if (p) {
			xstrcat(s8, ".", 14);
			q=p+4;
			i=strlen(s8);
			while (*p && q>p) {
				if(dosallowin83(*p)) s8[i++]=tolower(*p);
				p++;
			}
			s8[i]='\0';
		}
	}
	return s8;
}
	
int whattype(char *fn)
{
	static char *ext[] = {"su","mo","tu","we","th","fr","sa","pkt","req"};
	int i, l;char *p;
	char low;
	if(!fn) return IS_ERR;
	p=strrchr(fn,'.');
	if(!p) return IS_FILE;
	p++;
	l=strlen(p);
	if(l != 3) return IS_FILE;
	for(i=0;i<9;i++)
		if(!strncasecmp(p,ext[i],strlen(ext[i])))
			switch(i) {
			case 7: return IS_PKT;
			case 8: return IS_REQ;
			default:
				low=tolower(p[2]);
				if((low >= '0' && low <= '9') || (low >= 'a' && low <= 'z')) return IS_ARC;
				break;
			}
	return IS_FILE;
}                                           

int istic(char *fn)
{
	char *p;
	if(!fn) return 0;
	p=strrchr(fn,'.');
	if(!p) return 0;
	if(!strncasecmp(p+1,"tic",3)) return 1;
	    else return 0;
}
		
int lunlink(char *s)
{
	int rc=unlink(s);
	if(rc<0 && errno!=ENOENT)
		write_log("can't delete file %s: %s", s, strerror(errno));
	return rc;
}
			
char *mapname(char *fn, char *map, size_t size)
{
	int t;
	char *l;
	if(!map) return fn;
	if(strchr(map, 'c'))stodos((unsigned char*)fn);
	if(strchr(map, 'k'))stokoi((unsigned char*)fn);
	if(strchr(map, 'd')) {
		l=strrchr(fn, '.');if(l) {
			strtr(fn,'.','_');*l='.';
		}
	}
	t=whattype(fn);
	if(strchr(map, 'b') && t!=IS_FILE) 
		snprintf(fn,14,"%08lx%s",crc32s(fn),strrchr(fn,'.'));
	if(strchr(map, 'u')) strupr(fn);
	if(strchr(map, 'l')) strlwr(fn);
	if(strchr(map, 'f')) xstrcpy(fn, fnc(fn), size);

	switch(t) {
	case IS_PKT:
		if(strchr(map,'p')) strlwr(fn);
		else if(strchr(map,'P')) strupr(fn);
		break;
	case IS_ARC:
		if(strchr(map,'a')) strlwr(fn);
		else if(strchr(map,'A')) strupr(fn);
		break;
	case IS_FILE:
		if(istic(fn)) {
			if(strchr(map,'t')) strlwr(fn);
			else if(strchr(map,'T')) strupr(fn);
		} else {
			if(isdos83name(fn)) {
				if(strchr(map,'o')) strlwr(fn);
				else if(strchr(map,'O')) strupr(fn);
			}
		}
		break;
	default:
		break;
	}

	return fn;
}	

int isdos83name(char *fn)
{
	int nl=0,el=0,ec=0,uc=0,lc=0,f=1;
	char *p=fn;
	while(*p) {
		if(!dosallowin83(*p)&&('.'!=*p)){f=0;break;}
	    	if('.'==*p)ec++;
	    	    else {
			if(!ec)nl++; else el++;
			if(isalpha((int)*p)) {
				if(isupper((int)*p))uc++;
				    else lc++;
			}
		}
	    	p++;
	}
	return (f&&ec<2&&el<4&&nl<9&&(!lc||!uc));
}

int havestatus(int status, int cfgkey)
{
	static int stc[]={Q_NORM,Q_HOLD,Q_DIR,Q_CRASH,Q_IMM,Q_REQ};
	static char stl[]=Q_CHARS;
	int i;
	char *callon=cfgs(cfgkey);
	for(i=0;i<F_MAX;i++) if((status & stc[i]) && (strchr(callon,stl[i]))) return 1;
	return 0;
}

int needhold(int status, int what)
{
	status&=Q_ANYWAIT;
	if(status&Q_WAITA) return 1;
	if((status&Q_WAITR)&&!(what&(~T_REQ))) return 1;
	if((status&Q_WAITX)&&!(what&(~T_ARCMAIL))) return 1;
	return 0;
}
