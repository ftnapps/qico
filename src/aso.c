/**********************************************************
 * outbound management
 * $Id: aso.c,v 1.12 2004/06/05 06:49:13 sisoft Exp $
 **********************************************************/
#include "headers.h"

static int bso_defzone=2;
static char tmp[MAX_STRING];
static char *aso_base,*bso_base,*sts_base,*p_domain;

int aso_init(char *asopath,char *bsopath,char *stspath,int def_zone)
{
	int rc=0;
	if(bsopath) {
	    char *p=strrchr(bsopath,'/');
	    if(p) {
		p_domain=xstrdup(p+1);*p=0;
		bso_base=xstrdup(bsopath);
		bso_defzone=def_zone;
		rc|=BSO;
	    } else bsopath=NULL;
	}
	if(!bsopath) {
 		bso_base=NULL;
		p_domain=NULL;
	}
	if(asopath) {
		aso_base=xstrdup(asopath);
		rc|=ASO;
	} else aso_base=NULL;
	if(stspath)sts_base=xstrdup(stspath);
	else if(bsopath)sts_base=xstrdup(bsopath);
	else if(asopath)sts_base=xstrdup(asopath);
	return rc;
}

void aso_done()
{
	xfree(aso_base);
	xfree(bso_base);
	xfree(sts_base);
	xfree(p_domain);
}

static char *aso_name(int type,ftnaddr_t *fa)
{
	if(type==ASO) {
		snprintf(tmp,MAX_STRING,"%s/%d.%d.%d.%d.",
			aso_base,fa->z,fa->n,fa->f,fa->p);
	} else {
		char t[30];
		snprintf(tmp,MAX_STRING,"%s/%s",bso_base,p_domain);
		if(fa->z!=bso_defzone) {
			snprintf(t,30,".%03x",fa->z);
			xstrcat(tmp,t,MAX_STRING);
		}
		snprintf(t,30,"/%04x%04x.",fa->n,fa->f);
		xstrcat(tmp,t,MAX_STRING);
		if(fa->p) {
			snprintf(t,30,"pnt/%08x.",fa->p);
			xstrcat(tmp,t,MAX_STRING);
		}
	}
	return tmp;
}

static char *aso_pktn(int type,ftnaddr_t *fa,int fl)
{
	char *e="eut";
	aso_name(type,fa);
	switch(fl) {
	    case F_NORM:
	    case F_REQ: e="out";break;
	    case F_DIR: e="dut";break;
	    case F_CRSH:e="cut";break;
	    case F_HOLD:e="hut";break;
	    case F_IMM: e="iut";break;
	}
	xstrcat(tmp,e,MAX_STRING);
	return tmp;
}

static char *aso_flon(int type,ftnaddr_t *fa, int fl)
{
	char *e="elo";
	aso_name(type,fa);
	switch(fl) {
	    case F_NORM:
	    case F_REQ: e="flo";break;
	    case F_DIR: e="dlo";break;
	    case F_CRSH:e="clo";break;
	    case F_HOLD:e="hlo";break;
	    case F_IMM: e="ilo";break;
	}
	xstrcat(tmp,e,MAX_STRING);
	return tmp;
}

static char *aso_bsyn(int type,ftnaddr_t *fa,char b)
{
	char bn[]="esy";
	*bn=b;aso_name(type,fa);
	xstrcat(tmp,bn,MAX_STRING);
	return tmp;
}

static char *aso_reqn(int type,ftnaddr_t *fa)
{
	aso_name(type,fa);
	xstrcat(tmp,"req",MAX_STRING);
	return tmp;
}

static char *aso_stsn(ftnaddr_t *fa)
{
	snprintf(tmp,MAX_STRING,"%s/%d.%d.%d.%d.qst",
		sts_base,fa->z,fa->n,fa->f,fa->p);
	return tmp;
}

int aso_rescan(void (*each)(char *, ftnaddr_t *, int, int,int),int rslow)
{
	DIR *dz;
	int rc=0;
	FTNADDR_T(a);
	struct dirent *dez;
	char fn[MAX_PATH],*p;
	if(aso_base&&(dz=opendir(aso_base))) {
	    while((dez=readdir(dz))) {
		p=strrchr(dez->d_name,'.');
		if(!p)continue; *p=0;
		if(sscanf(dez->d_name,"%hd.%hd.%hd.%hd",&a.z,&a.n,&a.f,&a.p)!=4)continue;
		snprintf(fn,MAX_PATH,"%s/%s.%s",aso_base,dez->d_name,p+1);
		if(!strcasecmp(p+2,"lo"))each(fn,&a,T_ARCMAIL,aso_flavor(p[1]),rslow);
		if(!strcasecmp(p+2,"ut"))each(fn,&a,T_NETMAIL,aso_flavor(p[1]),rslow);
		if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);
	    }
	    closedir(dz);
	    rc|=ASO;
	}
	if(bso_base&&(dz=opendir(bso_base))) {
	    int flv;
	    DIR *dn,*dp;
	    struct dirent *den,*dep;
	    while((dez=readdir(dz))) {
		if(!strncmp(dez->d_name,p_domain,strlen(p_domain))) {
			p=strrchr(dez->d_name,'.');
			if(p)sscanf(p,".%03hx",(unsigned short*)&a.z);
			    else a.z=bso_defzone;
			snprintf(fn,MAX_PATH,"%s/%s",bso_base,dez->d_name);
			if((dn=opendir(fn))!=0) {
				while((den=readdir(dn))) {
					p=strrchr(den->d_name,'.');
					if(!p)continue;	*p=0;
					if(sscanf(den->d_name,"%04hx%04hx",(unsigned short*)&a.n,(unsigned short*)&a.f)!=2)continue;
					*p='.';a.p=0;
					snprintf(fn,MAX_PATH,"%s/%s/%s",bso_base,dez->d_name,den->d_name);
					if(!strcasecmp(p,".pnt")) {
						if((dp=opendir(fn))!=0) {
							while((dep=readdir(dp))) {
								p=strrchr(dep->d_name,'.');
								if(p) {
									*p=0;
									if(sscanf(dep->d_name+4,"%04hx",(unsigned short*)&a.p)!=1)continue;
									*p='.';
									snprintf(fn,MAX_PATH,"%s/%s/%s/%s",bso_base,dez->d_name,den->d_name,dep->d_name);
									if(!strcasecmp(p+2,"lo")&&F_ERR!=(flv=aso_flavor(p[1])))
										each(fn,&a,T_ARCMAIL,flv,rslow);
									if(!strcasecmp(p+2,"ut")&&F_ERR!=(flv=aso_flavor(p[1])))
										each(fn,&a,T_NETMAIL,flv,rslow);
									if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);
								}
							}
							closedir(dp);
						}
					} else {
						if(!strcasecmp(p+2,"lo")&&F_ERR!=(flv=aso_flavor(p[1])))
							each(fn,&a,T_ARCMAIL,flv,rslow);
						if(!strcasecmp(p+2,"ut")&&F_ERR!=(flv=aso_flavor(p[1])))
							each(fn,&a,T_NETMAIL,flv,rslow);
						if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);
					}
				}
				closedir(dn);
			}
		}
	    }
	    closedir(dz);
	    rc|=BSO;
	}
	return rc;
}

int aso_flavor(char fl)
{
	switch(toupper(fl)) {
	    case 'H': return F_HOLD;
	    case 'F':
	    case 'N':
	    case 'O': return F_NORM;
	    case 'D': return F_DIR;
	    case 'C': return F_CRSH;
	    case 'I': return F_IMM;
	    case 'R': return F_REQ;
	}
	return F_ERR;
}

static int aso_lock(int type,ftnaddr_t *adr,int l)
{
	mkdirs(aso_bsyn(type,adr,'b'));
	if(islocked(tmp))return 0;
	if(l==LCK_s)return lockpid(tmp);
	if(l==LCK_t)return getpid();
	if(islocked(aso_bsyn(type,adr,'c')))return 0;
	return lockpid(tmp);
}

int aso_locknode(ftnaddr_t *adr,int l)
{
	int rc=0;
	if(aso_base)if(aso_lock(ASO,adr,l))rc|=ASO;
	if(bso_base)if(aso_lock(BSO,adr,l))rc|=BSO;
	return rc;
}

int aso_unlocknode(ftnaddr_t *adr,int l)
{
	if(l==LCK_t)return 1;
	if(aso_base) {
		lunlink(aso_bsyn(ASO,adr,'b'));
		lunlink(aso_bsyn(ASO,adr,'c'));
	}
	if(bso_base) {
		lunlink(aso_bsyn(BSO,adr,'b'));
		lunlink(aso_bsyn(BSO,adr,'c'));
		if(adr->z!=bso_defzone)rmdirs(tmp);
		    else if(adr->p)rmdir(tmp);
	}
	return 1;
}

int aso_attach(ftnaddr_t *adr,int flv,slist_t *files)
{
	slist_t *fl;
	FILE *f=NULL;
	if(bso_base)f=mdfopen(aso_flon(BSO,adr,flv),"at");
	if(!f&&aso_base)f=mdfopen(aso_flon(ASO,adr,flv),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next)fprintf(f,"%s\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_request(ftnaddr_t *adr,slist_t *files)
{
	slist_t *fl;
	FILE *f=NULL;
	if(bso_base)f=mdfopen(aso_reqn(BSO,adr),"at");
	if(!f&&aso_base)f=mdfopen(aso_reqn(ASO,adr),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next)fprintf(f,"%s\r\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_poll(ftnaddr_t *fa,int flavor)
{
	if(flavor==F_ERR) {
		char *pf=cfgs(CFG_POLLFLAVOR);
		flavor=aso_flavor(*pf);
	}
	return aso_attach(fa,flavor,NULL);
}

int aso_setstatus(ftnaddr_t *fa,sts_t *st)
{
	FILE *f;
	if((f=mdfopen(aso_stsn(fa),"wt"))) {
		fprintf(f,"%d %d %lu %lu",st->try,st->flags,st->htime,st->utime);
		if(st->bp.name&&st->bp.flags)fprintf(f," %d %d %lu %s",st->bp.flags,st->bp.size,st->bp.time,st->bp.name);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_getstatus(ftnaddr_t *fa,sts_t *st)
{
	int rc;
	FILE *f;
	if((f=fopen(aso_stsn(fa),"rt"))) {
		rc=fscanf(f,"%d %d %lu %lu %d %d %lu %s",
		    &st->try,&st->flags,(unsigned long*)&st->htime,(unsigned long*)&st->utime,
			&st->bp.flags,(int*)&st->bp.size,(unsigned long*)&st->bp.time,tmp);
		fclose(f);
		if(rc<8)memset(&st->bp,0,sizeof(st->bp));
		    else if(*tmp)st->bp.name=xstrdup(tmp);
		if(rc==4||rc==8)return 1;
		write_log("status file %s corrupted",aso_stsn(fa));
		xfree(st->bp.name);
	}
	memset(st,0,sizeof(sts_t));
	return 0;
}

static void floflist(flist_t **fl,char *flon)
{
	FILE *f;
	off_t off;
	char *p,str[MAX_PATH+1],*l,*m,*map=cfgs(CFG_MAPOUT),*fp,*fn;
	slist_t *i;
	struct stat sb;
	int len;
	DEBUG(('S',2,"Add LO '%s'",flon));
	if(!stat(flon, &sb))if((f=fopen(flon,"r+b"))) {
		off=ftell(f);
		while(fgets(str,MAX_PATH,f)) {
			p=strrchr(str,'\r');
			if(!p)p=strrchr(str,'\n');
			if(p)*p=0;
			if(!*str)continue;
			p=str;
			switch(*p) {
			    case '~': break;
			    case '^': /* kill */
			    case '#': /* trunc */
				p++;
			    default:
				for(i=cfgsl(CFG_MAPPATH);i;i=i->next) {
					for(l=i->str;*l&&*l!=' ';l++);
					for(m=l;*m==' ';m++);
					len=l-i->str;
					if(!*l||!*m)write_log("bad mapping '%s'",i->str);
					    else if(!strncasecmp(i->str,p,len)) {
						memmove(p+strlen(m),p+len,strlen(p+len)+1);
						memcpy(p,m,strlen(m));
					}
				}
				if(map&&strchr(map,'S'))strtr(p,'\\','/');
				fp=xstrdup(p);l=basename(fp);
				if(map&&strchr(map,'L'))strlwr(l);
				else if(map&&strchr(map,'U'))strupr(l);
				fn=basename(p);
    				mapname(fn,map,MAX_PATH-(fn-str)-1);
				addflist(fl,fp,xstrdup(fn),str[0],off,f,1);
				if(!stat(fp,&sb)) {
					totalf+=sb.st_size;totaln++;
				}
			}
			off=ftell(f);
		}
		addflist(fl,xstrdup(flon),NULL,'^',-1,f,1);
	}
}

int asoflist(flist_t **fl,ftnaddr_t *fa,int mode)
{
	struct stat sb;
	char str[MAX_PATH];
	int fls[]={F_IMM,F_CRSH,F_DIR,F_NORM,F_HOLD},i;
	for(i=0;i<5;i++) {
		if(bso_base&&!stat(aso_pktn(BSO,fa,fls[i]),&sb)) {
			snprintf(str,MAX_STRING,"%08lx.pkt",sequencer());
			addflist(fl,xstrdup(tmp),xstrdup(str),'^',0,NULL,0);
			totalm+=sb.st_size;totaln++;
		}
		if(aso_base&&!stat(aso_pktn(ASO,fa,fls[i]),&sb)) {
			snprintf(str,MAX_STRING,"%08lx.pkt",sequencer());
			addflist(fl,xstrdup(tmp),xstrdup(str),'^',0,NULL,0);
			totalm+=sb.st_size;totaln++;
		}
	}
	if(bso_base&&!stat(aso_reqn(BSO,fa),&sb)) {
		snprintf(str,MAX_STRING,"%04x%04x.req",fa->n,fa->f);
		addflist(fl,xstrdup(tmp),xstrdup(str),' ',0,NULL,1);
		totalf+=sb.st_size;totaln++;
	}
	if(aso_base&&!stat(aso_reqn(ASO,fa),&sb)) {
		snprintf(str,MAX_STRING,"%04x%04x.req",fa->n,fa->f);
		addflist(fl,xstrdup(tmp),xstrdup(str),' ',0,NULL,1);
		totalf+=sb.st_size;totaln++;
	}
	for(i=0;i<(5-(cfgi(CFG_HOLDOUT)==1&&mode));i++) {
		if(bso_base)floflist(fl,aso_flon(BSO,fa,fls[i]));
		if(aso_base)floflist(fl,aso_flon(ASO,fa,fls[i]));
	}
	return 0;
}
