/**********************************************************
 * stuff
 * $Id: tools.c,v 1.15 2004/06/05 06:49:13 sisoft Exp $
 **********************************************************/
#include "headers.h"
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include "charset.h"

static unsigned long seq=0xFFFFFFFF;
char *hexdigits="0123456789abcdef";
char *engms[13]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
				    "Sep","Oct","Nov","Dec","Any"};
char *sigs[]={"","HUP","INT","QUIT","ILL","TRAP","IOT","BUS","FPE",
		"KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM"};

static int initcharset(char *name,unsigned char *tab)
{
	FILE *f;
	int rev=0;
	unsigned i,c;
	char buf[MAX_STRING];
	if(!name||!strcasecmp(name,"none"))return -1;
	if(!strcasecmp(name,"internal"))return 1;
	if(!strncasecmp(name,"revert",6)) {
		rev=1;name+=6;
		while(*name==' '||*name=='\t')name++;
		if(!*name)return -1;
	}
	if(!(f=fopen(name,"r"))) {
		write_log("can't open recode file '%s': %s",name,strerror(errno));
		return -1;
	}
	memset(tab,0,128);
	while(fgets(buf,MAX_STRING,f)) {
		if(!isdigit(*buf))continue;
		if(*buf=='0'&&buf[1]=='x') {
			if(sscanf(buf,"0x%x 0x%x",&i,&c)!=2)continue;
		} else {
			if(sscanf(buf,"%u %u",&i,&c)!=2)continue;
		}
		if(rev) { rev=i;i=c;c=rev; }
		if(c>255||i<128||i>255)continue;
		tab[i-128]=c;
	}
	for(rev=0;rev<128;rev++)if(!tab[rev])tab[rev]=rev+128;
	fclose(f);
	return 1;
}

void recode_to_remote(char *s)
{
	static int loaded=0;
	register unsigned char c;
	if(!loaded||!s)loaded=initcharset(cfgs(CFG_REMOTECP),ctab_remote);
	if(loaded==1&&s) {
		while(*s) {
			c=*(unsigned char*)s;
			if(c>=128)*(unsigned char*)s=ctab_remote[c-128];
			s++;
		}
	}
}

void recode_to_local(char *s)
{
	static int loaded=0;
	register unsigned char c;
	if(!loaded||!s)loaded=initcharset(cfgs(CFG_LOCALCP),ctab_local);
	if(loaded==1&&s) {
		while(*s) {
			c=*(unsigned char*)s;
			if(c>=128)*(unsigned char*)s=ctab_local[c-128];
			s++;
		}
	}
}

int hexdcd(char h,char l)
{
	l=tolower(l);h=tolower(h);
	if(l>='a'&&l<='f')l-=('a'-10);
	    else l-='0';
	if(h>='a'&&h<='f')h-=('a'-10);
	    else h-='0';
	return(h*16+l);
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

slist_t *slist_add(slist_t **l,char *s)
{
	slist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(slist_t *)xmalloc(sizeof(slist_t));
	(*t)->next=NULL;
	(*t)->str=xstrdup(s);
	return *t;
}

slist_t *slist_addl(slist_t **l,char *s)
{
	slist_t **t;
	for(t=l;*t;t=&((*t)->next));
	*t=(slist_t *)xmalloc(sizeof(slist_t));
	(*t)->next=NULL;
	(*t)->str=s;
	return *t;
}

char *slist_dell(slist_t **l)
{
	char *p=NULL;
	slist_t *t,*cc=NULL;
	for(t=*l;t&&t->next;cc=t,p=t->next->str,t=t->next);
	if(cc)xfree(cc->next);
	    else {
		xfree(t);
		*l=NULL;
	}
	return p;
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

void slist_killn(slist_t **l)
{
	slist_t *t;
	while(*l) {
		t=(*l)->next;
		xfree(*l);
		*l=t;
	}
}

void strbin2hex(char *s,const unsigned char *bptr,size_t blen)
{
	while(blen--) {
		*s++=hexdigits[(*bptr>>4)&0x0f];
		*s++=hexdigits[(*bptr)&0x0f];
		bptr++;
	}
	*s=0;
}

int strhex2bin(unsigned char *binptr,const char *string)
{
	int i,val,len=strlen(string);
	unsigned char *dest=binptr;
	const char *p;
	for(i=0;2*i<len;i++) {
		if((p=strchr(hexdigits,tolower(*(string++))))) {
			val=(int)(p-hexdigits);
			if((p=strchr(hexdigits,tolower(*(string++))))) {
				val=val*16+(int)(p-hexdigits);
				*dest++=(unsigned char)(val&0xff);
			} else return 0;
		} else return 0;
	}
	return(dest-binptr);
}

size_t filesize(char *fname)
{
	size_t s;
	FILE *f=fopen(fname,"r");
	if(!f)return 0;
	fseek(f,0L,SEEK_END);
	s=ftell(f);fclose(f);
	return s;
}

int islocked(char *pidfn)
{
	FILE *f;
	long pid;
	if((f=fopen(pidfn,"rt"))) {
		fscanf(f,"%ld",&pid);
		fclose(f);
		if(kill((pid_t)pid,0)&&(errno==ESRCH))lunlink(pidfn);
		else return((int)pid);
	}
	return 0;
}

int lockpid(char *pidfn)
{
	int rc;
	FILE *f;
	char tmpname[MAX_PATH],*p;
	if(islocked(pidfn))return 0;
#ifndef LOCKSTYLE_OPEN
	xstrcpy(tmpname,pidfn,MAX_PATH);
	p=strrchr(tmpname,'/');
	if(!p)p=tmpname;
	snprintf(tmpname+(p-tmpname),MAX_PATH-(p-tmpname+1),"/QTEMP.%ld",(long)getpid());
	if(!(f=fopen(tmpname,"w")))return 0;
	fprintf(f,"%10ld\n",(long)getpid());
	fclose(f);
	rc=link(tmpname,pidfn);
	lunlink(tmpname);
	if(rc)return 0;
#else
	rc=open(pidfn,O_WRONLY|O_CREAT|O_EXCL,0644);
	if(rc<0)return 0;
	snprintf(tmpname,MAX_PATH,"%10ld\n",(long)getpid());
	write(rc,tmpname,strlen(tmpname));
	close(rc);
#endif
	return 1;
}

unsigned long sequencer()
{
	if(seq==0xFFFFFFFF||time(NULL)>seq)seq=time(NULL);
	    else seq++;
	return seq;
}

int touch(char *fn)
{
	FILE *f=fopen(fn,"a");
 	if(f) {
 		fclose(f);
 		return 1;
 	} else write_log("can't touch '%s': %s",fn,strerror(errno));
 	return 0;
}

int mkdirs(char *name)
{
	int rc=0;
	char *p=name+1,*q;
	while((q=strchr(p,'/'))) {
		*q=0;rc=mkdir(name,cfgi(CFG_DIRPERM));
		*q='/';p=q+1;
	}
	return rc;
}

void rmdirs(char *name)
{
	int rc=0;
	char *q=strrchr(name,'/'),*t;
	while(q&&q!=name&&!rc) {
		*q=0;rc=rmdir(name);
		t=strrchr(name,'/');
		*q='/';q=t;
	}
}

FILE *mdfopen(char *fn,char *pr)
{
	FILE *f;
	struct stat sb;
	int nf=(stat(fn,&sb))?1:0;
 	if((f=fopen(fn,pr))) {
		if(nf)fchmod(fileno(f),cfgi(CFG_DEFPERM));
		return f;
	}
	if(errno==ENOENT) {
		mkdirs(fn);
		f=fopen(fn,pr);
		if(f&&nf)fchmod(fileno(f),cfgi(CFG_DEFPERM));
		return f;
	}
	return NULL;
}

int fexist(char *s)
{
	struct stat sb;
	return(!stat(s,&sb)&&S_ISREG(sb.st_mode));
}

int dosallowin83(int c)
{
	static char dos_allow[]="!@#$%^&()~`'-_{}";
	if((c>='a'&&c <= 'z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
		strchr(dos_allow,c))return 1;
	return 0;
}

char *fnc(char *s)
{
	char *p,*q;
	unsigned int i=0;
	static char s8[13];
	if(!s)return NULL;
	if(!(p=strrchr(s,'/')))p=s; else s=p;
	while(*p&&*p!='.'&&i<8) {
		if(dosallowin83(*p))s8[i++]=tolower(*p);
		p++;
	}
	s8[i]=0;
	if(strstr(s,".tar.gz"))xstrcat(s8,".tgz",14);
	else if(strstr(s,".tar.bz2"))xstrcat(s8,".tb2",14);
	else if(strstr(s,".html"))xstrcat (s8,".htm",14);
	else if(strstr(s,".jpeg"))xstrcat (s8,".jpg",14);
	else if(strstr(s,".desc"))xstrcat (s8,".dsc",14);
	    else {
		if((p=strrchr(s,'.'))) {
			xstrcat(s8,".",14);
			q=p+4;i=strlen(s8);
			while(*p&&q>p) {
				if(dosallowin83(*p))s8[i++]=tolower(*p);
				p++;
			}
			s8[i]=0;
		}
	}
	return s8;
}

int lunlink(char *s)
{
	int rc=unlink(s);
	if(rc<0&&errno!=ENOENT)write_log("can't delete file %s: %s",s,strerror(errno));
	return rc;
}

int isdos83name(char *fn)
{
	int nl=0,el=0,ec=0,uc=0,lc=0,f=1;
	while(*fn) {
		if(!dosallowin83(*fn)&&(*fn!='.')) {
			f=0;
			break;
		}
	    	if(*fn=='.')ec++;
	    	    else {
			if(!ec)nl++;
			    else el++;
			if(isalpha((int)*fn)) {
				if(isupper((int)*fn))uc++;
				    else lc++;
			}
		}
	    	fn++;
	}
	return(f&&ec<2&&el<4&&nl<9&&(!lc||!uc));
}

#if defined(HAVE_STATFS) && defined(STATFS_HAVE_F_BAVAIL)
#define STAT_V_FS statfs
#else
#if defined(HAVE_STATVFS) && defined(STATVFS_HAVE_F_BAVAIL)
#define STAT_V_FS statvfs
#else
#undef STAT_V_FS
#endif
#endif
size_t getfreespace(const char *path)
{
#ifdef STAT_V_FS
	struct STAT_V_FS sfs;
	if(!STAT_V_FS(path,&sfs)) {
		if(sfs.f_bsize>=1024)return((sfs.f_bsize/1024L)*sfs.f_bavail);
		    else return(sfs.f_bavail/(1024L/sfs.f_bsize));
	} else write_log("can't statfs '%s': %s",path,strerror(errno));
#endif
	return(~0L);
}

void to_dev_null()
{
	int fd;
	close(STDIN_FILENO);close(STDOUT_FILENO);close(STDERR_FILENO);
	fd=open(devnull,O_RDONLY);
	if(dup2(fd,STDIN_FILENO)!=STDIN_FILENO){write_log("reopening of stdin failed: %s",strerror(errno));exit(-1);}
	if(fd!=STDIN_FILENO)close(fd);
	fd=open(devnull,O_WRONLY|O_APPEND|O_CREAT,0600);
	if(dup2(fd,STDOUT_FILENO)!=STDOUT_FILENO){write_log("reopening of stdout failed: %s",strerror(errno));exit(-1);}
	if(fd!=STDOUT_FILENO)close(fd);
	fd=open(devnull,O_WRONLY|O_APPEND|O_CREAT,0600);
	if(dup2(fd,STDERR_FILENO)!=STDERR_FILENO){write_log("reopening of stderr failed: %s",strerror(errno));exit(-1);}
	if(fd!=STDERR_FILENO)close(fd);
}

int randper(int base,int diff)
{
	return base-diff+(int)(diff*2.0*rand()/(RAND_MAX+1.0));
}

int execsh(char *cmd)
{
	int pid,status,rc;

	if ((pid=fork()) == 0) {
		to_dev_null();
		rc=execl(SHELL,"sh","-c",cmd,NULL);
		exit(-1);
	}
	if(pid<0) {
		write_log("can't fork(): %s",strerror(errno));
		return -1;
	}
	do {
		rc=waitpid(pid, &status, 0);
	} while ((rc == -1) && (errno == EINTR));
	if(rc<0) {
		write_log("error in waitpid(): %s",strerror(errno));
		return -1;
	}
	return WEXITSTATUS(status);
}

int execnowait(char *cmd,char *p1,char *p2,char *p3)
{
	int pid,rc;

	if ((pid=fork()) == 0) {
		to_dev_null();
		setsid();
		rc=execl(cmd,cmd,p1,p2,p3,NULL);
		if(rc<0) write_log("can't exec %s: %s", cmd, strerror(errno));
		exit(-1);
	}
	if(pid<0) {
		write_log("can't fork(): %s",strerror(errno));
		return -1;
	}
	return 0;
}
