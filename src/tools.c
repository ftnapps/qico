/**********************************************************
 * stuff
 * $Id: tools.c,v 1.2 2004/02/06 21:54:46 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "charset.h"

static unsigned long seq=0xFFFFFFFF;

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

int touch(char *fn)
{
	FILE *f=fopen(fn, "a");
 	if(f) {
 		fclose(f);
 		return 1;
 	} else write_log("can't touch '%s': %s",fn,strerror(errno));
 	return 0;
}

int mkdirs(char *name)
{
	char *p,*q;int rc=0;
	p=name+1;
	while ((q=strchr(p,'/'))&&!rc) {
		*q=0;rc=mkdir(name,cfgi(CFG_DIRPERM));*q='/';
		p=q+1;
	}
	return rc;
}

void rmdirs(char *name)
{
	int rc=0;
	char *q,*t;
	q=strrchr(name,'/');
	while(q&&q!=name&&!rc) {
		*q=0;rc=rmdir(name);
		t=strrchr(name,'/');
		*q='/';q=t;
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

int fexist(char *s)
{
	struct stat sb;
	return !stat(s, &sb) &&	S_ISREG(sb.st_mode);
}

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
	else if (strstr(s,".html")) xstrcat (s8, ".htm", 14);
	else if (strstr(s,".jpeg")) xstrcat (s8, ".jpg", 14);
	else if (strstr(s,".desc")) xstrcat (s8, ".dsc", 14);
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

int lunlink(char *s)
{
	int rc=unlink(s);
	if(rc<0 && errno!=ENOENT)
		write_log("can't delete file %s: %s", s, strerror(errno));
	return rc;
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

#ifndef HAVE_SETPROCTITLE

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
