/**********************************************************
 * Queue operations
 * $Id: queue.c,v 1.10 2004/02/06 21:54:46 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "qipc.h"

qitem_t *q_queue=NULL;

#define _Z(x) ((qitem_t *)x)
int q_cmp(const void *q1,const void *q2)
{
	if(_Z(q1)->addr.z!=_Z(q2)->addr.z)return _Z(q1)->addr.z-_Z(q2)->addr.z;
	if(_Z(q1)->addr.n!=_Z(q2)->addr.n)return _Z(q1)->addr.n-_Z(q2)->addr.n;
	if(_Z(q1)->addr.f!=_Z(q2)->addr.f)return _Z(q1)->addr.f-_Z(q2)->addr.f;
	return _Z(q1)->addr.p-_Z(q2)->addr.p;
}

qitem_t *q_find(ftnaddr_t *fa)
{
	qitem_t *i;int r=1;
	for(i=q_queue;i&&(r=q_cmp(&i->addr,fa))<0;i=i->next);
	return r?NULL:i;
}

qitem_t *q_add(ftnaddr_t *fa)
{
	qitem_t **i,*q;
	int r=1;
	for(i=&q_queue;*i&&(r=q_cmp(&(*i)->addr,fa))<0;i=&((*i)->next));
	if(!r)return *i;
	q=xcalloc(1,sizeof(qitem_t));
	q->next=*i;*i=q;
	ADDRCPY(q->addr,(*fa));
	return q;
}

void q_recountflo(char *name,off_t *size,time_t *mtime,int rslow)
{
	FILE *f;
	struct stat sb;
	char s[MAX_STRING],*p;
	off_t total=0;
	DEBUG(('Q',4,"scan lo '%s'",name));
	if(!stat(name,&sb)) {
		if(sb.st_mtime!=*mtime||rslow) {
			*mtime=sb.st_mtime;
			f=fopen(name,"rt");
			if(f) {
				while(fgets(s,MAX_STRING-1,f)) {
					if(*s=='~')continue;
					p=strrchr(s,'\r');if(p)*p=0;
					p=strrchr(s,'\n');if(p)*p=0;
					p=s;if(*p=='^'||*p=='#')p++;
					if(!stat(p,&sb))total+=sb.st_size;
				}
				fclose(f);
			} else write_log("can't open %s: %s",name,strerror(errno));
			*size=total;
		}
	} else {
		*mtime=0;
		*size=0;
	}
}

void q_each(char *fname,ftnaddr_t *fa,int type,int flavor,int rslow)
{
	qitem_t *q;
	struct stat sb;

	q=q_add(fa);q->touched=1;
	q->what|=type;
	switch(type) {
		case T_REQ:
			if(!stat(fname,&sb)) q->reqs=sb.st_size;
			break;
		case T_NETMAIL:
			if(!stat(fname,&sb)) q->pkts=sb.st_size;
			break;
		case T_ARCMAIL:
			q_recountflo(fname,&q->sizes[flavor-1],&q->times[flavor-1],rslow);
			break;
	}
	q->flv|=(1<<(flavor-1));
}

off_t q_sum(qitem_t *q)
{
	int i;
	off_t total=0;
	for(i=0;i<6;i++)if(q->times[i])total+=q->sizes[i];
	return total;
}

void q_recountbox(char *name,off_t *size,time_t *mtime,int rslow)
{
	DIR *d;
	struct dirent *de;
	struct stat sb;
	char *p;
	off_t total=0;
	int len;
	if(!stat(name,&sb)&&((sb.st_mode&S_IFMT)==S_IFDIR||(sb.st_mode&S_IFMT)==S_IFLNK)) {
		if(sb.st_mtime!=*mtime||rslow) {
			DEBUG(('Q',4,"scan box '%s'",name));
			*mtime=sb.st_mtime;
			d=opendir(name);
			if(d) {
				while((de=readdir(d))) {
					len=strlen(name)+2+strlen(de->d_name);
					p=xmalloc(len);
					snprintf(p,len,"%s/%s",name,de->d_name);
					if(!stat(p,&sb)&&S_ISREG(sb.st_mode)) {
						DEBUG(('Q',6,"add file '%s'",p));
						total+=sb.st_size;
					}
					xfree(p);
				}
				closedir(d);
			} else write_log("can't open %s: %s",name,strerror(errno));
			*size=total;
		}
	} else {
		DEBUG(('Q',4,"box '%s' lost",name));
		*mtime=0;
		*size=0;
	}
}

void rescan_boxes(int rslow)
{
	faslist_t *i;
	qitem_t *q;
	DIR *d;
	struct dirent *de;
	ftnaddr_t a;
	char *p,rev[27],flv;
	int len,n;
	DEBUG(('Q',3,"rescan_boxes"));
	for(i=cfgfasl(CFG_FILEBOX);i;i=i->next) {
		q=q_add(&i->addr);
		q_recountbox(i->str,&q->sizes[4],&q->times[4],rslow);
		if(q->sizes[4]!=0) {
			q->touched=1;
			q->flv|=Q_HOLD;
			q->what|=T_ARCMAIL;
		}
	}
	if(cfgs(CFG_LONGBOXPATH)) {
		if((d=opendir(ccs))!=0) {
			while((de=readdir(d))) {
				n=sscanf(de->d_name,"%hd.%hd.%hd.%hd.%c",&a.z,&a.n,&a.f,&a.p,&flv);
				if(n)DEBUG(('Q',4,"found lbox '%s', parse: %d:%d/%d.%d (%c) rc=%d",de->d_name,a.z,a.n,a.f,a.p,n==5?flv:'*',n));
				if(n==4||n==5) {
					if(n==4)snprintf(rev,27,"%hd.%hd.%hd.%hd",a.z,a.n,a.f,a.p);
					    else snprintf(rev,27,"%hd.%hd.%hd.%hd.%c",a.z,a.n,a.f,a.p,flv);
					if(!strcmp(de->d_name,rev)) {
						len=strlen(ccs)+2+strlen(de->d_name);
						p=xmalloc(len);
						snprintf(p,len,"%s/%s",ccs,de->d_name);
						q=q_add(&a);
						q_recountbox(p,&q->sizes[5],&q->times[5],rslow);
						if(q->sizes[5]!=0) {
							q->touched=1;
							q->what|=T_ARCMAIL;
							if(n==4)flv=*cfgs(CFG_DEFBOXFLV);
							switch(tolower(flv)) {
							    case 'h': q->flv|=Q_HOLD;break;
							    case 'n':
							    case 'f': q->flv|=Q_NORM;break;
							    case 'd': q->flv|=Q_DIR;break;
							    case 'c': q->flv|=Q_CRASH;break;
							    case 'i': q->flv|=Q_IMM;break;
							    default : write_log("unknown longbox flavour '%c' for dir %s",flv,de->d_name);
							}
						}
						xfree(p);
					} else DEBUG(('Q',1,"strange: find: '%s', calc: '%s'",de->d_name,rev));
				} else DEBUG(('Q',2,"bad longbox name %s",de->d_name));
			}
			closedir(d);
		} else write_log("can't open %s: %s",ccs,strerror(errno));
	}
}

int q_rescan(qitem_t **curr,int rslow)
{
	int rc=0;
	sts_t sts;
	qitem_t *q,**p;

	DEBUG(('Q',3,"rescan queue (%d)",rslow));
	for(q=q_queue;q;q=q->next) {
		q->what=0;q->flv&=Q_DIAL;q->touched=0;
	}

	if(is_bso()==1)rc=bso_rescan(q_each,rslow);
	if(is_aso()==1)rc+=aso_rescan(q_each,rslow);
	if(!rc)return 0;
	rescan_boxes(rslow);
	p=&q_queue;
	qqreset();
	while((q=*p)) {
		if(!q->touched) {
			*p=q->next;if(q==*curr)*curr=*p;xfree(q);
		} else {
			if(is_bso()==1) {
				bso_getstatus(&q->addr,&sts);
				q->flv|=sts.flags;q->try=sts.try;
				if(sts.htime>time(NULL))q->onhold=sts.htime;
				    else q->flv&=~Q_ANYWAIT;
				qpqueue(&q->addr,q->pkts,q_sum(q)+q->reqs,q->try,q->flv);
			} else if(is_aso()==1) {
				aso_getstatus(&q->addr,&sts);
				q->flv|=sts.flags;q->try=sts.try;
				if(sts.htime>time(NULL))q->onhold=sts.htime;
				    else q->flv&=~Q_ANYWAIT;
				qpqueue(&q->addr,q->pkts,q_sum(q)+q->reqs,q->try,q->flv);
			}
			p=&((*p)->next);
		}
	}
	if(!*curr)*curr=q_queue;
	DEBUG(('Q',4,"rescan done"));
	return 1;
}

void qsendqueue()
{
	qitem_t *q;
	qqreset();
	for(q=q_queue;q;q=q->next)qpqueue(&q->addr,q->pkts,q_sum(q)+q->reqs,q->try,q->flv);
}
