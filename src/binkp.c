/******************************************************************
 * BinkP protocol implementation.
 * $Id: binkp.c,v 1.33 2004/04/17 07:25:18 sisoft Exp $
 ******************************************************************/
#include "headers.h"
#ifdef WITH_BINKP
#include "binkp.h"
#include "tty.h"
#include "crc.h"

extern int receivecb(char *fn);
static unsigned long key_in[3],key_out[3];
static int opt_nr,opt_nd,opt_md,opt_cr,opt_mb,opt_cht;
static char *mess[]={"NUL","ADR","PWD","FILE","OK","EOB","GOT",
		    "ERR","BSY","GET","SKIP","RESERVED","CHAT"};

static int datas(byte *buf,word len)
{
	DEBUG(('B',4,"datas block len=%u%s",len,(opt_cr==O_YES)?" (c)":""));
	if(opt_cr==O_YES)encrypt_buf((char*)buf,len,key_out);
	return(PUTBLK(buf,len));
}

static int msgs(int msg,char *str,...)
{
	va_list args;
	unsigned len;
	va_start(args,str);
	vsnprintf((char*)(txbuf+3),0x7fff,str,args);
	DEBUG(('B',2,"msgs M_%s '%s'",mess[msg],txbuf+3));
	va_end(args);
	len=strlen((char*)(txbuf+3));
	if(++len>0x7fff)len=0x7fff;
	*txbuf=((len>>8)&0x7f)|0x80;
	txbuf[1]=len&0xff;txbuf[2]=msg;
	return(datas(txbuf,(word)(len+2)));
}

static int msgr(char *buf)
{
	time_t t1;
	int rc=BPM_NONE,d;
	register int c;
	unsigned len=0,i;
	*buf=0;d=2;
	if((c=GETCHAR(0))>=0) {
		if(opt_cr==O_YES)update_keys(key_in,c^=decrypt_byte(key_in));
		if(c&0x80)d=0; else d=1;
		c&=0x7f;len=c<<8;
		t1=t_set(60);
		while((c=GETCHAR(1))==TIMEOUT&&!t_exp(t1))getevt();
		if(c<0||t_exp(t1))return c;
		if(opt_cr==O_YES)update_keys(key_in,c^=decrypt_byte(key_in));
		len+=c&0xff;
		for(i=0;i<len;i++) {
			while((c=GETCHAR(1))==TIMEOUT&&!t_exp(t1))getevt();
			if(c<0||t_exp(t1))return c;
			if(opt_cr==O_YES)update_keys(key_in,c^=decrypt_byte(key_in));
			rxbuf[i]=c;
		}
		if(len>0x7fff)len=0x7fff;
		rc=*rxbuf;rxbuf[len]=0;
		if(!d&&(rc>BPM_MAX||rc<BPM_MIN)) {
			rc=BPM_NONE;
			DEBUG(('B',1,"msgr unknown message: %d%s, skipped",rc,(opt_cr==O_YES)?" (crypted)":""));
		} else {
			if(!d)DEBUG(('B',2,"msgr M_%s '%s'%s",mess[rc],rxbuf+1,(opt_cr==O_YES)?" (crypted)":""));
			    else DEBUG(('B',2,"msgr data, len=%d%s",len,(opt_cr==O_YES)?" (crypted)":""));
		}
		if(len&&!d)memcpy(buf,rxbuf+1,len);
		    else *(long*)buf=(long)len;
	}
	if(c<0&&c!=OK&&c!=TIMEOUT)return c;
	return((d==1)?BPM_DATA:((!d)?rc:BPM_NONE));
}

static int f_pars(char *s,char **fn,size_t *sz,time_t *tm,size_t *offs)
{
	char *n=s,*name,*size,*time,*off=NULL;
	DEBUG(('B',4,"pars: in '%s'",n));
	name=strsep(&n," ");
	size=strsep(&n," ");
	time=strsep(&n," ");
	if(offs)off=strsep(&n," ");
	if(name&&size&&time&&(!offs||off)) {
		*fn=name;
		*sz=atol(size);
		*tm=atol(time);
		if(offs)*offs=atol(off);
		DEBUG(('B',3,"pars: name=%s, size=%ld, time=%ld, offs=%ld",*fn,*sz,*tm,offs?*offs:-3));
		return 0;
	}
	return 1;
}

int bink_devfree()
{
	if(opt_cht==O_YES)return 1;
	return 0;
}

int bink_devsend(byte *str,word len)
{
	if(opt_cht==O_YES) {
		msgs(BPM_CHAT,"%s",(char*)str);
		return 1;
	}
	return 0;
}

static void bink_devrecv(char *data)
{
	c_devrecv((byte*)data,strlen(data)+1);
}

int binkpsession(int mode,ftnaddr_t *remaddr)
{
	char tmp[BP_BUFS],*p,*fname=NULL,*rfname=NULL;
	int send_file=0,recv_file=0,i;
	int rc=0,n=0,chal_len=0,mes,cls;
	int nofiles=0,wait_got=0,bp_ver=10;
	int sent_eob=0,recv_eob=0,ticskip=0;
	size_t fsize,foffs;
	falist_t *pp=NULL;
	qitem_t *q=NULL;
	flist_t *lst=NULL;
	unsigned char chal[64];
	ftnaddr_t *ba=NULL;
	FTNADDR_T(fa);
	struct tm *tt;
	sts_t sts;
	time_t ti,t1,ftime,rmtime=0;
	totaln=0;totalf=0;totalm=0;got_req=0;
	rxstate=1;receive_callback=receivecb;
	opt_nr=opt_nd=opt_md=opt_cr=opt_mb=opt_cht=O_NO;
	for(p=cfgs(CFG_BINKPOPT);*p;p++)switch(tolower(*p)) {
	    case 'm': opt_md|=O_WANT; break;
	    case 'c': opt_cr|=O_WE; break;
	    case 'd': opt_nd|=O_NO;/*mode?O_THEY:O_NO;*/
	    case 'r': opt_nr|=mode?O_WANT:O_NO; break;
	    case 'b': opt_mb|=O_WANT; break;
	    case 'p': opt_md|=O_NEED|O_WANT; break;
	    case 't': opt_cht|=O_WANT; break;
	    default: write_log("unknown binkp option: '%c'",*p);
	}
	write_log("starting %sbound BinkP session",mode?"out":"in");
	txbuf=(byte*)xcalloc(BP_BUFFER,1);
	rxbuf=(byte*)xcalloc(BP_BUFFER,1);
	txstate=mode?BPO_Init:BPI_Init;
	if(mode)q=q_find(remaddr);
	if(q){totalm=q->pkts;totalf=q_sum(q)+q->reqs;}
	if(!mode&&opt_md) {
		long rnd=(long)random(),utm=time(NULL);
		int pid=((int)getpid())^((int)random());
		STORE32(chal,rnd);
		STORE16(chal+4,pid)
		STORE32(chal+6,utm);
		chal_len=10;
	}
	sline("BinkP handshake");
	t1=t_set(cfgi(CFG_HSTIMEOUT));
	while(rxstate&&!t_exp(t1)) {
		switch(txstate) {
		    case BPO_Init:
		    case BPI_Init:
			DEBUG(('B',3,"state: init (%d)",txstate));
			if(!mode&&chal_len>0&&(opt_md&O_WANT)) {
				char chall[128];
				strbin2hex(chall,chal,chal_len);
				msgs(BPM_NUL,"OPT CRAM-MD5-%s",chall);
			}
			recode_to_remote(cfgs(CFG_STATION));
			msgs(BPM_NUL,"SYS %s",ccs);
			recode_to_remote(cfgs(CFG_SYSOP));
			msgs(BPM_NUL,"ZYZ %s",ccs);
			recode_to_remote(cfgs(CFG_PLACE));
			msgs(BPM_NUL,"LOC %s",ccs);
			recode_to_remote(cfgs(CFG_FLAGS));
			snprintf(tmp,120,"%d,%s",cfgi(CFG_SPEED),ccs);
			msgs(BPM_NUL,"NDL %s",tmp);
			recode_to_remote(cfgs(CFG_PHONE));
			msgs(BPM_NUL,"PHN %s",ccs);
			ti=time(NULL);tt=gmtime(&ti);
			strftime(tmp,120,"%a, %d %b %Y %H:%M:%S GMT",tt);
			msgs(BPM_NUL,"TIME %s",tmp);
			snprintf(tmp,128,"%s-%s/%s",qver(0),qver(1),qver(2));
			strtr(tmp,' ','-');
			msgs(BPM_NUL,"VER %s %s",tmp,BP_VERSION);
			if(mode)msgs(BPM_NUL,"OPT NDA%s%s%s%s%s",
					(opt_nr&O_WANT)?" NR":"",
					(opt_nd&O_THEY)?" ND":"",
					(opt_mb&O_WANT)?" MB":"",
					(opt_cr&O_WE)?" CRYPT":"",
					(opt_cht&O_WANT)?" CHAT":"");
			pp=cfgal(CFG_ADDRESS);
			if(mode) {
				ba=akamatch(remaddr,pp);
				xstrcpy(tmp,ftnaddrtoda(ba),BP_BUFS);
			    } else {
				xstrcpy(tmp,ftnaddrtoda(&pp->addr),BP_BUFS);
				pp=pp->next;ba=NULL;
			}
			for(;pp;pp=pp->next)
			    if(&pp->addr!=ba) {
				xstrcat(tmp," ",BP_BUFS);
				xstrcat(tmp,ftnaddrtoda(&pp->addr),BP_BUFS);
			}
			msgs(BPM_ADR,tmp);
			txstate=(txstate==BPO_Init)?BPO_WaitNul:BPI_WaitAdr;
			break;
		    case BPO_SendPwd:
			DEBUG(('B',3,"state: sendpwd (%d)",txstate));
			msgs(BPM_NUL,"TRF %lu %lu",totalm,totalf);
			p=findpwd(remaddr);
			if(!p)msgs(BPM_PWD,"-");
			    else {
				if(opt_md==O_YES) {
					char dig_h[33];
					unsigned char dig_b[16];
					md5_cram_get((unsigned char*)p,chal,chal_len,dig_b);
					strbin2hex(dig_h,dig_b,16);
					msgs(BPM_PWD,"CRAM-MD5-%s",dig_h);
				} else msgs(BPM_PWD,"%s",p);
			}
			txstate=BPO_WaitAdr;
			break;
		    case BPO_Auth:
		    case BPI_Auth:
			rc=0;
			DEBUG(('B',3,"state: auth (%d)",txstate));
			title("%sbound session %s",mode?"Out":"In",ftnaddrtoa(&rnode->addrs->addr));
			if(BSO)for(pp=rnode->addrs;pp;pp=pp->next)
				rc+=bso_locknode(&pp->addr,LCK_s);
			if(ASO)for(pp=rnode->addrs;pp;pp=pp->next)
				rc+=aso_locknode(&pp->addr,LCK_s);
			if(!rc) {
				log_rinfo(rnode);
				write_log("can't lock outbound for %s!",ftnaddrtoa(mode?remaddr:&rnode->addrs->addr));
				msgs(BPM_BSY,"All addresses are busy");
				rc=S_REDIAL;goto failed;
			}
			if(mode) {
				if(!has_addr(remaddr,rnode->addrs)) {
					log_rinfo(rnode);
					write_log("remote isn't %s!",ftnaddrtoa(remaddr));
					msgs(BPM_ERR,"Sorry, you are not who I need");
					rc=S_FAILURE;goto failed;
				}
				flkill(&fl,0);totalf=0;totalm=0;
				for(pp=rnode->addrs;pp;pp=pp->next) {
					makeflist(&fl,&pp->addr,mode);
					if((p=findpwd(&pp->addr))) {
						rnode->options|=O_PWD;
						if(!rnode->pwd)restrcpy(&rnode->pwd,p);
					}
				}
				if(!rnode->pwd)restrcpy(&rnode->pwd,"-");
				if(is_listed(rnode->addrs,cfgs(CFG_NLPATH),cfgi(CFG_NEEDALLLISTED)))
					rnode->options|=O_LST;
			} else {
				for(pp=cfgal(CFG_ADDRESS);pp;pp=pp->next)
					if(has_addr(&pp->addr,rnode->addrs)) {
		    				log_rinfo(rnode);
						write_log("remote also has %s!",ftnaddrtoa(&pp->addr));
						msgs(BPM_ERR,"Sorry, you also has one of my aka's");
						rc=S_FAILURE;goto failed;
					}
				if(is_listed(rnode->addrs,cfgs(CFG_NLPATH),cfgi(CFG_NEEDALLLISTED)))
					rnode->options|=O_LST;
				rc=0;
				if(chal_len>0&&!strncmp(rnode->pwd,"CRAM-MD5-",9)) {
					opt_md|=O_WE;
					rc=9;
				} else if(opt_md&O_NEED) {
	    				log_rinfo(rnode);
					write_log("got disabled plain password");
					msgs(BPM_ERR,"You must support MD5");
					rc=S_FAILURE;goto failed;
				}
				if(opt_md&O_WE)opt_md=O_YES;
				for(pp=rnode->addrs;pp;pp=pp->next) {
					p=findpwd(&pp->addr);n=0;
					if(!p||(!rc&&!strcasecmp(rnode->pwd,p)))n=1;
					    else if(p&&rc) {
						char dig_h[33];
						unsigned char dig_b[16];
						md5_cram_get((unsigned char*)p,chal,chal_len,dig_b);
						strbin2hex(dig_h,dig_b,16);
						if(!strcasecmp(rnode->pwd+rc,dig_h))n=1;
						restrcpy(&rnode->pwd,p);
					}
					if(n) {
						makeflist(&fl,&pp->addr,mode);
						if(p)rnode->options|=O_PWD;
					} else {
						log_rinfo(rnode);
						write_log("password not matched for %s",ftnaddrtoa(&pp->addr));
						msgs(BPM_ERR,"Security violation");
						rnode->options|=O_BAD;
						rc=S_FAILURE;goto failed;
					}
				}
				if((opt_nd&O_WE)&&!(opt_nd&O_EXT))opt_nd|=O_THEY;
				if(!(opt_nd&O_WE)&&!(opt_nd&O_EXT))opt_nd&=~O_THEY;
				if((opt_nr&O_WANT)&&!(opt_nd&O_EXT)&&!(opt_nd&O_WE)) {
					if(bp_ver>=11)opt_nr|=O_WE;
					    else opt_nr&=~O_WANT;
				}
			}
			if(txstate==BPI_Auth) {
				if(!(rnode->options&O_PWD)||opt_md!=O_YES)opt_cr=O_NO;
				snprintf(tmp,128,"%s%s%s%s%s%s",
					(opt_nr&O_WANT)?" NR":"",
					(opt_nd&O_THEY)?" ND":"",
					(opt_mb&O_WANT)?" MB":"",
					(opt_cht&O_WANT)?" CHAT":"",
					((!(opt_nd&O_WE))!=(!(opt_nd&O_THEY)))?" NDA":"",
					((opt_cr&O_WE)&&(opt_cr&O_THEY))?" CRYPT":"");
				if(strlen(tmp))msgs(BPM_NUL,"OPT %s",tmp);
				msgs(BPM_NUL,"TRF %lu %lu",totalm,totalf);
				msgs(BPM_OK,(rnode->options&O_PWD)?"secure":"non-secure");
				rxstate=0;
				break;
			}
			txstate=BPO_WaitOk;
			break;
		}
		if(rxstate) {
			getevt();
			rc=msgr(tmp);
			if(rc==RCDO||tty_hangedup) {
				rc=S_REDIAL;goto failed;
			}
			switch(rc) {
			    case BPM_NONE: case TIMEOUT: case BPM_DATA:
			    case BPM_CHAT: case BPM_RESERVED:
				break;
			    case BPM_NUL: {
				char *n;
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				recode_to_local(tmp+4);
				if(!strncmp(tmp,"SYS ",4))restrcpy(&rnode->name,tmp+4);
				else if(!strncmp(tmp,"ZYZ ",4))restrcpy(&rnode->sysop,tmp+4);
				else if(!strncmp(tmp,"LOC ",4))restrcpy(&rnode->place,tmp+4);
				else if(!strncmp(tmp,"PHN ",4))restrcpy(&rnode->phone,tmp+4);
				else if(!strncmp(tmp,"NDL ",4)) {
					long x;rnode->speed=TCP_SPEED;
					if((x=atol(tmp+4))>=300) {
						rnode->speed=x;
						restrcpy(&rnode->flags,strchr(tmp+4,',')+1);
					} else restrcpy(&rnode->flags,tmp+4);
				} else if(!strncmp(tmp,"TIME ",5)) {
					long gmt=0;
					ti=time(NULL);
					tt=gmtime(&ti);
					n=strchr(tmp,':')-2;
					if(*n==' ')n++;
					tt->tm_hour=atoi(n);
					n=strchr(tmp,':')+1;
					tt->tm_min=atoi(n);n+=3;
					tt->tm_sec=atoi(n);
					n=strrchr(tmp,' ');
					if(n&&(n[1]=='+'||n[1]=='-'))
						gmt=(n[2]-'0')*600+(n[3]-'0')*60+(n[4]-'0');
					if(gmt<-86400||gmt>86400)gmt=0;
					rnode->time=mktime(tt)+((n[1]=='-')?-gmt:gmt)*60;
				} else if(!strncmp(tmp,"TRF ",4)) {
					n=tmp+4;rnode->netmail=atoi(n);
					n=strchr(n,' ');
					if(n)rnode->files=atoi(n+1);
				} else if(!strncmp(tmp,"OPT ",4)) {
					n=tmp+4;
					while((p=strsep(&n," "))) {
						if(!strcmp(p,"NR"))opt_nr|=O_WE;
						else if(!strcmp(p,"MB"))opt_mb|=O_WE;
						else if(!strcmp(p,"ND"))opt_nd|=O_WE;
						else if(!strcmp(p,"NDA"))opt_nd|=O_EXT;
						else if(!strcmp(p,"CHAT"))opt_cht|=O_WE;
						else if(!strcmp(p,"CRYPT"))opt_cr|=O_THEY;
						else if(!strncmp(p,"CRAM-MD5-",9)) {
							if(strlen(p+9)>(2*sizeof(chal)))
							    write_log("binkp got too long challenge string");
							    else {
								chal_len=strhex2bin(chal,p+9);
								if(chal_len>0)opt_md|=O_THEY;
							}
							if((opt_md&O_THEY)&&(opt_md&O_WANT))opt_md=O_YES;
						} else DEBUG(('B',1,"got unknown option '%s'",p));
					}
				} else if(!strncmp(tmp,"VER ",4)) {
					restrcpy(&rnode->mailer,tmp+4);
					n=strrchr(tmp+4,' ');
					if(n&&(n[1]=='('||n[1]=='['))n++;
					if(!n||strncasecmp(n+1,"binkp",5)||!(n=strchr(n,'/')))
					    write_log("BinkP: got bad NUL VER message: %s",tmp);
						else bp_ver=10*(n[1]-'0')+n[3]-'0';
				} else write_log("BinkP: got invalid NUL: \"%s\"",tmp);
				if(txstate==BPO_WaitNul)txstate=BPO_SendPwd;
				} break;
			    case BPM_ADR:
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				if(txstate==BPO_WaitAdr||txstate==BPI_WaitAdr) {
					char *n=tmp;
					while(*n==' ')n++;
					falist_kill(&rnode->addrs);
					while((p=strsep(&n," ")))
					    if(parseftnaddr(p,&fa,NULL,0))
						if(!falist_find(rnode->addrs,&fa))
						    falist_add(&rnode->addrs,&fa);
					txstate=(txstate==BPO_WaitAdr)?BPO_Auth:BPI_WaitPwd;
				}
				break;
			    case BPM_PWD:
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				if(txstate==BPI_WaitPwd) {
					restrcpy(&rnode->pwd,tmp);
					txstate=BPI_Auth;
				}
				break;
			    case BPM_OK:
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				if(!strcasecmp(tmp,"non-secure"))rnode->options&=~O_PWD;
				if(opt_nd==O_WE||opt_nd==O_THEY)opt_nd=O_NO;
				if(txstate==BPO_WaitOk)rxstate=0;
				break;
			    case BPM_ERR:
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				write_log("BinkP error: \"%s\"",tmp);
			    case ERROR:
				DEBUG(('B',3,"got: ERROR, state=%d",txstate));
				rc=S_FAILURE|S_ADDTRY;goto failed;
			    case BPM_BSY:
				DEBUG(('B',3,"got: M_%s, state=%d",mess[rc],txstate));
				write_log("BinkP busy: \"%s\"",tmp);
				rc=S_REDIAL;goto failed;
			    default:
				DEBUG(('B',2,"got: unknown msg %d, state=%d",rc,txstate));
			}
		}
	}
	if(t_exp(t1)) {
		log_rinfo(rnode);
		write_log("BinkP handshake timeout");
		msgs(BPM_ERR,"Handshake timeout");
		rc=S_REDIAL;goto failed;
	}
	if(!rnode->phone||!*rnode->phone)restrcpy(&rnode->phone,"-Unpublished-");
	if(!(rnode->options&O_PWD)||opt_md!=O_YES)opt_cr=O_NO;
	if((opt_cr&O_WE)&&(opt_cr&O_THEY)) {
		DEBUG(('B',2,"enable crypting messages"));
		opt_cr=O_YES;
		if(mode) {
			init_keys(key_out,rnode->pwd);
			init_keys(key_in,"-");
			for(p=rnode->pwd;*p;p++)update_keys(key_in,(int)*p);
		    } else {
			init_keys(key_in,rnode->pwd);
			init_keys(key_out,"-");
			for(p=rnode->pwd;*p;p++)update_keys(key_out,(int)*p);
		}
	}
	log_rinfo(rnode);
	write_log("we have: %d%c mail; %d%c files",SIZES(totalm),SIZEC(totalm),SIZES(totalf),SIZEC(totalf));
	rnode->starttime=time(NULL);
	if(cfgi(CFG_MAXSESSION))alarm(cci*60);
	DEBUG(('S',1,"Maxsession: %d",cci));
	qemsisend(rnode);
	qpreset(0);qpreset(1);
	if(opt_cht&O_WANT)chatinit(0);
	if(opt_nd&O_WE||(mode&&(opt_nr&O_WANT)&&bp_ver>=11))opt_nr|=O_WE;
	if((opt_cht&O_WE)&&(opt_cht&O_WANT))opt_cht=O_YES;
	if(bp_ver>=11||(opt_md&O_WE))opt_mb=O_YES;
	write_log("options: BinkP%s%s%s%s%s%s%s%s%s",
		(rnode->options&O_LST)?"/LST":"",
		(rnode->options&O_PWD)?"/PWD":"",
		(opt_nr&O_WE)?"/NR":"",
		((opt_nd&O_WE)&&(opt_nd&O_THEY))?"/ND":"",
		((opt_nd&O_WE)&&!(opt_nd&O_THEY)&&(opt_nd&O_EXT))?"/NDA":"",
		(opt_mb==O_YES)?"/MB":"",
		(opt_md==O_YES)?"/MD5":"/Plain",
		(opt_cr==O_YES)?"/CRYPT":"",
		(opt_cht==O_YES)?"/Chat":"");
	sendf.allf=totaln;sendf.ttot=totalf+totalm;
	recvf.ttot=rnode->netmail+rnode->files;
	effbaud=rnode->speed;lst=fl;
	if(BSO)bso_getstatus(&rnode->addrs->addr,&sts);
	    else if(ASO)aso_getstatus(&rnode->addrs->addr,&sts);
	sline("BinkP session");
	t1=t_set(BP_TIMEOUT);
	mes=0;cls=0;
	DEBUG(('B',1,"established binkp ver %d session. nr=%d,nd=%d,md=%d,mb=%d,cr=%d,cht=%d",bp_ver,opt_nr,opt_nd,opt_md,opt_mb,opt_cr,opt_cht));
	while(((sent_eob<2||recv_eob<2)||(opt_cht==O_YES&&chattimer>1&&!t_exp(chattimer)))&&!t_exp(t1)) {
		if(!send_file&&sent_eob<2&&!txfd&&!nofiles) {
			DEBUG(('B',2,"find files"));
			if(lst&&lst!=fl)lst=lst->next;
			if(lst)rc=cfgi(CFG_AUTOTICSKIP)?ticskip:0;
			while(lst&&!txfd) {
				if(!lst->sendas||(rc&&istic(lst->tosend))||!(txfd=txopen(lst->tosend,lst->sendas))) {
					if(rc&&istic(lst->tosend))write_log("tic file '%s' auto%sed",lst->tosend,rc==1?"skipp":"suspend");
					if(rc!=2)flexecute(lst);
					lst=lst->next;rc=0;
				}
			}
			ticskip=0;
			if(lst&&txfd) {
				DEBUG(('B',1,"found: %s",sendf.fname));
				send_file=1;txpos=(opt_nr&O_WE)?-1:0;
				msgs(BPM_FILE,"%s %ld %ld %ld",sendf.fname,(long)sendf.ftot,sendf.mtime,txpos);
				sent_eob=0;cls=1;
				t1=t_set(BP_TIMEOUT);
			} else nofiles=1;
		}
		if(send_file&&txfd&&txpos>=0) {
			if((n=fread(txbuf+2,1,BP_BLKSIZE,txfd))<0) {
				sline("binkp: file read error");
				DEBUG(('B',1,"binkp: file read error"));
				txclose(&txfd,FOP_ERROR);
				send_file=0;
			} else {
				DEBUG(('B',2,"readed %d bytes of %d (d)",n,BP_BLKSIZE));
				if(n) {
					*txbuf=((n>>8)&0x7f);txbuf[1]=n&0xff;
					datas(txbuf,(word)(n+2));
					txpos=n+2;sendf.foff+=n;
					t1=t_set(BP_TIMEOUT);
					qpfsend();
				}
				if(n<BP_BLKSIZE) {
					wait_got=1;
					send_file=0;
				}
			}
		}
		if(nofiles&&!wait_got&&!sent_eob) {
			msgs(BPM_EOB,NULL);
			t1=t_set(BP_TIMEOUT);
			sent_eob=1;
			if(opt_mb!=O_YES)sent_eob++;
		}
		getevt();
		rc=msgr(tmp);
		if(rc==RCDO||tty_hangedup) {
			DEBUG(('B',1,"msgr: connect aborted"));
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			rc=S_REDIAL;goto failed;
		}
		switch(rc) {
		    case BPM_NONE: case TIMEOUT: case BPM_RESERVED:
			break;
		    case BPM_NUL: {
			char *n;
			DEBUG(('B',3,"got: M_%s (%s)",mess[rc],tmp));
			t1=t_set(BP_TIMEOUT);mes++;
			if(!strncmp(tmp,"TRF ",4)) {
				n=tmp+4;rnode->netmail=atoi(n);
				n=strchr(n,' ');
				if(n)rnode->files=atoi(n+1);
				if(rnode->files||rnode->netmail)
				    write_log("traffic: %d%c mail; %d%c files",
					SIZES(rnode->netmail), SIZEC(rnode->netmail),
					    SIZES(rnode->files), SIZEC(rnode->files));
			} else DEBUG(('B',2,"message ignored"));
		      } break;
		    case BPM_DATA:
			DEBUG(('B',3,"got: data"));
			t1=t_set(BP_TIMEOUT);
			if(recv_file) {
				if((n=fwrite(rxbuf,1,*(long*)tmp,rxfd))<0) {
					recv_file=0;mes++;
					sline("binkp: file write error");
					write_log("can't write %s, suspended.",recvf.fname);
					msgs(BPM_SKIP,"%s %ld %ld",rfname,(long)recvf.ftot,rmtime);
					rxclose(&rxfd,FOP_ERROR);
				} else {
					rxpos+=*(long*)tmp;
					recvf.foff+=*(long*)tmp;
					qpfrecv();
					if(rxpos>recvf.ftot) {
						recv_file=0;mes++;
						write_log("binkp: got too many data (%ld, %ld expected)",rxpos,(long)recvf.ftot);
						rxclose(&rxfd,FOP_SUSPEND);
						msgs(BPM_SKIP,"%s %ld %ld",rfname,(long)recvf.ftot,rmtime);
					} else if(rxpos==recvf.ftot) {
						recv_file=0;
						snprintf(tmp,512,"%s %ld %ld",rfname,(long)recvf.ftot,rmtime);
						if(rxclose(&rxfd,FOP_OK)!=FOP_OK) {
							msgs(BPM_SKIP,tmp);mes++;
							sline("binkp: file close error");
							write_log("can't close %s, suspended.",recvf.fname);
						} else {
							msgs(BPM_GOT,tmp);mes++;
						}
						qpfrecv();
					}
				}
			} else DEBUG(('B',1,"ignore received data block"));
			break;
		    case BPM_FILE:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			t1=t_set(BP_TIMEOUT);mes++;
			if(sent_eob&&recv_eob)sent_eob=0;
			cls=1;recv_eob=0;
			if(recv_file) {
				rxclose(&rxfd,FOP_OK);
				recv_file=0;qpfrecv();
			}
			if(f_pars(tmp,&fname,&fsize,&ftime,&foffs)) {
				msgs(BPM_ERR,"FILE: unparsable arguments");
				write_log("unparsable file info: \"%s\"",tmp);
				if(send_file)txclose(&txfd,FOP_ERROR);
				rc=S_FAILURE;goto failed;
			}
			if(rfname&&!strncasecmp(fname,rfname,MAX_PATH) &&
			    rmtime==ftime&&recvf.ftot==fsize&&recvf.soff==foffs) {
				recv_file=1;rxpos=foffs;
				break;
			}
			xfree(rfname);rmtime=ftime;
			rfname=xstrdup(basename(fname));
			for(i=0,n=0;i<strlen(fname);) {
				if(fname[i]=='\\'&&tolower(fname[i+1])=='x') {
					fname[n++]=hexdcd(fname[i+2],fname[i+3]);
					i+=4;
				} else fname[n++]=fname[i++];
			}
			switch(rxopen(fname,ftime,fsize,&rxfd)) {
			    case FOP_ERROR:
				write_log("binkp: error open for write file \"%s\"",recvf.fname);
			    case FOP_SUSPEND:
				DEBUG(('B',2,"trying to suspend file \"%s\"",recvf.fname));
				msgs(BPM_SKIP,"%s %ld %ld",rfname,(long)recvf.ftot,rmtime);
				mes++;
				break;
			    case FOP_SKIP:
				DEBUG(('B',2,"trying to skip file \"%s\"",recvf.fname));
				msgs(BPM_GOT,"%s %ld %ld",rfname,(long)recvf.ftot,rmtime);
				mes++;
				break;
			    case FOP_OK:
				if(foffs!=-1) {
					if(!(opt_nr&O_THEY))opt_nr|=O_THEY;
					recv_file=1;
					rxpos=0;
					break;
				}
			    case FOP_CONT:
				DEBUG(('B',2,"trying to get file \"%s\" from offset %d",recvf.fname,recvf.soff));
				msgs(BPM_GET,"%s %ld %ld %ld",rfname,(long)recvf.ftot,rmtime,(long)recvf.soff);
				mes++;
				break;
			}
			break;
		    case BPM_EOB:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			DEBUG(('B',4,"mes=%d, sent_eob=%d, recv_eob=%d",mes,sent_eob,recv_eob));
			t1=t_set(BP_TIMEOUT);
			if(chattimer&&sent_eob==2&&recv_eob==2) {
				opt_cht=O_NO;
				break;
			}
			if(recv_file) {
				rxclose(&rxfd,FOP_OK);
				recv_file=0;qpfrecv();
			}
			recv_eob++;
			if(!lst&&nofiles) {
				for(lst=fl;lst&&!lst->sendas;lst=lst->next);
				if(lst){lst=fl;mes=0;nofiles=0;break;}
			}
			if(opt_mb!=O_YES||(opt_mb==O_YES&&!cls&&!mes&&sent_eob)) {
				recv_eob++;
				if(sent_eob==1)sent_eob=2;
			} else if(recv_eob>1&&mes)recv_eob=1;
			if(recv_eob&&sent_eob==1) {
				msgs(BPM_EOB,NULL);
				sent_eob=2;
			}
			DEBUG(('B',4,"cls=%d, sent_eob=%d, recv_eob=%d",cls,sent_eob,recv_eob));
			mes=0;
			break;
		    case BPM_GOT:
		    case BPM_SKIP:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			t1=t_set(BP_TIMEOUT);
			if(!f_pars(tmp,&fname,&fsize,&ftime,NULL)) {
				if(send_file&&sendf.fname&&!strncasecmp(fname,sendf.fname,MAX_PATH) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize) {
					DEBUG(('B',1,"file %s %s",sendf.fname,(rc==BPM_GOT)?"skipped":"suspended"));
					txclose(&txfd,(rc==BPM_GOT)?FOP_SKIP:FOP_SUSPEND);
					if(rc==BPM_GOT)flexecute(lst);
					ticskip=(rc==BPM_GOT)?1:2;
					send_file=0;qpfsend();
					break;
				}
				if(wait_got&&sendf.fname&&!strncasecmp(fname,sendf.fname,MAX_PATH) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize) {
					wait_got=0;
					DEBUG(('B',1,"file %s %s",sendf.fname,(rc==BPM_GOT)?"done":"suspended"));
					txclose(&txfd,(rc==BPM_GOT)?FOP_OK:FOP_SUSPEND);
					if(rc==BPM_GOT)flexecute(lst);
					ticskip=(rc==BPM_GOT)?0:2;
					qpfsend();
				} else write_log("got M_%s for unknown file",mess[rc]);
			} else DEBUG(('B',1,"unparsable fileinfo"));
			break;
		    case BPM_ERR:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			write_log("BinkP error: \"%s\"",tmp);
		    case ERROR:
			DEBUG(('B',1,"got: ERROR, connect aborted"));
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			rc=S_REDIAL|S_ADDTRY;goto failed;
		    case BPM_BSY:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			write_log("BinkP busy: \"%s\"",tmp);
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			rc=S_REDIAL|S_ADDTRY;goto failed;
		    case BPM_GET:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			t1=t_set(BP_TIMEOUT);
			if(!f_pars(tmp,&fname,&fsize,&ftime,&foffs)) {
				if(send_file&&sendf.fname&&!strncasecmp(fname,sendf.fname,MAX_PATH) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize) {
					if(fseek(txfd,foffs,SEEK_SET)<0) {
						write_log("can't send file from requested offset");
						msgs(BPM_ERR,"can't send file from requested offset");
						txclose(&txfd,FOP_ERROR);
						send_file=0;
					} else {
						sendf.soff=sendf.foff=txpos=foffs;
						msgs(BPM_FILE,"%s %ld %ld %ld",sendf.fname,(long)sendf.ftot,sendf.mtime,(long)foffs);
					}
				} else write_log("got M_%s for unknown file",mess[rc]);
			} else DEBUG(('B',1,"unparsable fileinfo"));
			break;
		    case BPM_CHAT:
			DEBUG(('B',3,"got: M_%s",mess[rc]));
			t1=t_set(BP_TIMEOUT);
			if(opt_cht==O_YES)bink_devrecv(tmp);
			    else DEBUG(('B',1,"got chat msg with disabled chat"));
			break;
		    default:
			DEBUG(('B',1,"got unknown msg %d",rc));
		}
/*	???	if(!txfd&&!recv_file&&nofiles&&sent_eob==1) {
			msgs(BPM_EOB,NULL);
			sent_eob=2;
		}*/
		check_cps();
	}
	if(chattimer&&sent_eob==2&&recv_eob==2) {
		DEBUG(('S',4,"binkp chat autoclosed (%s)",t_exp(chattimer)?"timeout":"hangup"));
		msgs(BPM_EOB,NULL);
	} else if(t_exp(t1)) {
		DEBUG(('B',1,"BinkP session timeout (%d)",BP_TIMEOUT));
		msgs(BPM_ERR,"Session timeout");
		if(send_file)txclose(&txfd,FOP_ERROR);
		if(recv_file)rxclose(&rxfd,FOP_ERROR);
		rc=S_REDIAL;goto failed;
	}
	rc=S_OK;xfree(rfname);
	DEBUG(('B',1,"session done"));
failed:	flkill(&fl,rc==S_OK);
	xfree(rxbuf);
	xfree(txbuf);
	return rc;
}

#endif
