/**********************************************************
 * work with nodelists
 * $Id: nodelist.c,v 1.1.1.1 2003/07/12 21:27:04 sisoft Exp $
 **********************************************************/
#include "headers.h"

char *NL_IDX="/qnode.idx";
char *NL_SIGN="qico nodelist index";

static int compare_ftnaddr(const void *a, const void *b)
{
	const ftnaddr_t *aa = (const ftnaddr_t *)a;
	const ftnaddr_t *bb = (const ftnaddr_t *)b;
	int res;
	if((res = aa->z - bb->z)) return res;
	if((res = aa->n - bb->n)) return res;
	if((res = aa->f - bb->f)) return res;
	if((res = aa->p - bb->p)) return res;
	return 0;
}

int query_nodelist(ftnaddr_t *addr, char *nlpath, ninfo_t **nl)
{
	ninfo_t *nlent=xmalloc(sizeof(ninfo_t));
	FILE *idx;
	idxh_t ih;
	idxent_t ie;
	char nlp[MAX_PATH], str[MAX_STRING], *p, *t;
	int rc;
	struct stat sb;
	int l, r;

	*nl=NULL;
	memset(nlent, 0, sizeof(ninfo_t));
	snprintf(nlp, MAX_PATH, "%s%s.lock", nlpath, NL_IDX);
	if(islocked(nlp)) return 0;
	snprintf(nlp, MAX_PATH, "%s%s", nlpath, NL_IDX);
	idx=fopen(nlp, "rb");
	if(!idx) { xfree(nlent);return 1; }
	if(fread(&ih, sizeof(idxh_t), 1, idx)!=1) {
		fclose(idx);
		xfree(nlent);return 1;
	}
	if(strcmp(ih.sign, NL_SIGN)) {
		fclose(idx);
		xfree(nlent);return 1;
	}
	if(fstat(fileno(idx),&sb)) {
		fclose(idx);
		xfree(nlent);return 1;
	}
	l = 0;
	r = (sb.st_size-sizeof(idxh_t))/sizeof(idxent_t);
	rc = -1;
	while(l < r) {
		int m;
		m = (l+r)/2;
		if(fseek(idx, sizeof(idxh_t)+m*sizeof(idxent_t), SEEK_SET)) {
			fclose(idx);
			xfree(nlent);return 1;
		}
		if(fread(&ie, sizeof(ie), 1, idx) != 1) {
			fclose(idx);
			xfree(nlent);return 1;
		}
		rc = compare_ftnaddr(addr, &ie.addr);
		if(rc < 0)
			r = m;
		else if(rc > 0)
			l = m+1;
		else
			break;
	}
	fclose(idx);
	if(rc != 0) {
		xfree(nlent);return 0;
	}
	xstrcpy(nlp, nlpath, MAX_PATH);xstrcat(nlp, "/", MAX_PATH);
	xstrcat(nlp, ih.nlname[ie.index], MAX_PATH);
	if(stat(nlp,&sb)) {
		xfree(nlent);return 2;
	} else
		if(sb.st_mtime!=ih.nltime[ie.index]) {
			xfree(nlent);return 3;
		}
	idx=fopen(nlp, "rt");
	if(!idx) { xfree(nlent);return 2; }
	fseek(idx, ie.offset, SEEK_SET);
	if(!fgets(str, MAX_STRING, idx)) {
		xfree(nlent);return 2;
	}
	fclose(idx);t=str;
	p=strsep(&t, ",");
	if(p) {
		if(!strcasecmp(p,"pvt")) nlent->type=NT_PVT;
		if(!strcasecmp(p,"hold")) nlent->type=NT_HOLD;
		if(!strcasecmp(p,"down")) nlent->type=NT_DOWN;
		if(!strcasecmp(p,"hub")) nlent->type=NT_HUB;
	}
	strsep(&t, ",");
	p=strsep(&t, ",");if(p) {
		strtr(p,'_',' ');nlent->name=xstrdup(p);
		p=strsep(&t, ",");
		if(p) {
			strtr(p,'_',' ');nlent->place=xstrdup(p);
			p=strsep(&t, ",");
			if(p) {
				strtr(p,'_',' ');nlent->sysop=xstrdup(p);
				p=strsep(&t, ",");
				if(p) {
					nlent->phone=xstrdup(p);
					p=strsep(&t, ",");
					if(p) {
						nlent->speed=atoi(p);
						p=strsep(&t, "\r\n\0");
						if(p) nlent->flags=xstrdup(p);
					}
				}
			}
		}
	}
	nlent->haswtime=0;
	*nl=nlent;
	falist_add(&nlent->addrs, addr);
	return 0;
}


int is_listed_one(ftnaddr_t *addr, char *nlpath)
{
	FILE *idx;
	idxh_t ih;
	idxent_t ie;
	int rc;
	char nlp[MAX_PATH];
	struct stat sb;
	int l, r;

	xstrcpy(nlp, nlpath, MAX_PATH);
	xstrcat(nlp, NL_IDX, MAX_PATH);
	idx=fopen(nlp, "rb");
	if(!idx) { return 0; }
	if(fread(&ih, sizeof(idxh_t), 1, idx)!=1) {
		return 0;
	}
	if(strcmp(ih.sign, NL_SIGN)) 
		return 0;
	if(fstat(fileno(idx),&sb)) {
		fclose(idx);
		return 0;
	}
	l = 0;
	r = (sb.st_size-sizeof(idxh_t))/sizeof(idxent_t);
	rc = -1;
	while(l < r) {
		int m;
		m = (l+r)/2;
		if(fseek(idx, sizeof(idxh_t)+m*sizeof(idxent_t), SEEK_SET)) {
			fclose(idx);
			return 0;
		}
		if(fread(&ie, sizeof(ie), 1, idx) != 1) {
			fclose(idx);
			return 0;
		}
		rc = compare_ftnaddr(addr, &ie.addr);
		if(rc < 0)
			r = m;
		else if(rc > 0)
			l = m+1;
		else
			break;
	}
	fclose(idx);
	return rc == 0;
}

int is_listed(falist_t *addrs, char *nlpath, int needall)
{
	falist_t *i;
	for(i=addrs;i;i=i->next) {
		if(needall) {
			if(!is_listed_one(&i->addr,nlpath)) return 0;
		} else {
			if(is_listed_one(&i->addr,nlpath)) return 1;
		}
	}
	return needall;
}

void phonetrans(char **pph, slist_t *phtr)
{
	int rc=1;slist_t *pht;
	char *s, *t,*p, tmp[MAX_STRING];
	char *ph = *pph;
	int len=0;

	if(!*pph || !*ph) return;
	for(pht=phtr;pht && rc;pht=pht->next) {
		s=xstrdup(pht->str);
		p=strchr(s,' ');
		if(p) {
			*(p++)='\0';
			while(*p==' ') p++;
			t=strchr(p,' ');
			if(t) *t='\0';
		}
		if(strncmp(ph, s, strlen(s))==0) {
			if(p) xstrcpy(tmp, p, MAX_STRING);else *tmp=0;
			xstrcat(tmp, ph+strlen(s), MAX_STRING);
			len=strlen(tmp)+1;
			xfree(ph); ph=xmalloc(len); *pph=ph;
			xstrcpy(ph, tmp, len);xfree(s);
			return;
		}
		if(s[0]=='=') {
			if(p) xstrcpy(tmp, p, MAX_STRING);else *tmp=0;
			xstrcat(tmp, ph, MAX_STRING);
			len=strlen(tmp)+1;
			xfree(ph); ph=xmalloc(len); *pph=ph;
			xstrcpy(ph, tmp, len);xfree(s);
			return;
		}
		xfree(s);
	}
}

/* most part of checktimegaps() was taken from ifcico-tx.sc by Alexey Gretchaninov */
int checktimegaps(char *ranges)
{
	int firstDay, secondDay, firstHour, firstMinute, secondHour, secondMinute,
		firstMark, secondMark, currentMark, Day, Hour, Min;
	char *p, *s, *f, *rs, *r;
	time_t tim=time(NULL);
	struct tm *ti;

	if(!ranges) return 0;
	if(!strlen(ranges)) return 0;

	if(!(rs=xstrdup(ranges))) return 0;

	for(r=strtok(rs, ",");r;r=strtok(NULL, ",")) {
		while(*r==' ') r++;
		if(!*r) continue;
		if(!strcasecmp(r, "cm")) { xfree(rs); return 1;}
		if(!strcasecmp(r, "never")) { xfree(rs); return 0;}
		if(r[0]=='T') {
			if(chktxy(r)) { xfree(rs); return 1;}
			else continue;     
		} 

		firstDay = -1;
		secondDay = -1;
		firstHour = -1;
		firstMinute = -1;
		secondHour = -1;
		secondMinute = -1;
			
		if((p = strchr(r, '-'))) {
			if((s = strchr(r, '.')))
				{
					firstDay = atoi(s - 1);
					firstHour = atoi(s + 1);
					if((f = strchr(s + 1, ':')) && f < p)
						firstMinute = atoi(f + 1);
							
					if((s = strchr(p + 1, '.'))) {
						secondDay = atoi(s - 1);
						secondHour = atoi(s + 1);
						if((f = strchr(s + 1, ':')))
							secondMinute = atoi(f + 1);
					} else {
						xfree(rs);
						return 0;
					}
				} else {						
					firstHour = atoi(r);
					if((f = strchr(r, ':')) && f < p)
						firstMinute = atoi(f + 1);
					secondHour = atoi(p + 1);
					if((f = strchr(p + 1, ':')))
						secondMinute = atoi(f + 1);
				}
		} else {
			if((s = strchr(r, '.'))) {
				firstDay = atoi(s - 1);
				firstHour = atoi(s + 1);
				if((p = strchr(s + 1,':')))
					firstMinute = atoi(p + 1);
			} else {
				firstHour = atoi(r);
				if((p = strchr(r,':'))) firstMinute = atoi(p + 1);
			}
		}
			
		if( firstDay < -1 || firstDay > 7 ||
			firstDay == 0 || firstHour < -1 ||
			firstHour > 23 || firstMinute< -1 ||
			firstMinute > 59 || secondDay < -1 ||
			secondDay > 7  || secondDay == 0 || secondHour < -1 ||
			secondHour > 24 || secondMinute < -1 || secondMinute > 59 ||
			firstHour == -1 || (secondDay != -1 && secondHour==-1))
			{ xfree(rs);return 0; }

		ti=localtime(&tim);
		Day=ti->tm_wday;if(!Day) Day = 7;
		Hour=ti->tm_hour;Min=ti->tm_min;
		firstMark = firstHour * 60; 
		if(firstMinute != -1) firstMark += firstMinute;
		secondMark = secondHour * 60;   
		if(secondMinute != -1) secondMark += secondMinute;
		currentMark = Hour * 60 + Min;  

/* 			write_log("%d.%d:%d-%d.%d:%d %d.%d:%d", */
/* 				firstDay,firstHour,firstMinute, */
/* 				secondDay,secondHour,secondMinute, */
/* 				Day,Hour,Min); */
		if(secondDay != -1)	{
			if(firstDay < secondDay) {
				if(Day >= firstDay && Day <= secondDay) {
					if(firstMark < secondMark) {
						if(currentMark >= firstMark && currentMark < secondMark)
							{ xfree(rs); return 1; }
					} else {
						if(currentMark >= firstMark || currentMark < secondMark)
							{ xfree(rs); return 1; }
					}
				}
			} else {
				if(firstDay == secondDay) {
					if(Day == firstDay) {
						if(firstMark <= secondMark) {
							if(currentMark >= firstMark && currentMark < secondMark)
								{ xfree(rs); return 1;}
						} else {
							if(currentMark >= firstMark || currentMark < secondMark)
								{ xfree(rs); return 1;}
						}
					}
				} else {
					if(Day >= firstDay || Day <= secondDay) {
						if(firstMark <= secondMark) {
							if(currentMark >= firstMark && currentMark < secondMark)
								{ xfree(rs); return 1;}
						} else {
							if(currentMark >= firstMark || currentMark < secondMark)
								{ xfree(rs); return 1;}
						}
					}
				}
			}
		} else if(secondHour != -1) {
			if(firstMark <= secondMark) {
				if(currentMark >= firstMark && currentMark < secondMark)
					{ xfree(rs); return 1;}
			} else {
				if(currentMark >= firstMark || currentMark < secondMark)
					{ xfree(rs); return 1;}
			}
		} else if(firstDay != -1) {
			if(firstMinute != -1) {
				if(Day == firstDay && Hour == firstHour && Min == firstMinute)
					{ xfree(rs); return 1;}
			} else {
				if(Day == firstDay && Hour == firstHour)
					{ xfree(rs); return 1;}
			}
		} else {
			if(firstMinute != -1) {
				if(Hour == firstHour && Min == firstMinute)
					{ xfree(rs); return 1;}
			} else {
				if(Hour == firstHour)
					{ xfree(rs); return 1;}
			}
		}
    }
			
	xfree(rs);
	return 0;
}

int chktxy(char *p)
{
	time_t tim=time(NULL);
	struct tm *ti=gmtime(&tim);
	int t=ti->tm_hour*60+ti->tm_min,t1,t2;
	ti=localtime(&tim);
	if(ti->tm_isdst>0){t+=60;if(t>86399)t-=86400;}
	t1=(toupper((int)p[1])-'A')*60+(islower((int)p[1]) ? 30:0);
	t2=(toupper((int)p[2])-'A')*60+(islower((int)p[2]) ? 30:0);
	return ((t1<=t2 && t>=t1 && t<=t2) || (t1>t2 && (t>=t1 || t<=t2)));
}

int checktxy(char *flags)
{
	char *u, *p, *w;
	int i;
	
	w=xstrdup(flags);u=w;
	while((p=strsep(&u, ","))) {
		if(!strcmp(p,"CM")) {
			xfree(w);
			return 1;
		}
		if(p[0]=='T' && p[3]==0) {
			i=chktxy(p);
			xfree(w);
			return i;
		}
	}
	xfree(w);
	return 0;
}

subst_t *findsubst(ftnaddr_t *fa, subst_t *subs)
{
	while(subs && !ADDRCMP((*fa), subs->addr)) subs=subs->next;
	return subs;
}

subst_t *parsesubsts(faslist_t *sbs)
{
	subst_t *subs=NULL,*q;char *p,*t;
	dialine_t *d, *c;

	while(sbs) {
		q=findsubst(&sbs->addr, subs);
		if(!q) {
			q=xmalloc(sizeof(subst_t));
			q->next=subs;subs=q;			
			ADDRCPY(q->addr, sbs->addr);
			q->hiddens=q->current=NULL;
			q->nhids=0;
		}
		d=xmalloc(sizeof(dialine_t));

		/* Insert ind _end_ of list */
		c=q->hiddens;
		if(!c) { q->current=q->hiddens=d;
		} else { while(c->next)c=c->next; c->next=d; }
		
        d->next=NULL;
		d->num=++q->nhids;
		d->phone=NULL;
		d->timegaps=NULL;
		p=sbs->str;
		while(*p==' ') p++;
		t=strsep(&p, " ");
		if(t) {
			if(*t!='-')
				d->phone=xstrdup(t);
			if(p) {
				while(*p==' ') p++;
				t=strsep(&p, " ");	
				if(t) if(*t!='-') d->timegaps=xstrdup(t);
			}
		}
		sbs=sbs->next;
	}
	return subs;
}
	
int applysubst(ninfo_t *nl, subst_t *subs)
{
	subst_t *sb=findsubst(&nl->addrs->addr, subs);
	dialine_t *d;
	ninfo_t *from_nl = NULL;

	if(!sb) return 0;
	d=sb->current;
	sb->current=sb->current->next;
	if(!sb->current) sb->current=sb->hiddens;

	if(!d->phone) query_nodelist(&nl->addrs->addr,cfgs(CFG_NLPATH),&from_nl);

	if(d->phone) {
		if(nl->phone) xfree(nl->phone);
		nl->phone=xstrdup(d->phone);
	} else if(from_nl && from_nl->phone) {
		if(nl->phone) xfree(nl->phone);
		nl->phone=xstrdup(from_nl->phone);
		if(!cfgi(CFG_TRANSLATESUBST))phonetrans(&nl->phone,cfgsl(CFG_PHONETR));
	}
	if(d->timegaps) {
		if(nl->wtime) xfree(nl->wtime);
		nl->wtime=xstrdup(d->timegaps);
		nl->haswtime=1;
	} else {
		if(nl->wtime) xfree(nl->wtime);
		nl->haswtime=0;
	}
	nl->hidnum=(sb->nhids>1)?d->num:0;
	if(from_nl) nlkill(&from_nl);
	return 1;
}

void killsubsts(subst_t **l)
{
	subst_t *t;
	dialine_t *d, *e;
	while(l && *l) {
		t=(*l)->next;
		d=(*l)->hiddens;
		while(d) {
			e=d->next;
			xfree(d->phone);xfree(d->timegaps);
			xfree(d);
			d=e;
		}
		xfree(*l);
	    *l=t;
	}
}

int can_dial(ninfo_t *nl, int ct)
{
	char *p;int d=0;
	if(!nl) return 0;
	if(!*nl->phone) return 0;
	for(p=nl->phone;*p;p++) if(!strchr("0123456789*#TtPpRr,.\"Ww@!-",*p)) d++;
	if(d>0) return 0;
	if(ct) return 1;
	if(nl->haswtime) return checktimegaps(nl->wtime);
	if(nl->type==NT_HOLD||nl->type==NT_DOWN||nl->type==NT_PVT) return 0;
	if(checktxy(nl->flags)) return 1;
	if(nl->addrs->addr.p==0 && checktimegaps(cfgs(CFG_ZMH)))
		return 1;
	return 0;
}

int find_dialable_subst(ninfo_t *nl, int ct, subst_t *subs)
{
	int start;
	applysubst(nl,subs);
	start = nl->hidnum;
	while (!can_dial(nl,ct) && nl->hidnum && applysubst(nl,subs) && nl->hidnum != start);
	return can_dial(nl,ct);
}

void nlfree(ninfo_t *nl)
{
	if(!nl) return;
	falist_kill(&(nl)->addrs);
	xfree(nl->name);
	xfree(nl->place);
	xfree(nl->sysop);
	xfree(nl->phone);
	xfree(nl->wtime);
	xfree(nl->flags);
	xfree(nl->pwd);
	xfree(nl->mailer);
}

void nlkill(ninfo_t **nl)
{
	if(!*nl) return;
	nlfree(*nl);
	xfree(*nl);
}
	
