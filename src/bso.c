/**********************************************************
 * File: bso.c
 * Created at Thu Jul 15 16:10:30 1999 by pk // aaz@ruxy.org.ru
 * bso management
 * $Id: bso.c,v 1.9 2001/01/07 11:25:26 lev Exp $
 **********************************************************/
#include "headers.h"

#define STS_EXT "qst"

char *bso_base, *bso_tmp;
#ifndef AMIGA4D
char *p_domain;
int bso_defzone=2;

int bso_init(char *bsopath, int def_zone)
{
	char *p;
	p=strrchr(bsopath, '/');
	if(!p) return 0;
	else p_domain=strdup(p+1);
	*p=0;
	bso_base=strdup(bsopath);
	bso_tmp=malloc(strlen(bso_base)+50);
	bso_defzone=def_zone;
	return 1;
}

void bso_done()
{
	sfree(bso_base);
	sfree(p_domain);
	sfree(bso_tmp);
}

char *bso_name(ftnaddr_t *fa)
{
	char t[30];
	sprintf(bso_tmp, "%s/%s", bso_base, p_domain);
	if(fa->z!=bso_defzone) {
		sprintf(t, ".%03x", fa->z);strcat(bso_tmp, t);
	}
	sprintf(t, "/%04x%04x.", fa->n, fa->f);strcat(bso_tmp, t);
	if(fa->p) {
		sprintf(t, "pnt/%08x.", fa->p);strcat(bso_tmp, t);
	}
	return bso_tmp;
}



int bso_rescan(void (*each)(char *, ftnaddr_t *, int, int ))
{
	struct dirent *dez, *den, *dep;
	char fn[MAX_PATH], *p;
	ftnaddr_t a;
	DIR *dz,*dn,*dp;

	dz=opendir(bso_base);if(!dz) return 0;
	while((dez=readdir(dz))) {
		if(!strncmp(dez->d_name, p_domain, strlen(p_domain))) {
			p=strrchr(dez->d_name, '.');
			if(p)
				sscanf(p, ".%03hx", &a.z);
			else
				a.z=bso_defzone;
			sprintf(fn, "%s/%s", bso_base, dez->d_name);
			dn=opendir(fn);if(dn) {
				while((den=readdir(dn))) {
					p=strrchr(den->d_name, '.');
					if(!p) continue;
					*p=0;sscanf(den->d_name, "%04hx%04hx", &a.n, &a.f);*p='.';
					a.p=0;
					sprintf(fn, "%s/%s/%s", bso_base, dez->d_name,
							den->d_name);
					if(!strcasecmp(p, ".pnt")) {
						dp=opendir(fn);if(dp) {
							while((dep=readdir(dp))) {
								p=strrchr(dep->d_name, '.');
								if(p) {
									*p=0;sscanf(dep->d_name+4, "%04hx", &a.p);*p='.';
									sprintf(fn, "%s/%s/%s/%s", bso_base, dez->d_name,
											den->d_name, dep->d_name);
									if(!strcasecmp(p+2, "lo"))
										each(fn, &a, T_ARCMAIL, bso_flavor(p[1]));	
									if(!strcasecmp(p+2, "ut"))
										each(fn, &a, T_NETMAIL, bso_flavor(p[1]));	
									if(!strcasecmp(p+1, "req"))
										each(fn, &a, T_REQ, F_NORM);	
								}
							}
							closedir(dp);
						}						
					} else {
						if(!strcasecmp(p+2, "lo"))
							each(fn, &a, T_ARCMAIL, bso_flavor(p[1]));	
						if(!strcasecmp(p+2, "ut"))
							each(fn, &a, T_NETMAIL, bso_flavor(p[1]));	
						if(!strcasecmp(p+1, "req"))
							each(fn, &a, T_REQ, F_NORM);	
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
	lunlink(bso_bsyn(adr));
	rmdirs(bso_tmp);
	return 1;
}

int bso_rmstatus(ftnaddr_t *adr)
{
	lunlink(bso_stsn(adr));
	rmdirs(bso_tmp);
	return 1;
}


#else

int bso_init(char *bsopath, int def_zone)
{
	bso_base=strdup(bsopath);
	bso_tmp=malloc(strlen(bso_base)+30);
	return 1;
}

void bso_done()
{
	sfree(bso_base);
	sfree(bso_tmp);
}

char *bso_name(ftnaddr_t *fa)
{
	sprintf(bso_tmp, "%s/%d.%d.%d.%d.", bso_base,
			fa->z, fa->n, fa->f, fa->p);
	return bso_tmp;
}



int bso_rescan(void (*each)(char *, ftnaddr_t *, int, int ))
{
	struct dirent *dez;
	char *p;
	ftnaddr_t a;
	DIR *dz;
	char fn[MAX_PATH];

	dz=opendir(bso_base);if(!dz) return 0;
	while((dez=readdir(dz))) {
		p=strrchr(dez->d_name, '.');
		if(!p) continue;
		*p=0;sscanf(dez->d_name, "%04hd.%04hd.%04hd.%04hd",
					&a.z, &a.n, &a.f, &a.p);
		sprintf(fn, "%s/%s.%s", bso_base, dez->d_name,p+1);
		if(!strcasecmp(p+2, "lo"))
			each(fn, &a, T_ARCMAIL, bso_flavor(p[1]));
		if(!strcasecmp(p+2, "ut"))
			each(fn, &a, T_NETMAIL, bso_flavor(p[1]));	
		if(!strcasecmp(p+1, "req"))
			each(fn, &a, T_REQ, F_NORM);	
	}	
		
	closedir(dz);
	return 1;
}

int bso_unlocknode(ftnaddr_t *adr)
{
	lunlink(bso_bsyn(adr));
	return 1;
}

int bso_rmstatus(ftnaddr_t *adr)
{
	lunlink(bso_stsn(adr));
	return 1;
}


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
	}
	return F_ERR;
}

char *bso_pktn(ftnaddr_t *fa, int fl)
{
	bso_name(fa);
	switch(fl) {
	case F_NORM:
		strcat(bso_tmp, "out");
		break;
	case F_DIR:
		strcat(bso_tmp, "dut");
		break;
	case F_CRSH:
		strcat(bso_tmp, "cut");
		break;
	case F_HOLD:
		strcat(bso_tmp, "hut");
		break;
	case F_IMM:
		strcat(bso_tmp, "iut");
		break;
	}
	return bso_tmp;
}

char *bso_flon(ftnaddr_t *fa, int fl)
{
	bso_name(fa);
	switch(fl) {
	case F_NORM:
		strcat(bso_tmp, "flo");
		break;
	case F_DIR:
		strcat(bso_tmp, "dlo");
		break;
	case F_CRSH:
		strcat(bso_tmp, "clo");
		break;
	case F_HOLD:
		strcat(bso_tmp, "hlo");
		break;
	case F_IMM:
		strcat(bso_tmp, "ilo");
		break;
	}
	return bso_tmp;
}


char *bso_bsyn(ftnaddr_t *fa)
{
	bso_name(fa);strcat(bso_tmp, "bsy");
	return bso_tmp;
}

char *bso_reqn(ftnaddr_t *fa)
{
	bso_name(fa);strcat(bso_tmp, "req");
	return bso_tmp;
}

char *bso_stsn(ftnaddr_t *fa)
{
	bso_name(fa);strcat(bso_tmp, STS_EXT);
	return bso_tmp;
}

						
int bso_locknode(ftnaddr_t *adr)
{
	mkdirs(bso_bsyn(adr));
	return lockpid(bso_tmp);
}

int bso_attach(ftnaddr_t *adr, int flv, slist_t *files)
{
	slist_t *fl;
	FILE *f;
	bso_flon(adr,flv);
		f=mdfopen(bso_flon(adr,flv),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next) 
			fprintf(f,"%s\n",fl->str);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_request(ftnaddr_t *adr, slist_t *files)
{
	slist_t *fl;
	FILE *f=mdfopen(bso_reqn(adr),"at");
	if(f) {
		for(fl=files;fl;fl=fl->next) 
			fprintf(f,"%s\r\n",fl->str);
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
		fprintf(f, "%d %d %lu %lu", st->try, st->flags, st->htime,
				st->utime);
		fclose(f);
		return 1;
	}
	return 0;
}

int bso_getstatus(ftnaddr_t *fa, sts_t *st)
{
	FILE *f;
	f=fopen(bso_stsn(fa), "rt");
	if(f) {
		fscanf(f, "%d %d %lu %lu", &st->try, &st->flags,
			   &st->htime, &st->utime);fclose(f);
		return 1;
	}
	bzero(st, sizeof(sts_t));
	return 0;
}

int bso_poll(ftnaddr_t *fa, int flavor)
{
	char *pf;
	if(F_ERR==flavor) {
		pf=cfgs(CFG_POLLFLAVOR);
		flavor=bso_flavor(*pf);
	}
	return bso_attach(fa,flavor,NULL);
}
