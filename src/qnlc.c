/**********************************************************
 * File: qnlc.c
 * Created at Tue Jul 27 13:28:49 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qnlc.c,v 1.7 2000/11/26 13:17:34 lev Exp $
 **********************************************************/
#include "headers.h"

int nl_ext(char *s)
{
	char *p, *q;
	if(!(p=strchr(s, ','))) return -1;
	q=strchr(p+1, ',');
	if(q) *q=0;
	if(!p) return -1;else return atoi(p+1);
}

int compile_nodelists()
{
	slist_t *n;
	char fn[MAX_PATH], *p, s[MAX_STRING];
	FILE *idx, *f;
	DIR *d;struct dirent *de;
	int num, max, i=0, gp=0, rc, k, total=0, line=0;
	idxh_t idxh;
	idxent_t ie;
	unsigned long pos;
	struct stat sb;

	strcpy(idxh.sign, NL_SIGN);
	ie.addr.z=cfgal(CFG_ADDRESS)->addr.z;
	ie.addr.n=cfgal(CFG_ADDRESS)->addr.n;
	ie.addr.f=0;ie.addr.p=0;

	setbuf(stdout, NULL);
	sprintf(fn,"%s/%s.lock", cfgs(CFG_NLPATH), NL_IDX);
	lockpid(fn);
	sprintf(fn,"%s/%s", ccs, NL_IDX);
	idx=fopen(fn, "wb");
	if(!idx) {
		write_log("can't open nodelist index %s for writing!",fn);
		sprintf(fn,"%s/%s.lock", ccs, NL_IDX);unlink(fn);
		return 0;		
	}
	fseek(idx, sizeof(idxh), SEEK_SET);
	printf("compiling nodelists...\n");
	for(n=cfgsl(CFG_NODELIST);n;n=n->next) {
		strcpy(s, n->str);*fn=0;
		if((p=strchr(s, ' '))) {
			*p++=0;
			ie.addr.z=atoi(p);
		}
		if(!strcmp(s+strlen(s)-4,".999")) {
			s[strlen(s)-4]=0;
			d=opendir(ccs);
			if(!d) {
				write_log("can't open nodelist directory %s!",ccs);
				fclose(idx);
				sprintf(fn,"%s/%s.lock",ccs, NL_IDX);unlink(fn);
				return 0;
			}
			max=-1;
			while((de=readdir(d))) 
				if(!strncmp(de->d_name, s, strlen(s))) {
					p=de->d_name+strlen(s);				
					if ((*p == '.') && (strlen(p) == 4) &&
						(strspn(p+1,"0123456789") == 3)) {
						num=atoi(p+1);
						if(num>max) max=num;
					}
				}
			closedir(d);
			if(max<0) 
				write_log("no lists matching %s.[0-9]", s);
			else {
				sprintf(idxh.nlname[i], "%s.%03d", s, max);
				sprintf(fn, "%s/%s.%03d", ccs, s, max);
			}
		} else {
			strcpy(idxh.nlname[i], s);
			sprintf(fn, "%s/%s", ccs, s);
		}
		if(!*fn) continue;
		if(!stat(fn,&sb)) idxh.nltime[i]=sb.st_mtime;
		f=fopen(fn, "rt");
		if(!f) 
			write_log("can't open %s for reading!", fn);
		else {
			k=0;pos=0;line=0;
			while(1) {
				pos=ftell(f);
				line++;
				if(!fgets(s, MAX_STRING, f)) break;
				if(s[0]==';' || s[0]=='\r' || s[0]=='\n' ||
				   s[0]==0x1a || !s[0]) continue;
				if(!strncmp(s, "Boss,", 5)) {
					rc=parseftnaddr(s+5, &ie.addr, NULL, 0);
					if(rc) gp=1;
					continue;
				}
				if(gp && !strncmp(s, "Point,", 6)) 
					ie.addr.p=nl_ext(s);
				if(!strncmp(s, "Zone,", 5)) {
					gp=0;
					ie.addr.z=ie.addr.n=nl_ext(s);
					ie.addr.f=0;ie.addr.p=0;
				} else if(!strncmp(s, "Host,", 5) ||
						  !strncmp(s,"Region,", 7)) {
					gp=0;
					ie.addr.n=nl_ext(s);
					ie.addr.f=0;ie.addr.p=0;
				} else if(!gp && strncmp(s, "Down,", 5)) {
					ie.addr.f=nl_ext(s);ie.addr.p=0;
				}					
				if(ie.addr.z < 0 || ie.addr.n < 0 || ie.addr.f < 0 || ie.addr.p < 0) {
					write_log("can not parse address in %s at line %d!",basename(fn),line);
					fclose(idx);
					sprintf(fn,"%s/%s.lock", ccs, NL_IDX);unlink(fn);
					return 0;
				}
				ie.offset=pos;ie.index=i;
				k++;
				if(fwrite(&ie, sizeof(ie), 1, idx)!=1) {
					write_log("can't write to index!");
					fclose(idx);
					sprintf(fn,"%s/%s.lock", ccs, NL_IDX);unlink(fn);
					return 0;
				}
			}
			fclose(f);total+=k;
			printf("compiled %s: %d nodes\n", basename(fn), k);
			i++;if(i>MAX_NODELIST) {
				write_log("too much lists - increase MAX_NODELIST in CONFIG and rebuild!");
				break;
			}
		}
	}
	printf("total %d lists, %d nodes\n", i, total);
	fseek(idx, 0, SEEK_SET);
	if(fwrite(&idxh, sizeof(idxh), 1, idx)!=1) 
		write_log("can't write to index!");
	fclose(idx);
	sprintf(fn,"%s/%s.lock", ccs, NL_IDX);unlink(fn);
	return 1;
}
