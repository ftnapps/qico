/**********************************************************
 * bso management
 * $Id: bso.c,v 1.8 2004/02/09 01:05:33 sisoft Exp $
 **********************************************************/
#include "headers.h"

static char *bso_base,*bso_base_sts,*p_domain;
static int bso_base_len,bso_tmp_len,bso_base_len_sts;
static int bso_defzone=2;

int bso_init(char *bsopath,int def_zone)
{
	char *p;
	if(bsopath==NULL) {
		bso_base=NULL;
		bso_tmp=NULL;
		return 0;
	}
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
					if(!p)continue;	*p=0;
					if(sscanf(den->d_name,"%04hx%04hx",&a.n,&a.f)!=2)continue;
					*p='.';a.p=0;
					snprintf(fn,MAX_PATH,"%s/%s/%s",bso_base,dez->d_name,den->d_name);
					if(!strcasecmp(p,".pnt")) {
						if((dp=opendir(fn))!=0) {
							while((dep=readdir(dp))) {
								p=strrchr(dep->d_name,'.');
								if(p) {
									*p=0;
									if(sscanf(dep->d_name+4,"%04hx",&a.p)!=1)continue;
									*p='.';
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

int bso_unlocknode(ftnaddr_t *adr,int l)
{
	if(l==LCK_t)return 1;
	lunlink(bso_bsyn(adr,'b'));
	lunlink(bso_bsyn(adr,'c'));
	if(adr->z!=bso_defzone)rmdirs(bso_tmp);
	    else if(adr->p)rmdir(bso_tmp);
	return 1;
}

/*int bso_rmstatus(ftnaddr_t *adr)
{
	lunlink(bso_stsn(adr));
	rmdirs(bso_tmp);
	return 1;
}*/

int bso_flavor(char fl)
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

char *bso_bsyn(ftnaddr_t *fa,char b)
{
	char bn[]="xsy";
	*bn=b;bso_name(fa);
	xstrcat(bso_tmp,bn,bso_tmp_len);
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

int bso_locknode(ftnaddr_t *adr,int l)
{
	mkdirs(bso_bsyn(adr,'b'));
	if(islocked(bso_tmp))return 0;
	if(l==LCK_s)return lockpid(bso_tmp);
	if(l==LCK_t)return getpid();
	if(islocked(bso_bsyn(adr,'c')))return 0;
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

int bso_setstatus(ftnaddr_t *fa, sts_t *st)
{
	FILE *f;
	f=mdfopen(bso_stsn(fa), "wt");
	if(f) {
		fprintf(f,"%d %d %lu %lu",st->try,st->flags,st->htime,st->utime);
		if(st->bp.name&&st->bp.flags)fprintf(f," %d %d %lu %s",st->bp.flags,st->bp.size,st->bp.time,st->bp.name);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_getstatus(ftnaddr_t *fa, sts_t *st)
{
	int rc;
	FILE *f;
	char buf[MAX_PATH];
	f=fopen(bso_stsn(fa),"rt");
	if(f) {
		rc=fscanf(f,"%d %d %lu %lu %d %d %lu %s",
		    &st->try,&st->flags,&st->htime,&st->utime,
			&st->bp.flags,&st->bp.size,&st->bp.time,buf);
		fclose(f);
		if(rc<8)memset(&st->bp,0,sizeof(st->bp));
		    else if(*buf)st->bp.name=xstrdup(buf);
		if(rc==4||rc==8)return 1;
		write_log("status file %s corrupted",bso_tmp);
		xfree(st->bp.name);
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
