/**********************************************************
 * aso management
 * $Id: aso.c,v 1.9 2004/02/09 01:05:33 sisoft Exp $
 **********************************************************/
#include "headers.h"

static char *aso_base,*aso_base_sts;
static int aso_base_len,aso_tmp_len,aso_base_len_sts;

int aso_init(char *asopath, int def_zone)
{
	if(asopath == NULL) {
		aso_base = NULL;
		aso_tmp = NULL;
		return 0;
	}
	aso_base=xstrdup(asopath);
	aso_base_sts=xstrdup(cfgs(CFG_QSTOUTBOUND)?ccs:asopath);
	aso_base_len = strlen(aso_base)+1;
	aso_base_len_sts=strlen(aso_base_sts)+1;
	aso_tmp_len = aso_base_len+50;
	aso_tmp=xmalloc(aso_tmp_len);
	return 1;
}

void aso_done()
{
	xfree(aso_base_sts);
	xfree(aso_base);
	xfree(aso_tmp);
}

char *aso_name(ftnaddr_t *fa)
{
	snprintf(aso_tmp, aso_tmp_len, "%s/%d.%d.%d.%d.", aso_base,
			fa->z, fa->n, fa->f, fa->p);
	return aso_tmp;
}

int aso_rescan(void (*each)(char *, ftnaddr_t *, int, int,int),int rslow)
{
	struct dirent *dez;
	char *p;
	ftnaddr_t a;
	DIR *dz;
	char fn[MAX_PATH];

	dz=opendir(aso_base);if(!dz) return 0;
	while((dez=readdir(dz))) {
		p=strrchr(dez->d_name, '.');
		if(!p) continue; *p=0;
		if(sscanf(dez->d_name, "%hd.%hd.%hd.%hd",&a.z, &a.n, &a.f, &a.p)!=4)continue;
		snprintf(fn, MAX_PATH, "%s/%s.%s", aso_base, dez->d_name,p+1);
		if(!strcasecmp(p+2, "lo"))
			each(fn, &a, T_ARCMAIL, aso_flavor(p[1]),rslow);
		if(!strcasecmp(p+2, "ut"))
			each(fn, &a, T_NETMAIL, aso_flavor(p[1]),rslow);
		if(!strcasecmp(p+1, "req"))
			each(fn, &a, T_REQ, F_REQ,rslow);
	}

	closedir(dz);
	return 1;
}

int aso_unlocknode(ftnaddr_t *adr,int l)
{
	if(l==LCK_t)return 1;
	lunlink(aso_bsyn(adr,'b'));
	lunlink(aso_bsyn(adr,'c'));
	return 1;
}

/*
int aso_rmstatus(ftnaddr_t *adr)
{
	lunlink(aso_stsn(adr));
	return 1;
}
*/

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

char *aso_pktn(ftnaddr_t *fa, int fl)
{
	aso_name(fa);
	switch(fl) {
	    case F_NORM:
	    case F_REQ:
		xstrcat(aso_tmp, "out", aso_tmp_len);
		break;
	    case F_DIR:
		xstrcat(aso_tmp, "dut", aso_tmp_len);
		break;
	    case F_CRSH:
		xstrcat(aso_tmp, "cut", aso_tmp_len);
		break;
	    case F_HOLD:
		xstrcat(aso_tmp, "hut", aso_tmp_len);
		break;
	    case F_IMM:
		xstrcat(aso_tmp, "iut", aso_tmp_len);
		break;
	}
	return aso_tmp;
}

char *aso_flon(ftnaddr_t *fa, int fl)
{
	aso_name(fa);
	switch(fl) {
	    case F_NORM:
	    case F_REQ:
		xstrcat(aso_tmp, "flo", aso_tmp_len);
		break;
	    case F_DIR:
		xstrcat(aso_tmp, "dlo", aso_tmp_len);
		break;
	    case F_CRSH:
		xstrcat(aso_tmp, "clo", aso_tmp_len);
		break;
	    case F_HOLD:
		xstrcat(aso_tmp, "hlo", aso_tmp_len);
		break;
	    case F_IMM:
		xstrcat(aso_tmp, "ilo", aso_tmp_len);
		break;
	}
	return aso_tmp;
}


char *aso_bsyn(ftnaddr_t *fa,char b)
{
	char bn[]="xsy";
	*bn=b;aso_name(fa);
	xstrcat(aso_tmp,bn,aso_tmp_len);
	return aso_tmp;
}

char *aso_reqn(ftnaddr_t *fa)
{
	aso_name(fa);xstrcat(aso_tmp, "req", aso_tmp_len);
	return aso_tmp;
}

char *aso_stsn(ftnaddr_t *fa)
{
	char *asob=aso_base;
	aso_base=aso_base_sts;
	aso_name(fa);aso_base=asob;
	xstrcat(aso_tmp,"qst",aso_tmp_len);
	return aso_tmp;
}

int aso_locknode(ftnaddr_t *adr,int l)
{
	mkdirs(aso_bsyn(adr,'b'));
	if(islocked(aso_tmp))return 0;
	if(l==LCK_s)return lockpid(aso_tmp);
	if(l==LCK_t)return getpid();
	if(islocked(aso_bsyn(adr,'c')))return 0;
	return lockpid(aso_tmp);
}

int aso_attach(ftnaddr_t *adr, int flv, slist_t *files)
{
	slist_t *fl;
	FILE *f;
	aso_flon(adr,flv);
	f=mdfopen(aso_flon(adr,flv),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next)
			fprintf(f,"%s\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_request(ftnaddr_t *adr, slist_t *files)
{
	slist_t *fl;
	FILE *f=mdfopen(aso_reqn(adr),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next)
			fprintf(f,"%s\r\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_setstatus(ftnaddr_t *fa, sts_t *st)
{
	FILE *f;
	f=mdfopen(aso_stsn(fa), "wt");
	if(f) {
		fprintf(f,"%d %d %lu %lu",st->try,st->flags,st->htime,st->utime);
		if(st->bp.name&&st->bp.flags)fprintf(f," %d %d %lu %s",st->bp.flags,st->bp.size,st->bp.time,st->bp.name);
		fclose(f);
		return 1;
	}
	return 0;
}

int aso_getstatus(ftnaddr_t *fa, sts_t *st)
{
	int rc;
	FILE *f;
	char buf[MAX_PATH];
	f=fopen(aso_stsn(fa),"rt");
	if(f) {
		rc=fscanf(f,"%d %d %lu %lu %d %d %lu %s",
		    &st->try,&st->flags,&st->htime,&st->utime,
			&st->bp.flags,&st->bp.size,&st->bp.time,buf);
		fclose(f);
		if(rc<8)memset(&st->bp,0,sizeof(st->bp));
		    else if(*buf)st->bp.name=xstrdup(buf);
		if(rc==4||rc==8)return 1;
		write_log("status file %s corrupted",aso_tmp);
		xfree(st->bp.name);
	}
	memset(st,0,sizeof(sts_t));
	return 0;
}

int aso_poll(ftnaddr_t *fa, int flavor)
{
	char *pf;
	if(F_ERR==flavor) {
		pf=cfgs(CFG_POLLFLAVOR);
		flavor=aso_flavor(*pf);
	}
	return aso_attach(fa,flavor,NULL);
}
