/**********************************************************
 * bso management
 * $Id: bso.c,v 1.1.1.1 2003/07/12 21:26:28 sisoft Exp $
 **********************************************************/
#include "headers.h"

char *bso_base,*bso_tmp,*bso_base_sts;
static int bso_base_len,bso_tmp_len,bso_base_len_sts;
#ifndef AMIGA4D
char *p_domain;
int bso_defzone=2;

int bso_init(char *bsopath,int def_zone)
{
	char *p;
	p=strrchr(bsopath,'/');
	if(!p)return 0;
	else p_domain=xstrdup(p+1);
	*p=0;
	bso_base=xstrdup(bsopath);
	bso_base_sts=xstrdup(cfgs(CFG_QSTOUTBOUND)?ccs:bsopath);
	bso_base_len=strlen(bso_base)+1;
	bso_base_len_sts=strlen(bso_base_sts)+1;
	bso_tmp_len=((bso_base_len>bso_base_len_sts)?bso_base_len:bso_base_len_sts)+50;
	bso_tmp=xmalloc(bso_tmp_len);
	bso_defzone=def_zone;
	return 1;
}

void bso_done()
{
	xfree(bso_base_sts);
	xfree(bso_base);
	xfree(p_domain);
	xfree(bso_tmp);
}

char *bso_name(ftnaddr_t *fa)
{
	char t[30];
	snprintf(bso_tmp,bso_tmp_len,"%s/%s",bso_base,p_domain);
	if(fa->z!=bso_defzone) {
		snprintf(t,30,".%03x",fa->z);xstrcat(bso_tmp,t,bso_tmp_len);
	}
	snprintf(t,30,"/%04x%04x.",fa->n,fa->f);xstrcat(bso_tmp,t,bso_tmp_len);
	if(fa->p) {
		snprintf(t,30,"pnt/%08x.",fa->p);xstrcat(bso_tmp,t,bso_tmp_len);
	}
	return bso_tmp;
}

int bso_rescan(void (*each)(char *,ftnaddr_t *,int,int,int),int rslow)
{
	struct dirent *dez,*den,*dep;
	char fn[MAX_PATH],*p;
	int flv;
	ftnaddr_t a;
	DIR *dz,*dn,*dp;

	if(!(dz=opendir(bso_base)))return 0;
	while((dez=readdir(dz))) {
		if(!strncmp(dez->d_name,p_domain,strlen(p_domain))) {
			p=strrchr(dez->d_name,'.');
			if(p)sscanf(p,".%03hx",&a.z);
			else a.z=bso_defzone;
			snprintf(fn,MAX_PATH,"%s/%s",bso_base,dez->d_name);
			if((dn=opendir(fn))!=0) {
				while((den=readdir(dn))) {
					p=strrchr(den->d_name,'.');
					if(!p)continue;
					*p=0;sscanf(den->d_name,"%04hx%04hx",&a.n,&a.f);*p='.';
					a.p=0;
					snprintf(fn,MAX_PATH,"%s/%s/%s",bso_base,dez->d_name,den->d_name);
					if(!strcasecmp(p,".pnt")) {
						if((dp=opendir(fn))!=0) {
							while((dep=readdir(dp))) {
								p=strrchr(dep->d_name,'.');
								if(p) {
									*p=0;sscanf(dep->d_name+4,"%04hx",&a.p);*p='.';
									snprintf(fn,MAX_PATH,"%s/%s/%s/%s",bso_base,dez->d_name,den->d_name,dep->d_name);
									if(!strcasecmp(p+2,"lo")&&F_ERR!=(flv=bso_flavor(p[1])))
										each(fn,&a,T_ARCMAIL,flv,rslow);
									if(!strcasecmp(p+2,"ut")&&F_ERR!=(flv=bso_flavor(p[1])))
										each(fn,&a,T_NETMAIL,flv,rslow);
									if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);
								}
							}
							closedir(dp);
						}						
					} else {
						if(!strcasecmp(p+2,"lo")&&F_ERR!=(flv=bso_flavor(p[1])))
							each(fn,&a,T_ARCMAIL,flv,rslow);
						if(!strcasecmp(p+2,"ut")&&F_ERR!=(flv=bso_flavor(p[1])))
							each(fn,&a,T_NETMAIL,flv,rslow);
						if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);	
					}
				}
				closedir(dn);
			}
		}
	}
	closedir(dz);
	return 1;
}

int bso_unlocknode(ftnaddr_t *adr)
{
	char *q;
	lunlink(bso_bsyn(adr));
	if(adr->z!=bso_defzone)rmdirs(bso_tmp);
	    else {
		q=strrchr(bso_tmp,'/');
		if(q&&q!=bso_tmp){*q=0;rmdir(bso_tmp);}
	}
	return 1;
}

/*int bso_rmstatus(ftnaddr_t *adr)
{
	lunlink(bso_stsn(adr));
	rmdirs(bso_tmp);
	return 1;
}*/

#else

int bso_init(char *bsopath,int def_zone)
{
	bso_base=xstrdup(bsopath);
	bso_base_sts=xstrdup(cfgs(CFG_QSTOUTBOUND)?ccs:bsopath);
	bso_base_len=strlen(bso_base)+1;
	bso_base_len_sts=strlen(bso_base_sts)+1;
	bso_tmp_len=((bso_base_len>bso_base_len_sts)?bso_base_len:bso_base_len_sts)+50;
	bso_tmp=xmalloc(bso_tmp_len);
	return 1;
}

void bso_done()
{
	xfree(bso_base_sts);
	xfree(bso_base);
	xfree(bso_tmp);
}

char *bso_name(ftnaddr_t *fa)
{
	snprintf(bso_tmp,bso_tmp_len,"%s/%d.%d.%d.%d.",bso_base,fa->z,fa->n,fa->f,fa->p);
	return bso_tmp;
}

int bso_rescan(void (*each)(char *,ftnaddr_t *,int,int,int),int rslow)
{
	struct dirent *dez;
	char *p;
	ftnaddr_t a;
	DIR *dz;
	char fn[MAX_PATH];

	dz=opendir(bso_base);if(!dz) return 0;
	while((dez=readdir(dz))) {
		p=strrchr(dez->d_name,'.');
		if(!p) continue;
		*p=0;sscanf(dez->d_name,"%hd.%hd.%hd.%hd",&a.z,&a.n,&a.f,&a.p);
		snprintf(fn,MAX_PATH,"%s/%s.%s",bso_base,dez->d_name,p+1);
		if(!strcasecmp(p+2,"lo"))each(fn,&a,T_ARCMAIL,bso_flavor(p[1]),rslow);
		if(!strcasecmp(p+2,"ut"))each(fn,&a,T_NETMAIL,bso_flavor(p[1]),rslow);	
		if(!strcasecmp(p+1,"req"))each(fn,&a,T_REQ,F_REQ,rslow);	
	}	
	closedir(dz);
	return 1;
}

int bso_unlocknode(ftnaddr_t *adr)
{
	lunlink(bso_bsyn(adr));
	return 1;
}

/*int bso_rmstatus(ftnaddr_t *adr)
{
	lunlink(bso_stsn(adr));
	return 1;
}*/

#endif

int bso_flavor(char fl)
{
	fl=toupper(fl);
	switch(fl) {
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

char *bso_pktn(ftnaddr_t *fa,int fl)
{
	bso_name(fa);
	switch(fl) {
		case F_NORM:
		case F_REQ:
			xstrcat(bso_tmp,"out",bso_tmp_len);
			break;
		case F_DIR:
			xstrcat(bso_tmp,"dut",bso_tmp_len);
			break;
		case F_CRSH:
			xstrcat(bso_tmp,"cut",bso_tmp_len);
			break;
		case F_HOLD:
			xstrcat(bso_tmp,"hut",bso_tmp_len);
			break;
		case F_IMM:
			xstrcat(bso_tmp,"iut",bso_tmp_len);
			break;
	}
	return bso_tmp;
}

char *bso_flon(ftnaddr_t *fa,int fl)
{
	bso_name(fa);
	switch(fl) {
		case F_NORM:
		case F_REQ:
			xstrcat(bso_tmp,"flo",bso_tmp_len);
			break;
		case F_DIR:
			xstrcat(bso_tmp,"dlo",bso_tmp_len);
			break;
		case F_CRSH:
			xstrcat(bso_tmp,"clo",bso_tmp_len);
			break;
		case F_HOLD:
			xstrcat(bso_tmp,"hlo",bso_tmp_len);
			break;
		case F_IMM:
			xstrcat(bso_tmp,"ilo",bso_tmp_len);
			break;
	}
	return bso_tmp;
}

char *bso_bsyn(ftnaddr_t *fa)
{
	bso_name(fa);xstrcat(bso_tmp,"bsy",bso_tmp_len);
	return bso_tmp;
}

char *bso_reqn(ftnaddr_t *fa)
{
	bso_name(fa);xstrcat(bso_tmp,"req",bso_tmp_len);
	return bso_tmp;
}

char *bso_stsn(ftnaddr_t *fa)
{
	char *bsob=bso_base;
	bso_base=bso_base_sts;
	bso_name(fa);bso_base=bsob;
	xstrcat(bso_tmp,"qst",bso_tmp_len);
	return bso_tmp;
}

int bso_locknode(ftnaddr_t *adr)
{
	mkdirs(bso_bsyn(adr));
	return lockpid(bso_tmp);
}

int bso_attach(ftnaddr_t *adr,int flv,slist_t *files)
{
	slist_t *fl;
	FILE *f;
	bso_flon(adr,flv);
	if((f=mdfopen(bso_flon(adr,flv),"at"))!=NULL) {
		for(fl=files;fl;fl=fl->next)fprintf(f,"%s\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_request(ftnaddr_t *adr,slist_t *files)
{
	slist_t *fl;
	FILE *f=mdfopen(bso_reqn(adr),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next)fprintf(f,"%s\r\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_setstatus(ftnaddr_t *fa,sts_t *st)
{
	FILE *f;
	if((f=mdfopen(bso_stsn(fa),"wt"))!=NULL) {
		fprintf(f,"%d %d %lu %lu",st->try,st->flags,st->htime,st->utime);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_getstatus(ftnaddr_t *fa,sts_t *st)
{
	FILE *f;
	if((f=fopen(bso_stsn(fa),"rt"))!=NULL) {
		fscanf(f,"%d %d %lu %lu",&st->try,&st->flags,&st->htime,&st->utime);
		fclose(f);
		return 1;
	}
	memset(st,0,sizeof(sts_t));
	return 0;
}

int bso_poll(ftnaddr_t *fa,int flavor)
{
	char *pf;
	if(F_ERR==flavor) {
		pf=cfgs(CFG_POLLFLAVOR);
		flavor=bso_flavor(*pf);
	}
	return bso_attach(fa,flavor,NULL);
}
