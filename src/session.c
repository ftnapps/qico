/**********************************************************
 * session
 * $Id: session.c,v 1.1.1.1 2003/07/12 21:27:16 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "defs.h"
#include "ls_zmodem.h"
#include "hydra.h"
#include "janus.h"

int runtoss;
ftnaddr_t ndefaddr;

void addflist(flist_t **fl, char *loc, char *rem, char kill, off_t off, FILE *lo, int fromlo)
{
	flist_t **t, *q;
	int type;

	DEBUG(('S',2,"Add file: '%s', sendas: '%s', kill: '%c', fromLO: %s, offset: %d",
			loc,rem?rem:"(null)",kill,lo?"yes":"no",off));

	type=whattype(rem);
	/* If *.PKT from ?LO, sort is as file */
	if(type==IS_PKT && fromlo) type=IS_FILE;

	if((checktimegaps(cfgs(CFG_MAILONLY)) ||
		checktimegaps(cfgs(CFG_ZMH))) && type!=IS_PKT) return;
	switch(type) {
	case IS_REQ:
		if(rnode&&rnode->options&O_HRQ)return;
		if(rnode&&rnode->options&O_NRQ&&!cfgi(CFG_IGNORENRQ))return;
		break;
	case IS_PKT:
		if(rnode && rnode->options&O_HAT) return;
		break;
	default:
		if(rnode && rnode->options&(O_HXT|O_HAT) && rem) return;
		break;
	}
	for(t=fl;*t && ((*t)->type<=type);t=&((*t)->next));
	q=(flist_t *)xmalloc(sizeof(flist_t));
	q->next=*t;*t=q;
	if(fromlo && '#'==kill && cfgi(CFG_ALWAYSKILLFILES)) q->kill = '^';
	else q->kill=kill;
	q->loff=off;
	if(rnode && (rnode->options&O_FNC) && rem) {
	    q->sendas=xstrdup(fnc(rem));
	    xfree(rem);
	} else q->sendas=rem;
	q->lo=lo;q->tosend=loc;
	q->type=type;
}


void floflist(flist_t **fl, char *flon)
{
	FILE *f;
	off_t off;
	char *p,str[MAX_PATH+1],*l,*m,*map=cfgs(CFG_MAPOUT),*fp,*fn;
	slist_t *i;
	struct stat sb;
	int len;

	DEBUG(('S',2,"Add LO '%s'",flon));

	if(!stat(flon, &sb)) if((f=fopen(flon, "r+b"))) {
		off=ftell(f);
		while(fgets(str, MAX_PATH, f)) {
			p=strrchr(str, '\r');if(!p) p=strrchr(str, '\n');
			if(p) *p=0;
			if(!str[0] && str[0]=='~') continue;
			p=str;
			switch(*p) {
			case '~': break;
			case '^': /* kill */
			case '#': /* trunc */
				p++;
			default:
				for(i=cfgsl(CFG_MAPPATH);i;i=i->next) {
					for(l=i->str;*l && *l!=' ';l++);
					for(m=l;*m==' ';m++);
					len=l-i->str;
					if(!*l||!*m) write_log("bad mapping '%s'!", i->str);
					else if(!strncasecmp(i->str,p,len)) {
						memmove(p+strlen(m),p+len,strlen(p+len)+1);
						memcpy(p,m,strlen(m));
					}
				}
				if(map && strchr(map, 'S')) strtr(p,'\\','/');
				fp=xstrdup(p);l=strrchr(fp, '/');if(l) l++;else l=fp;
				if(map && strchr(map, 'U')) strupr(l);
				if(map && strchr(map, 'L')) strlwr(l);
				
				fn=strrchr(p, '/');if(fn) fn++;else fn=p;
    				mapname((char*)fn, map, fn-(char*)str);
				addflist(fl, fp, xstrdup(fn), str[0], off, f, 1);
				
				if(!stat(fp,&sb)) {
					totalf+=sb.st_size;totaln++;
				}
			}		
			off=ftell(f);
		}
		addflist(fl, xstrdup(flon), NULL, '^', -1, f, 1);
	}
}

int boxflist(flist_t **fl, char *path)
{
	DIR *d;char *p;struct dirent *de;struct stat sb;
	char mn[MAX_PATH];
	int len;
	
	DEBUG(('S',2,"Add filebox '%s'",path));

	d=opendir(path);
	if(!d) return 0;
	else {
		while((de=readdir(d))) {
			len=strlen(path)+2+strlen(de->d_name);
			p=xmalloc(len);
			snprintf(p,len,"%s/%s", path, de->d_name);
			if(!stat(p,&sb)&&(S_ISREG(sb.st_mode)||S_ISLNK(sb.st_mode))) {
				xstrcpy(mn,de->d_name,MAX_PATH);
				mapname((char*)mn,cfgs(CFG_MAPOUT),MAX_PATH);
				addflist(fl, p, xstrdup(mn), '^', 0, NULL, 0);
				totalf+=sb.st_size;totaln++;
			} else xfree(p);
		}
		closedir(d);
	}
	return 1;
}

void makeflist(flist_t **fl, ftnaddr_t *fa)
{
	int fls[]={F_IMM, F_CRSH, F_DIR, F_NORM, F_HOLD}, i;
	char str[MAX_PATH];
	struct stat sb;
	faslist_t *j;

	DEBUG(('S',1,"Make filelist for %s", ftnaddrtoa(fa)));
	for(i=0;i<5;i++)
		if(!stat(bso_pktn(fa, fls[i]), &sb)) {
			snprintf(str, MAX_STRING, "%08lx.pkt", sequencer());
			addflist(fl, xstrdup(bso_tmp), xstrdup(str), '^', 0, NULL, 0);
			totalm+=sb.st_size;totaln++;
		}
	
	if(!stat(bso_reqn(fa), &sb)) {
		snprintf(str, MAX_STRING, "%04x%04x.req", fa->n, fa->f);
		addflist(fl, xstrdup(bso_tmp), xstrdup(str), ' ', 0, NULL, 1);
		totalf+=sb.st_size;totaln++;
	}
	
	for(i=0;i<5;i++) floflist(fl, bso_flon(fa, fls[i]));

	for(j=cfgfasl(CFG_FILEBOX);j;j=j->next) 
		if(ADDRCMP((*fa),j->addr)) {
			if(!boxflist(fl, j->str))
				write_log("can't open filebox '%s'!", j->str);
			break;
		}

	if(cfgs(CFG_LONGBOXPATH)) {
		snprintf(str, MAX_STRING, "%s/%d.%d.%d.%d", ccs, fa->z, fa->n, fa->f, fa->p); 
		boxflist(fl, str);
	}
}

void flexecute(flist_t *fl)
{
	char cmt='~', str[MAX_STRING];
	FILE *f;int rem;

	DEBUG(('S',2,"Execute file: '%s', sendas: '%s', kill: '%c' fromLO: %s, offset: %d",
			fl->tosend,fl->sendas?fl->sendas:"(null)",fl->kill,fl->lo?"yes":"no",fl->loff));
	if(fl->lo) {
		if(fl->loff<0) {
			fseek(fl->lo, 0L, SEEK_SET);
			rem=0;
			while(fgets(str, MAX_STRING, fl->lo)) 
				if(*str!='~' && *str!='\n' && *str!='\r') rem++;
			fclose(fl->lo);fl->lo=NULL;
			if(!rem) lunlink(fl->tosend);
		} else if(fl->sendas) {
			switch(fl->kill) {
			case '^':
				lunlink(fl->tosend);break;
			case '#':
				f=fopen(fl->tosend, "w");
				if(f) fclose(f);
				else write_log("can't truncate %s!", fl->tosend);
				break;
			}
			fseek(fl->lo, fl->loff, SEEK_SET);
			fwrite(&cmt, 1, 1, fl->lo);
			xfree(fl->sendas);
		}
 	} else if(fl->sendas) {
		switch(fl->kill) {
		case '^':
			lunlink(fl->tosend);break;
		case '#':
			f=fopen(fl->tosend, "w");
			if(f) fclose(f);
			else write_log("can't truncate %s!", fl->tosend);
			break;
		}
		xfree(fl->sendas);
	}
}

void flkill(flist_t **l, int rc)
{
	flist_t *t;
	DEBUG(('S',1,"Kill filelist"));
	while(*l) {
		DEBUG(('S',2,"Kill file: '%s', sendas: '%s', kill: '%c', type: %d, fromLO: %s, offset: %d",
			(*l)->tosend,(*l)->sendas?(*l)->sendas:"(null)",(*l)->kill,(*l)->type,(*l)->lo?"yes":"no",(*l)->loff));
		if((*l)->lo && (*l)->loff<0) {
			fseek((*l)->lo, 0L, SEEK_END);
			fclose((*l)->lo);
		}
		if((*l)->type==IS_REQ && rc && !(*l)->sendas) lunlink((*l)->tosend);
		xfree((*l)->sendas);
		xfree((*l)->tosend);
		t=(*l)->next;
		xfree(*l);*l=t;
	}	
}

void simulate_send(ftnaddr_t *fa)
{
	flist_t *fl=NULL, *l=NULL;
	makeflist(&fl, fa);
	for(l=fl;l;l=l->next) flexecute(l);
	flkill(&fl, 1);
}

int receivecb(char *fn)
{
	char *p=strrchr(fn,'.');
	if(!p)return 0;
	if(!strcasecmp(p,".req")&&(cfgs(CFG_EXTRP)||cfgs(CFG_SRIFRP))) {
		FILE *f;
		char s[MAX_PATH];
		slist_t *reqs=NULL;
		f=fopen(fn,"rt");
		if(!f){write_log("can't open '%s' for reading",fn);return 0;}
		while(fgets(s,MAX_PATH-1,f)) {
			p=s+strlen(s)-1;
			while(*p=='\r'||*p=='\n')*p--=0;
			slist_add(&reqs,s);
		}
		fclose(f);
		freq_ifextrp(reqs);
		slist_kill(&reqs);
		got_req=1;
		return 1;
	}
	if(!strcasecmp(p,".pkt")&&cfgi(CFG_SHOWPKT)) {
		FILE *f;
		int i,n=1;
		pkthdr_t ph;
		pktmhdr_t mh;
		char from[36],to[36],a;
		f=fopen(fn,"r");
		if(!f){write_log("can't open '%s' for reading",fn);return 0;}
		if(fread(&ph,sizeof(ph),1,f)!=1)write_log("packet read error");
		    else if(ph.phType!=2)write_log("packet don't 2+ format");
			else {
			    while(fread(&mh,sizeof(mh),1,f)==1) {
				while(fgetc(f)>0);i=0;
				while((a=fgetc(f))>0&&i<36)to[i++]=a;
				to[i]=0;i=0;
				while((a=fgetc(f))>0&&i<36)from[i++]=a;
				from[i]=0;
				while(fgetc(f)>0);while(fgetc(f)>0);
				write_log(" *msg:%d from: \"%s\", to: \"%s\"",n++,from,to);
			    }
		}
		fclose(f);
	}
	return 0;
}

int wazoosend(int zap)
{
	flist_t *l;
	int rc,ticskip=0;
	unsigned long total=totalf+totalm;

	write_log("wazoo send");sline("Init zsend...");
	rc=zmodem_sendinit(zap);
	sendf.cps=1;sendf.allf=totaln;sendf.ttot=totalf;
 	if(!rc) for(l=fl;l;l=l->next) {
		if(l->sendas) {
			rc=cfgi(CFG_AUTOTICSKIP)?ticskip:0;ticskip=0;
			if(!rc||!istic(l->tosend))rc=zmodem_sendfile(l->tosend,l->sendas,&total,&totaln);
			    else write_log("tic file '%s' auto%sed",l->tosend,rc==ZSKIP?"skipp":"refus");
			if(rc<0) break;
			if(!rc || rc==ZSKIP) {
				if(l->type==IS_REQ) was_req=1;
				flexecute(l);
			if(rc==ZSKIP||rc==ZFERR)ticskip=rc;
			}
		} else flexecute(l);
	}

	sline("Done zsend...");
	rc=zmodem_senddone();
	qpreset(1);
	if(rc<0) return 1;
	return 0;
}

int wazoorecv(int zap)
{
	int rc;
	write_log("wazoo receive");
	rc=zmodem_receive(cfgs(CFG_INBOUND),zap);
	qpreset(0);
	if(rc==RCDO || rc==ERROR) return 1;
	return 0;
}

int hydra(int mode, int hmod, int rh1)
{
	flist_t *l;
	int rc=XFER_OK,ticskip=0;

	sline("Hydra-%dk session", hmod*2);
	hydra_init(HOPT_XONXOFF|HOPT_TELENET, mode, hmod, cfgi(CFG_HRXWIN), cfgi(CFG_HTXWIN));
	for(l=fl;l;l=l->next) 
		if(l->sendas) {
			if(l->type==IS_REQ || !rh1) {
				rc=hydra_file(l->tosend, l->sendas);
				if(rc==XFER_ABORT) break;
				if(rc==XFER_OK || rc==XFER_SKIP) flexecute(l);
			}
		} else if(!rh1) flexecute(l);
	if(rc==XFER_ABORT) {
		hydra_deinit();
		return 1;
	}
	rc=hydra_file(NULL, NULL);

	for(l=fl;l;l=l->next) 
		if(l->sendas) {
			rc=cfgi(CFG_AUTOTICSKIP)?ticskip:0;ticskip=0;
			if(!rc||!istic(l->tosend))rc=hydra_file(l->tosend,l->sendas);
			    else write_log("tic file '%s' auto%sed",l->tosend,rc==XFER_SKIP?"skipp":"refus");
			if(rc==XFER_ABORT) break;
			if(rc==XFER_OK || rc==XFER_SKIP) flexecute(l);
			if(rc==XFER_SKIP||rc==XFER_SUSPEND)ticskip=rc;
		} else flexecute(l);
	if(rc==XFER_ABORT) {
		hydra_deinit();
		return 1;
	}
	rc=hydra_file(NULL, NULL);
	hydra_deinit();
	return rc==XFER_ABORT; 	
}

#define SIZES(x) (((x)<1024)?(x):((x)/1024))
#define SIZEC(x) (((x)<1024)?'b':'k')
 
void log_rinfo(ninfo_t *e)
{
	falist_t *i;
	struct tm *t;
	char s[MAX_STRING]={0};
	int k=0;

	if((i=e->addrs)) { write_log("address: %s", ftnaddrtoa(&i->addr));i=i->next; }
	for(;i;i=i->next) {
		if(k) xstrcat(s, " ", MAX_STRING);xstrcat(s, ftnaddrtoa(&i->addr), MAX_STRING);
		k++;if(k==2) { write_log("    aka: %s", s);k=0;s[0]=0; }
	}
	if(k) write_log("    aka: %s", s);
	write_log(" system: %s", e->name);
	write_log("   from: %s", e->place);
	write_log("  sysop: %s", e->sysop);
	write_log("  phone: %s", e->phone);
	write_log("  flags: [%d] %s", e->speed, e->flags);
	write_log(" mailer: %s", e->mailer);
	t=gmtime(&e->time);
	write_log("   time: %02d:%02d:%02d, %s",
		t->tm_hour, t->tm_min, t->tm_sec,
		e->wtime ? e->wtime : "unknown");
	if(e->holded && !e->files && !e->netmail)
		write_log(" for us: %d%c on hold", SIZES(e->holded), SIZEC(e->holded));
	else
		write_log(" for us: %d%c mail; %d%c files",
			SIZES(e->netmail), SIZEC(e->netmail),
			SIZES(e->files), SIZEC(e->files));
}

int emsisession(int mode, ftnaddr_t *calladdr, int speed)
{
	int rc, emsi_lo=0,proto;
	unsigned long nfiles;
	unsigned char *mydat;
	char *t, pr[3];
	falist_t *pp = NULL;
	qitem_t *q = NULL;

	was_req=0;got_req=0;receive_callback=receivecb;
	totaln=0;totalf=0;totalm=0;emsi_lo=0;
	if(mode) {
		write_log("starting outbound EMSI session");
		q=q_find(calladdr);
		if(q) {
			totalm=q->pkts;
			totalf=q_sum(q)+q->reqs;
		}
		mydat=(unsigned char*)emsi_makedat(calladdr,totalm,totalf,O_PUA,cfgs(CFG_PROTORDER),NULL,1);
		rc=emsi_send(mode, mydat);xfree(mydat);
		if(rc<0) return S_REDIAL|S_ADDTRY;
		rc=emsi_recv(mode, rnode);
		if(rc<0) return S_REDIAL|S_ADDTRY;
	    } else {
		rc=emsi_recv(mode, rnode);
		if(rc<0) {
			write_log("unable to establish EMSI session");
			return S_REDIAL|S_ADDTRY;
		}
		write_log("starting inbound EMSI session");
	}

	if(mode)title("Outbound session %s", ftnaddrtoa(&rnode->addrs->addr));
	    else {
		if((t=getenv("CALLER_ID")) && strcasecmp(t,"none")&&strlen(t)>3)
		  title("Inbound session %s (CID %s)",ftnaddrtoa(&rnode->addrs->addr), t);
		    else title("Inbound session %s",ftnaddrtoa(&rnode->addrs->addr));
	}			
	log_rinfo(rnode);
	for(pp=rnode->addrs;pp;pp=pp->next)
		bso_locknode(&pp->addr);
	if(mode) {
		if(!has_addr(calladdr, rnode->addrs)) {
			write_log("remote isn't %s!", ftnaddrtoa(calladdr));
			return S_FAILURE;
		}
		flkill(&fl, 0);totalf=0;totalm=0;
		for(pp=rnode->addrs;pp;pp=pp->next) 
			makeflist(&fl, &pp->addr);
		if(strlen(rnode->pwd)) rnode->options|=O_PWD;
	} else {
		for(pp=cfgal(CFG_ADDRESS);pp;pp=pp->next)
			if(has_addr(&pp->addr, rnode->addrs)) {
				write_log("remote also has %s!", ftnaddrtoa(&pp->addr));
				return S_FAILURE;
			}		
		nfiles=0;rc=0;
		for(pp=rnode->addrs;pp;pp=pp->next) {
			t=findpwd(&pp->addr);
			if(!t&&*rnode->pwd&&pp==rnode->addrs)write_log("remote has been password for us!");
			if(!t || !strcasecmp(rnode->pwd, t)) {
				makeflist(&fl, &pp->addr);
				if(t) rnode->options|=O_PWD;
			} else {
				write_log("password not matched for %s",ftnaddrtoa(&pp->addr));
				write_log("  (got '%s' instead of '%s')", rnode->pwd, t);
				rc=1;
			}
		}

		if(is_listed(rnode->addrs, cfgs(CFG_NLPATH), cfgi(CFG_NEEDALLLISTED)))
			rnode->options|=O_LST;
		if(rc) {
			emsi_lo|=O_BAD;
			rnode->options|=O_BAD;
		}
		if(!cfgs(CFG_FREQTIME)||rc) 
			emsi_lo|=O_NRQ;
		if(ccs && !checktimegaps(ccs))
			emsi_lo|=O_HRQ;
		if(checktimegaps(cfgs(CFG_MAILONLY)) ||
		   checktimegaps(cfgs(CFG_ZMH))||rc) 
			emsi_lo|=O_HXT|O_HRQ;

		pr[2]=0;pr[1]=0;pr[0]=0;
		for(t=cfgs(CFG_PROTORDER);*t;t++) {
#ifdef HYDRA8K16K
			if(*t=='8' && rnode->options&P_HYDRA8)
				{pr[0]='8';emsi_lo|=P_HYDRA8;break;}
			if(*t=='6' && rnode->options&P_HYDRA16)
				{pr[0]='6';emsi_lo|=P_HYDRA16;break;}
#endif/*HYDRA8K16K*/			
			if(*t=='H' && rnode->options&P_HYDRA)
				{pr[0]='H';emsi_lo|=P_HYDRA;break;}
			if(*t=='J' && rnode->options&P_JANUS)
				{pr[0]='J';emsi_lo|=P_JANUS;break;}
			if(*t=='D' && rnode->options&P_DIRZAP)
				{pr[0]='D';emsi_lo|=P_DIRZAP;break;}
			if(*t=='Z' && rnode->options&P_ZEDZAP)
				{pr[0]='Z';emsi_lo|=P_ZEDZAP;break;}
			if(*t=='1' && rnode->options&P_ZMODEM)
				{pr[0]='1';emsi_lo|=P_ZMODEM;break;}
		}
		if(strchr(cfgs(CFG_PROTORDER),'C'))pr[1]='C';
		    else rnode->chat=0;
		if(!pr[0]) emsi_lo|=P_NCP;
		mydat=(unsigned char*)emsi_makedat(&rnode->addrs->addr, totalm, totalf, emsi_lo,
						   pr, NULL, !(emsi_lo&O_BAD));
		rc=emsi_send(0, mydat);xfree(mydat);
		if(rc<0) {
			flkill(&fl,0);
			return S_REDIAL|S_ADDTRY;
		}
	}
	if(cfgs(CFG_RUNONEMSI)&&*ccs) {
		write_log("starting %s %s",ccs,rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"));
		execnowait(ccs,rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),NULL,NULL);
	}
	write_log("we have: %d%c mail; %d%c files",SIZES(totalm), SIZEC(totalm),SIZES(totalf), SIZEC(totalf));
	rnode->starttime=time(NULL);
	if(cfgi(CFG_MAXSESSION)) alarm(cci*60);
	DEBUG(('S',1,"Maxsession: %d",cci));
	
	qemsisend(rnode);
	qpreset(0);qpreset(1);

	proto=(mode?rnode->options:emsi_lo)&P_MASK;
	switch(proto) {
	case P_NCP: 
		write_log("no compatible protocols");
		flkill(&fl, 0);
		return S_FAILURE;
	case P_ZMODEM:
		t="ZModem-1k";break;
	case P_ZEDZAP:
		t="ZedZap";break;
	case P_DIRZAP:
		t="DirZap";break;
#ifdef HYDRA8K16K
	case P_HYDRA8:
		t="Hydra-8k";break;
	case P_HYDRA16:
		t="Hydra-16k";break;
#endif/*HYDRA8K16K*/	
	case P_HYDRA:
		t="Hydra";break;
	case P_JANUS:
		t="Janus";break;
	case P_TCPP:
		t="IFCTCP";break;
	default:
		t="Unknown";		
	}
	DEBUG(('S',1,"emsopts: %s %x %x %x %x", t, rnode->options&P_MASK, rnode->options, emsi_lo, rnode->chat));
	write_log("options: %s%s%s%s%s%s%s%s%s%s", t,
		(rnode->options&O_LST)?"/LST":"",
		(rnode->options&O_PWD)?"/PWD":"",
		(rnode->options&O_HXT)?"/MO":"",
		(rnode->options&O_HAT)?"/HAT":"",
		(rnode->options&O_HRQ)?"/HRQ":"",
		(rnode->options&O_NRQ)?"/NRQ":"",
		(rnode->options&O_FNC)?"/FNC":"",
		(rnode->options&O_BAD)?"/BAD":"",
		(rnode->chat)?"/CHT":"");
	chatinit(proto);
	switch(proto) {
	case P_ZEDZAP:
	case P_DIRZAP:
	case P_ZMODEM:  
		recvf.cps=1;recvf.ttot=rnode->netmail+rnode->files;
		if(mode) {
			rc=wazoosend((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0));
			if(!rc) rc=wazoorecv((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0));
			if(got_req && !rc) rc=wazoosend((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0));
		} else {
			rc=wazoorecv(((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0))|0x0100);
			if(rc) return S_REDIAL;
			rc=wazoosend((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0));
			if(was_req) rc=wazoorecv((proto&P_ZEDZAP)?1:((proto&P_DIRZAP)?2:0));
		}
		flkill(&fl, !rc);
		return rc?S_REDIAL:S_OK;
	case P_HYDRA:
#ifdef HYDRA8K16K
	case P_HYDRA8:
	case P_HYDRA16:
#endif/*HYDRA8K16K*/
		sendf.allf=totaln;sendf.ttot=totalf+totalm;
		recvf.ttot=rnode->netmail+rnode->files;
		switch(proto) {
		case P_HYDRA:   rc=1;break;
#ifdef HYDRA8K16K
		case P_HYDRA8:  rc=4;break;
		case P_HYDRA16: rc=8;break;
#endif/*HYDRA8K16K*/		
		}
		rc=hydra(mode, rc, rnode->options&O_RH1);
		flkill(&fl, !rc);
		return rc?S_REDIAL:S_OK;
	case P_JANUS:
		sendf.allf=totaln;sendf.ttot=totalf+totalm;
		recvf.ttot=rnode->netmail+rnode->files;
		rc=janus();
		flkill(&fl, !rc);
		return rc?S_REDIAL:S_OK;
	}
	return S_OK;
}

void sessalarm(int sig)
{
	signal(SIGALRM, SIG_DFL);
	write_log("session limit of %d minutes is over",cfgi(CFG_MAXSESSION));
	tty_hangedup=1;
}

int session(int mode, int type, ftnaddr_t *calladdr, int speed)
{
	int rc;
	time_t sest;
	FILE *h;
	char s[MAX_STRING];
	falist_t *pp;

	rnode->realspeed=effbaud=speed;rnode->starttime=0;
	if(!mode) rnode->options|=O_INB;
	if(is_ip) rnode->options|=O_TCP;
	runtoss=0;bzero(&ndefaddr,sizeof(ndefaddr));
	if(calladdr)ADDRCPY(ndefaddr,(*calladdr));

	rc=cfgi(CFG_MINSPEED);
	if(rc && speed<rc) {
		write_log("connection speed is too slow");
		return S_REDIAL|S_ADDTRY;
	}
		
	memset(&sendf,0, sizeof(sendf));
	memset(&recvf,0, sizeof(recvf));
	signal(SIGALRM, sessalarm);
	signal(SIGTERM, tty_sighup);
	signal(SIGINT, tty_sighup);
	signal(SIGCHLD, SIG_DFL);

	switch(type) {
	case SESSION_AUTO:
		write_log("trying EMSI...");
		rc=emsi_init(mode);
		if(rc<0) {
			write_log("unable to establish EMSI session");
			return S_REDIAL|S_ADDTRY;
		}
		rc=emsisession(mode, calladdr, speed);
		break;
	case SESSION_EMSI: 
		rc=emsisession(mode, calladdr, speed);
		break;
	default:
		write_log("unsupported session type! (%d)", type);
		return S_REDIAL|S_ADDTRY;
	}
	for(pp=rnode->addrs;pp;pp=pp->next) bso_unlocknode(&pp->addr);
	if((rnode->options&O_NRQ&&!cfgi(CFG_IGNORENRQ))||rnode->options&O_HRQ)rc|=S_HOLDR;
	if(rnode->options&O_HXT) rc|=S_HOLDX;
	if(rnode->options&O_HAT) rc|=S_HOLDA;
	signal(SIGALRM, SIG_DFL);
	sest=rnode->starttime?time(NULL)-rnode->starttime:0;
	write_log("total: %d:%02d:%02d online, %d%c sent, %d%c received",
		sest/3600,sest%3600/60,sest%60,
		SIZES(sendf.toff-sendf.soff), SIZEC(sendf.toff-sendf.soff),
		SIZES(recvf.toff-recvf.soff), SIZEC(recvf.toff-recvf.soff));
	write_log("session with %s %s [%s]",
		rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
		((rc&S_MASK)==S_OK)?"successful":"failed", M_STAT);
	if(rnode->starttime && cfgs(CFG_HISTORY)) {
		h=fopen(ccs,"at");
		if(!h) write_log("can't open %s for writing!",ccs);
		else {
			fprintf(h,"%s,%ld,%ld,%s,%s%s%c%d,%d,%d\n",
					rnode->tty,rnode->starttime, sest,
					rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
					(rnode->options&O_PWD)?"P":"",(rnode->options&O_LST)?"L":"",
					(rnode->options&O_INB)?'I':'O',((rc&S_MASK)==S_OK)?1:0,
					sendf.toff-sendf.stot, recvf.toff-recvf.stot);
			fclose(h);
		}
	}
	while(--freq_pktcount) {
		snprintf(s,MAX_STRING,"/tmp/qpkt.%04lx%02x",(long)getpid(),freq_pktcount);
		if(fexist(s)) lunlink(s);
	}
	if(chatlg)chatlog_done();
	if(cfgs(CFG_AFTERSESSION)) {
		write_log("starting %s %s %c %d",
			cfgs(CFG_AFTERSESSION),
			rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
			(rnode->options&O_INB)?'I':'O',((rc&S_MASK)==S_OK)?1:0);
		execnowait(cfgs(CFG_AFTERSESSION),
				   rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
				   (rnode->options&O_INB)?"I":"O",((rc&S_MASK)==S_OK)?"1":"0");
	}
	if(cfgs(CFG_AFTERMAIL)){
		if(recvf.toff-recvf.soff!=0||runtoss){
			write_log("starting %s %s %c %d",
				cfgs(CFG_AFTERMAIL),
				rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
				(rnode->options&O_INB)?'I':'O',((rc&S_MASK)==S_OK)?1:0);
			execnowait(cfgs(CFG_AFTERMAIL),
				   	rnode->addrs?ftnaddrtoa(&rnode->addrs->addr):(calladdr?ftnaddrtoa(calladdr):"unknown"),
				   	(rnode->options&O_INB)?"I":"O",((rc&S_MASK)==S_OK)?"1":"0");
		}
	}
	return rc&~S_ADDTRY;
}
