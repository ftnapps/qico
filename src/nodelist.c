/**********************************************************
 * File: nodelist.c
 * Created at Thu Jul 15 16:14:36 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: nodelist.c,v 1.3 2000/07/18 12:58:58 lev Exp $
 **********************************************************/
#include "ftn.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include "qconf.h"

#define NL_RECS 1000

char *NL_IDX="/qnode.idx";
char *NL_SIGN="qico nodelist index";

int query_nodelist(ftnaddr_t *addr, char *nlpath, ninfo_t **nl)
{
	ninfo_t *nlent=malloc(sizeof(ninfo_t));
	FILE *idx;
	idxh_t ih;
	idxent_t ie[NL_RECS];
	char nlp[MAX_PATH], str[MAX_STRING], *p, *t;
	int rc,i;
	struct stat sb;

	*nl=NULL;
	bzero(nlent, sizeof(ninfo_t));
	sprintf(nlp, "%s/%s.lock", nlpath, NL_IDX);
	if(islocked(nlp)) return 0;
	sprintf(nlp, "%s/%s", nlpath, NL_IDX);
	idx=fopen(nlp, "rb");
	if(!idx) { sfree(nlent);return 1; }
	if(fread(&ih, sizeof(idxh_t), 1, idx)!=1) {
		sfree(nlent);return 1;
	}
	if(strcmp(ih.sign, NL_SIGN)) {
		sfree(nlent);return 1;
	}
	do {
		rc=fread(&ie, sizeof(idxent_t), NL_RECS, idx);
		for(i=0;i<rc;i++) if(ADDRCMP(ie[i].addr, (*addr))) break;
	} while(rc>=NL_RECS && i==rc);
	fclose(idx);
	if(i==rc) { sfree(nlent);return 0; }
	strcpy(nlp, nlpath);strcat(nlp, "/");
	strcat(nlp, ih.nlname[ie[i].index]);
	if(stat(nlp,&sb)) {
		sfree(nlent);return 2;
	} else
		if(sb.st_mtime!=ih.nltime[ie[i].index]) {
			sfree(nlent);return 3;
		}
	idx=fopen(nlp, "rt");
	if(!idx) { sfree(nlent);return 2; }
	fseek(idx, ie[i].offset, SEEK_SET);
	if(!fgets(str, MAX_STRING, idx)) {
		sfree(nlent);return 2;
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
		strtr(p,'_',' ');nlent->name=strdup(p);
		p=strsep(&t, ",");
		if(p) {
			strtr(p,'_',' ');nlent->place=strdup(p);
			p=strsep(&t, ",");
			if(p) {
				strtr(p,'_',' ');nlent->sysop=strdup(p);
				p=strsep(&t, ",");
				if(p) {
					nlent->phone=strdup(p);
					p=strsep(&t, ",");
					if(p) {
						nlent->speed=atoi(p);
						p=strsep(&t, "\r\n\0");
						if(p) nlent->flags=strdup(p);
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

int is_listed(ftnaddr_t *addr, char *nlpath)
{
	FILE *idx;
	idxh_t ih;
	idxent_t ie;
	int rc;
	char nlp[MAX_PATH];

	strcpy(nlp, nlpath);
	strcat(nlp, NL_IDX);
	idx=fopen(nlp, "rb");
	if(!idx) { return 0; }
	if(fread(&ih, sizeof(idxh_t), 1, idx)!=1) {
		return 0;
	}
	if(strcmp(ih.sign, NL_SIGN)) 
		return 0;
	do rc=fread(&ie, sizeof(idxent_t), 1, idx);
	while(!ADDRCMP(ie.addr, (*addr)) && rc==1);
	fclose(idx);
	return rc;
}

void phonetrans(char *ph, slist_t *phtr)
{
	int rc=1;slist_t *pht;
	char *s, *t,*p, tmp[MAX_STRING];
	if(!*ph) return;
	for(pht=phtr;pht && rc;pht=pht->next) {
		s=strdup(pht->str);
		p=strchr(s,' ');
		if(p) {
			*(p++)='\0';
			while(*p==' ') p++;
			t=strchr(p,' ');
			if(t) *t='\0';
		}
		if(strncmp(ph, s, strlen(s))==0) {
			if(p) strcpy(tmp, p);else *tmp=0;
			strcat(tmp, ph+strlen(s));
			strcpy(ph, tmp);sfree(s);
			return;
		}
		if(s[0]=='=') {
			if(p) strcpy(tmp, p);else *tmp=0;
			strcat(tmp, ph);
			strcpy(ph, tmp);sfree(s);
			return;
		}
		sfree(s);
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

	if(!(rs=strdup(ranges))) return 0;

	for(r=strtok(rs, ",");r;r=strtok(NULL, ",")) {
		while(*r==' ') r++;
		if(!*r) continue;
		if(!strcasecmp(r, "cm")) { sfree(rs); return 1;}
		if(!strcasecmp(r, "never")) { sfree(rs); return 0;}
		if(r[0]=='T') {
			if(chktxy(r)) { sfree(rs); return 1;}
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
						sfree(rs);
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
			{ sfree(rs);return 0; }

		ti=localtime(&tim);
		Day=ti->tm_wday;if(!Day) Day = 7;
		Hour=ti->tm_hour;Min=ti->tm_min;
		firstMark = firstHour * 60; 
		if(firstMinute != -1) firstMark += firstMinute;
		secondMark = secondHour * 60;   
		if(secondMinute != -1) secondMark += secondMinute;
		currentMark = Hour * 60 + Min;  

/* 			log("%d.%d:%d-%d.%d:%d %d.%d:%d", */
/* 				firstDay,firstHour,firstMinute, */
/* 				secondDay,secondHour,secondMinute, */
/* 				Day,Hour,Min); */
		if(secondDay != -1)	{
			if(firstDay < secondDay) {
				if(Day >= firstDay && Day <= secondDay) {
					if(firstMark < secondMark) {
						if(currentMark >= firstMark && currentMark < secondMark)
							{ sfree(rs); return 1; }
					} else {
						if(currentMark >= firstMark || currentMark < secondMark)
							{ sfree(rs); return 1; }
					}
				}
			} else {
				if(firstDay == secondDay) {
					if(Day == firstDay) {
						if(firstMark <= secondMark) {
							if(currentMark >= firstMark && currentMark < secondMark)
								{ sfree(rs); return 1;}
						} else {
							if(currentMark >= firstMark || currentMark < secondMark)
								{ sfree(rs); return 1;}
						}
					}
				} else {
					if(Day >= firstDay || Day <= secondDay) {
						if(firstMark <= secondMark) {
							if(currentMark >= firstMark && currentMark < secondMark)
								{ sfree(rs); return 1;}
						} else {
							if(currentMark >= firstMark || currentMark < secondMark)
								{ sfree(rs); return 1;}
						}
					}
				}
			}
		} else if(secondHour != -1) {
			if(firstMark <= secondMark) {
				if(currentMark >= firstMark && currentMark < secondMark)
					{ sfree(rs); return 1;}
			} else {
				if(currentMark >= firstMark || currentMark < secondMark)
					{ sfree(rs); return 1;}
			}
		} else if(firstDay != -1) {
			if(firstMinute != -1) {
				if(Day == firstDay && Hour == firstHour && Min == firstMinute)
					{ sfree(rs); return 1;}
			} else {
				if(Day == firstDay && Hour == firstHour)
					{ sfree(rs); return 1;}
			}
		} else {
			if(firstMinute != -1) {
				if(Hour == firstHour && Min == firstMinute)
					{ sfree(rs); return 1;}
			} else {
				if(Hour == firstHour)
					{ sfree(rs); return 1;}
			}
		}
    }
			
	sfree(rs);
	return 0;
}

int chktxy(char *p)
{
	time_t tim=time(NULL);
	struct tm *ti=gmtime(&tim);
	int t=ti->tm_hour*60+ti->tm_min, t1, t2;
	t1=(toupper(p[1])-'A')*60+(islower(p[1]) ? 30:0);
	t2=(toupper(p[2])-'A')*60+(islower(p[2]) ? 30:0);
	return ((t1<=t2 && t>=t1 && t<=t2) || (t1>t2 && (t>=t1 || t<=t2)));
}

int checktxy(char *flags)
{
	char *u, *p, *w;
	
	w=strdup(flags);u=w;
	while((p=strsep(&u, ","))) {
		if(!strcmp(p,"CM")) {
			sfree(w);
			return 1;
		}
		if(p[0]=='T' && p[3]==0) {
			sfree(w);
			return chktxy(p);
		}
	}
	sfree(w);
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
	dialine_t *d;

	while(sbs) {
		q=findsubst(&sbs->addr, subs);
		if(!q) {
			q=malloc(sizeof(subst_t));
			q->next=subs;subs=q;			
			ADDRCPY(q->addr, sbs->addr);
			q->hiddens=q->current=NULL;
			q->nhids=0;
		}
		d=malloc(sizeof(dialine_t));
		d->next=q->hiddens;q->current=q->hiddens=d;
		d->num=++q->nhids;
		d->phone=NULL;
		d->timegaps=NULL;
		p=sbs->str;
		while(*p==' ') p++;
		t=strsep(&p, " ");
		if(t) {
			if(*t!='-')
				d->phone=strdup(t);
			if(p) {
				while(*p==' ') p++;
				t=strsep(&p, " ");	
				if(t) if(*t!='-') d->timegaps=strdup(t);
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

	if(!sb) return 0;
	sb->current=sb->current->next;
	if(!sb->current) sb->current=sb->hiddens;
	d=sb->current;

	if(d->phone) {
		if(nl->phone) sfree(nl->phone);
		nl->phone=strdup(d->phone);
	}
	if(d->timegaps) {
		if(nl->wtime) sfree(nl->wtime);
		nl->wtime=strdup(d->timegaps);
		nl->haswtime=1;
	}
	nl->hidnum=(sb->nhids>1)?d->num:0;
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
			sfree(d->phone);sfree(d->timegaps);
			sfree(d);
			d=e;
		}
		sfree(*l);
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

void nlkill(ninfo_t **nl)
{
	if(!*nl) return;
	falist_kill(&(*nl)->addrs);
	sfree((*nl)->name);
	sfree((*nl)->place);
	sfree((*nl)->sysop);
	sfree((*nl)->phone);
	sfree((*nl)->wtime);
	sfree((*nl)->flags);
	sfree((*nl)->pwd);
	sfree((*nl)->mailer);
	sfree(*nl);
}
	
