/******************************************************************
 * BinkP protocol implementation. by sisoft\\trg'2003.
 * $Id: binkp.c,v 1.5 2003/09/14 16:45:20 sisoft Exp $
 ******************************************************************/
#include "headers.h"
#include "defs.h"
#include "binkp.h"
#include "byteop.h"

static int bp_opt,bp_crypt;
char *hexdigits="0123456789abcdef";
static unsigned long key_in[3],key_out[3];
static char *mess[]={"NUL","ADR","PWD","FILE",
		"OK","EOB","GOT","ERR","BSY","GET","SKIP"};

static int datas(byte *buf,word len)
{
	DEBUG(('B',5,"datas block len=%u",len));
	if(bp_crypt)encrypt_buf((char*)buf,len,key_out);
	return(PUTBLK(buf,len));
}

static int msgs(int msg,char *t1,char *t2)
{
	unsigned len=1;
	DEBUG(('B',3,"msgs M_%s '%s%s'",mess[msg],t1?t1:"",t2?t2:""));
	if(t1)len+=strlen(t1);
	if(t2)len+=strlen(t2);
	if(len>0x7fff)len=0x7fff;
	*txbuf=((len>>8)&0x7f)|0x80;
	txbuf[1]=len&0xff;txbuf[2]=msg;
	snprintf((char*)(txbuf+3),len,"%s%s",t1?t1:"",t2?t2:"");
	return(datas(txbuf,(word)(len+2)));
}

static int msgr(char *buf)
{
	int rc=BPM_NONE,d;
	register int c;
	unsigned len=0,i;
	*buf=0;d=2;
	if((c=GETCHAR(0))>=0) {
		if(bp_crypt)update_keys(key_in,c^=decrypt_byte(key_in));
		if(c&0x80)d=0; else d=1;
		c&=0x7f;len=c<<8;
		while((c=GETCHAR(10))==TIMEOUT);
		if(c<0)return c;
		if(bp_crypt)update_keys(key_in,c^=decrypt_byte(key_in));
		len+=c&0xff;
		for(i=0;i<len;i++) {
			while((c=GETCHAR(10))==TIMEOUT);
			if(c<0)return c;
			if(bp_crypt)update_keys(key_in,c^=decrypt_byte(key_in));
			rxbuf[i]=c;
		}
		if(len>0x7fff)len=0x7fff;
		rc=*rxbuf;rxbuf[len]=0;
		if(!d)DEBUG(('B',3,"msgr M_%s '%s'",mess[rc],rxbuf+1));
		    else DEBUG(('B',3,"msgr data, len=%d",len));
		if(len&&!d)memcpy(buf,rxbuf+1,len);
		    else *(long*)buf=(long)len;
	}
	if(c<0&&c!=OK&&c!=TIMEOUT)return c;
	return((d==1)?BPM_DATA:((!d)?rc:BPM_NONE));
}

static void str_bin2hex(char *string,const unsigned char *binptr,int binlen)
{
	int i;
	for(i=0;i<binlen;i++) {
		*string++=hexdigits[(*binptr>>4)&0x0f];
		*string++=hexdigits[(*binptr)&0x0f];
		++binptr;
	}
	*string=0;
}

static int str_hex2bin(unsigned char *binptr,const char *string)
{
	int i,val,len=strlen(string);
	unsigned char *dest=binptr;
	const char *p;
	for(i=0;2*i<len;i++) {
		if((p=strchr(hexdigits,tolower(*(string++))))) {
			val=(int)(p-hexdigits);
			if((p=strchr(hexdigits,tolower(*(string++))))) {
				val=val*16+(int)(p-hexdigits);
				*dest++=(unsigned char)(val&0xff);
			} else return 0;
		} else return 0;
	}
	return(dest-binptr);
}

static int f_pars(char *s,char **fn,size_t *sz,time_t *tm,size_t *offs)
{
	char *n=s,*name,*size,*time,*off=NULL;
	DEBUG(('B',3,"pars: in '%s'",n));
	name=strsep(&n," ");
	size=strsep(&n," ");
	time=strsep(&n," ");
	if(offs)off=strsep(&n," ");
	if(name&&size&&time&&(!offs||off)) {
		*fn=name;
		*sz=atol(size);
		*tm=atol(time);
		if(offs)*offs=atol(off);
		DEBUG(('B',3,"pars: name=%s, size=%ld, time=%ld",*fn,*sz,*tm));
		return 0;
	}
	return 1;
}

int binkpsession(int mode,ftnaddr_t *remaddr)
{
	int rc,n=0,chal_len=0;
	int nofiles=0,wait_got=0;
	int sent_eob=0,recv_eob=0;
	int send_file=0,recv_file=0;
	char tmp[1024],*p,*fname;
	size_t fsize,foffs;
	falist_t *pp=NULL;
	qitem_t *q=NULL;
	flist_t *lst=NULL;
	unsigned char chal[64];
	ftnaddr_t *ba=NULL,fa;
	struct tm *tt;
	time_t ti,t1,ftime;
	totaln=0;totalf=0;totalm=0;got_req=0;
	bp_opt=BP_OPTIONS;rxstate=1;was_req=0;bp_crypt=0;
	write_log("starting %sbound BinkP session",mode?"out":"in");
	txbuf=(byte*)xcalloc(32770,1);
	rxbuf=(byte*)xcalloc(32770,1);
	fname=(char*)xcalloc(64,1);
	txstate=mode?BPO_Init:BPI_Init;
	if(mode)q=q_find(remaddr);
	if(q){totalm=q->pkts;totalf=q_sum(q)+q->reqs;}
	if(!mode&&bp_opt&BP_OPT_MD5) {
		long rnd=(long)random(),utm=time(NULL);
		int pid=((int)getpid())^((int)random());
		bp_opt|=BP_OPT_MD5;
		*chal=(unsigned char)rnd;
		chal[1]=(unsigned char)(rnd>>8);
		chal[2]=(unsigned char)(rnd>>16);
		chal[3]=(unsigned char)(rnd>>24);
		chal[4]=(unsigned char)(pid);
		chal[5]=(unsigned char)(pid>>8);
		chal[6]=(unsigned char)(utm);
		chal[7]=(unsigned char)(utm>>8);
		chal[8]=(unsigned char)(utm>>16);
		chal[9]=(unsigned char)(utm>>24);
		chal_len=10;
	}
	sline("BinkP handshake");
	t1=t_set(cfgi(CFG_HSTIMEOUT));
	while(rxstate&&!t_exp(t1)) {
		switch(txstate) {
		    case BPO_Init:
		    case BPI_Init:
			DEBUG(('B',3,"state: init (%d)",txstate));
			if(!mode&&chal_len>0) {
				char chall[128];
				str_bin2hex(chall,chal,chal_len);
				msgs(BPM_NUL,"OPT CRAM-MD5-",chall);
			}
			msgs(BPM_NUL,"SYS ",cfgs(CFG_STATION));
			msgs(BPM_NUL,"ZYZ ",cfgs(CFG_SYSOP));
			msgs(BPM_NUL,"LOC ",cfgs(CFG_PLACE));
			snprintf(tmp,120,"%d,%s",cfgi(CFG_SPEED),cfgs(CFG_FLAGS));
			msgs(BPM_NUL,"NDL ",tmp);
			msgs(BPM_NUL,"PHN ",cfgs(CFG_PHONE));
			ti=time(NULL);tt=gmtime(&ti);
			strftime(tmp,120,"%a, %d %b %Y %H:%M:%S GMT",tt);
			msgs(BPM_NUL,"TIME ",tmp);
			snprintf(tmp,128,"%s-%s/%s",qver(0),qver(1),qver(2));
			strtr(tmp,' ','_');xstrcat(tmp," " BP_VERSION,128);
			msgs(BPM_NUL,"VER ",tmp);
			snprintf(tmp,128,"%s%s%s%s%s",
				(bp_opt&BP_OPT_NR)?" NR":"",
				(bp_opt&BP_OPT_ND)?" ND":"",
				(bp_opt&BP_OPT_MB)?" MB":"",
				(bp_opt&BP_OPT_MPWD)?" MPWD":"",
				(bp_opt&BP_OPT_CRYPT&&bp_opt&BP_OPT_MD5)?" CRYPT":"");
			if(!mode&&strlen(tmp))msgs(BPM_NUL,"OPT",tmp);
			pp=cfgal(CFG_ADDRESS);
			if(mode) {
				ba=akamatch(remaddr,pp);
				xstrcpy(tmp,ftnaddrtoa(ba),1023);
			    } else {
				xstrcpy(tmp,ftnaddrtoa(&pp->addr),1023);
				pp=pp->next;ba=NULL;
			}
			xstrcat(tmp,"@",1023);
			xstrcat(tmp,cfgs(CFG_DOMAIN)?(ccs+(*ccs=='@')):"fidonet",1023);
			for(;pp;pp=pp->next)
			    if(&pp->addr!=ba) {
				xstrcat(tmp," ",1023);
				xstrcat(tmp,ftnaddrtoa(&pp->addr),1023);
				xstrcat(tmp,"@",1023);
				xstrcat(tmp,cfgs(CFG_DOMAIN)?(ccs+(*ccs=='@')):"fidonet",1023);
			}
			msgs(BPM_ADR,tmp,NULL);
			txstate=(txstate==BPO_Init)?BPO_WaitNul:BPI_WaitAdr;
			break;
		    case BPO_SendPwd:
			DEBUG(('B',3,"state: sendpwd (%d)",txstate));
			p=findpwd(remaddr);
			if(!p)msgs(BPM_PWD,"-",NULL);
			    else {
				if(bp_opt&BP_OPT_MD5) {
					char dig_h[33];
					unsigned char dig_b[16];
					md5_cram_get((unsigned char*)p,chal,chal_len,dig_b);
					str_bin2hex(dig_h,dig_b,16);
					msgs(BPM_PWD,"CRAM-MD5-",dig_h);
				} else msgs(BPM_PWD,p,NULL);
			}
			txstate=BPO_WaitAdr;
			break;
		    case BPO_Auth:
		    case BPI_Auth:
			rc=0;
			DEBUG(('B',3,"state: auth (%d)",txstate));
			title("%sbound session %s",mode?"Out":"In",ftnaddrtoa(&rnode->addrs->addr));
			if(is_bso()==1)for(pp=rnode->addrs;pp;pp=pp->next)
				rc+=bso_locknode(&pp->addr,LCK_s);
			if(is_aso()==1)for(pp=rnode->addrs;pp;pp=pp->next)
				rc+=aso_locknode(&pp->addr,LCK_s);
			if(!rc) {
				write_log("can't lock outbound for %s!",ftnaddrtoa(remaddr));
				msgs(BPM_BSY,"All addresses are busy",NULL);
				flkill(&fl,0);
				return S_REDIAL;
			}
			if(mode) {
				if(!has_addr(remaddr,rnode->addrs)) {
					write_log("remote isn't %s!",ftnaddrtoa(remaddr));
					msgs(BPM_ERR,"Sorry, you are not who I need",NULL);
					flkill(&fl,0);
					return S_FAILURE;
				}
				flkill(&fl,0);totalf=0;totalm=0;
				for(pp=rnode->addrs;pp;pp=pp->next) {
					makeflist(&fl,&pp->addr,mode);
					if(findpwd(&pp->addr))rnode->options|=O_PWD;
				}
				if(is_listed(rnode->addrs,cfgs(CFG_NLPATH),cfgi(CFG_NEEDALLLISTED)))
					rnode->options|=O_LST;
			} else {
				for(pp=cfgal(CFG_ADDRESS);pp;pp=pp->next)
					if(has_addr(&pp->addr,rnode->addrs)) {
						write_log("remote also has %s!",ftnaddrtoa(&pp->addr));
						msgs(BPM_ERR,"Sorry, you also has one of my aka's",NULL);
						flkill(&fl,0);
						return S_FAILURE;
					}
				if(is_listed(rnode->addrs,cfgs(CFG_NLPATH),cfgi(CFG_NEEDALLLISTED)))
					rnode->options|=O_LST;
				rc=0;
				if(chal_len>0&&!strncmp(rnode->pwd,"CRAM-MD5-",9))rc=9;
				for(pp=rnode->addrs;pp;pp=pp->next) {
					p=findpwd(&pp->addr);n=0;
					if(!p||(!rc&&!strcasecmp(rnode->pwd,p)))n=1;
					    else if(p&&rc) {
						char dig_h[33];
						unsigned char dig_b[16];
						md5_cram_get((unsigned char*)p,chal,chal_len,dig_b);
						str_bin2hex(dig_h,dig_b,16);
						if(!strcasecmp(rnode->pwd+rc,dig_h))n=1;
					}
					if(n) {
						makeflist(&fl,&pp->addr,mode);
						if(p)rnode->options|=O_PWD;
					} else {
						write_log("password not matched for %s",ftnaddrtoa(&pp->addr));
						msgs(BPM_ERR,"Security violation",NULL);
						rnode->options|=O_BAD;
						flkill(&fl,0);
						return S_FAILURE;
					}
				}
			}
			if(mode) {
				bp_opt&=BP_OPTIONS;
				snprintf(tmp,128,"%s%s%s%s%s",
					(bp_opt&BP_OPT_NR)?" NR":"",
					(bp_opt&BP_OPT_ND)?" ND":"",
					(bp_opt&BP_OPT_MB)?" MB":"",
					(bp_opt&BP_OPT_MPWD)?" MPWD":"",
					(bp_opt&BP_OPT_CRYPT&&bp_opt&BP_OPT_MD5)?" CRYPT":"");
				if(strlen(tmp))msgs(BPM_NUL,"OPT",tmp);
			}
			if(totalm||totalf) {
				snprintf(tmp,120,"%lu %lu",totalm,totalf);
				msgs(BPM_NUL,"TRF ",tmp);
			}
			if(txstate==BPI_Auth) {
				msgs(BPM_OK,(rnode->options&O_PWD)?"secure":"non-secure",NULL);
				rxstate=0;
				break;
			}
			txstate=BPO_WaitOk;
			break;
		}
		rc=msgr(tmp);
		if(rc==RCDO||tty_hangedup) {
			flkill(&fl,0);
			return S_REDIAL;
		}
		switch(rc) {
		    case BPM_NONE: case TIMEOUT: case BPM_DATA:
			break;
		    case BPM_NUL:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			if(txstate==BPO_WaitNul||txstate==BPO_WaitAdr||txstate==BPI_WaitAdr||txstate==BPO_WaitOk) {
				char *n;
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
						if(!strcmp(p,"NR"))bp_opt|=BP_OPT_NR;
						else if(!strcmp(p,"MB"))bp_opt|=BP_OPT_MB;
						else if(!strcmp(p,"ND"))bp_opt|=BP_OPT_ND;
						else if(!strcmp(p,"MPWD"))bp_opt|=BP_OPT_MPWD;
						else if(!strcmp(p,"CRYPT"))bp_opt|=BP_OPT_CRYPT;
						else if(!strncmp(p,"CRAM-",5)) {
							char *hash_t=p+5,*chall;
							chall=strchr(hash_t,'-');
							if(chall) {
								char *o;
								*chall++=0;
								while((o=strsep(&hash_t,"/"))) {
									if(!strcmp(o,"SHA1"))bp_opt|=BP_OPT_SHA1;
									if(!strcmp(o,"MD5"))bp_opt|=BP_OPT_MD5;
									if(!strcmp(o,"DES"))bp_opt|=BP_OPT_DES;
								}
								if(strlen(chall)>(2*sizeof(chal)))write_log("binkp got too long challenge string");
								    else chal_len=str_hex2bin(chal,chall);
							} else write_log("binkp got invalid option \"%s\"",p);
						}
					}
					bp_opt&=BP_OPTIONS;
				} else if(!strncmp(tmp,"VER ",4))restrcpy(&rnode->mailer,tmp+4);
				    else write_log("BinkP: got invalid NUL: \"%s\"",tmp);
				if(txstate==BPO_WaitNul)txstate=BPO_SendPwd;
			}
			break;
		    case BPM_ADR:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			if(txstate==BPO_WaitAdr||txstate==BPI_WaitAdr) {
				char *n=tmp;
				falist_kill(&rnode->addrs);
				while((p=strsep(&n," ")))
				    if(parseftnaddr(p,&fa,NULL,0))
					if(!falist_find(rnode->addrs,&fa))
					    falist_add(&rnode->addrs,&fa);
				txstate=(txstate==BPO_WaitAdr)?BPO_Auth:BPI_WaitPwd;
			}
			break;
		    case BPM_PWD:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			if(txstate==BPI_WaitPwd) {
				restrcpy(&rnode->pwd,tmp);
				txstate=BPI_Auth;
			}
			break;
		    case BPM_OK:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			if(txstate==BPO_WaitOk)rxstate=0;
			break;
		    case BPM_ERR:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			write_log("BinkP error: \"%s\"",tmp);
		    case ERROR:
			flkill(&fl,0);
			return S_FAILURE|S_ADDTRY;
		    case BPM_BSY:
			DEBUG(('B',4,"got: M_%s, state=%d",mess[rc],txstate));
			write_log("BinkP busy: \"%s\"",tmp);
			flkill(&fl,0);
			return S_REDIAL;
		    default:
			DEBUG(('B',4,"got: unknown msg %d, state=%d",rc,txstate));
		}
	}
	if(t_exp(t1)) {
		write_log("BinkP handshake timeout");
		msgs(BPM_ERR,"Handshake timeout",NULL);
		flkill(&fl,0);
		return S_REDIAL;
	}
	if(!(rnode->options&O_PWD)||!(bp_opt&BP_OPT_MD5))bp_opt&=BP_OPT_CRYPT;
	if(bp_opt&BP_OPT_CRYPT) {
		bp_crypt=1;
		if(mode) { // bogus, as all other crypt code
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
	write_log("options: BinkP%s%s%s%s%s%s%s%s",
		(rnode->options&O_LST)?"/LST":"",
		(rnode->options&O_PWD)?"/PWD":"",
		(bp_opt&BP_OPT_NR)?"/NR":"",
		(bp_opt&BP_OPT_ND)?"/ND":"",
		(bp_opt&BP_OPT_MB)?"/MB":"",
		(bp_opt&BP_OPT_MPWD)?"/MPWD":"",
		(bp_opt&BP_OPT_MD5)?"/MD5":"/Plain",
		(bp_opt&BP_OPT_CRYPT)?"/CRYPT":"");
	sendf.allf=totaln;sendf.ttot=totalf+totalm;
	recvf.ttot=rnode->netmail+rnode->files;
	effbaud=rnode->speed;lst=fl;
	sline("BinkP session");
	while(!sent_eob||!recv_eob) {
		if(!send_file&&!sent_eob&&!txfd) {
			DEBUG(('B',4,"find files"));
			if(lst&&lst!=fl)lst=lst->next;
			while(lst&&!txfd) {
				if(!lst->sendas||!(txfd=txopen(lst->tosend,lst->sendas))) {
					flexecute(lst);
					lst=lst->next;
				}
			}
			if(lst&&txfd) {
				DEBUG(('B',4,"found: %s",sendf.fname));
				send_file=1;txpos=0;
				snprintf(tmp,512,"%s %ld %ld 0",sendf.fname,(long)sendf.ftot,sendf.mtime);
				msgs(BPM_FILE,tmp,NULL);
			} else nofiles=1;
		}
		if(send_file&&txfd) {
			if((n=fread(txbuf+2,1,BP_BLKSIZE,txfd))<0) {
				sline("binkp: file read error");
				DEBUG(('B',1,"binkp: file read error"));
				txclose(&txfd,FOP_ERROR);
				send_file=0;
			} else {
				DEBUG(('B',3,"readed %d bytes of %d (D)",n,BP_BLKSIZE));
				if(n) {
					*txbuf=((n>>8)&0x7f);txbuf[1]=n&0xff;
					datas(txbuf,(word)(n+2));
					txpos=n+2;sendf.foff+=n;
					qpfsend();
				}
				if(n<BP_BLKSIZE) {
					wait_got=1;
					send_file=0;
				}
			}
		}	
		if(nofiles&&!wait_got&&!sent_eob) {
			sent_eob=1;
			msgs(BPM_EOB,NULL,NULL);
		}
		rc=msgr(tmp);
		if(rc==RCDO||tty_hangedup) {
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			flkill(&fl,0);
			return S_REDIAL;
		}
		switch(rc) {
		    case BPM_NONE: case TIMEOUT:
			break;
		    case BPM_NUL: {
			char *n;
			DEBUG(('B',4,"got: M_%s (%s)",mess[rc],tmp));
			if(!strncmp(tmp,"OPT ",4)) {
				n=tmp+4;
				while((p=strsep(&n," "))) {
					if(!strcmp(p,"NR"))bp_opt|=BP_OPT_NR;
					else if(!strcmp(p,"MB"))bp_opt|=BP_OPT_MB;
					else if(!strcmp(p,"ND"))bp_opt|=BP_OPT_ND;
					else if(!strcmp(p,"MPWD"))bp_opt|=BP_OPT_MPWD;
					else if(!strcmp(p,"CRYPT"))bp_opt|=BP_OPT_CRYPT;
				}
				bp_opt&=BP_OPTIONS;
			} else if(!strncmp(tmp,"TRF ",4)) {
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
			DEBUG(('B',4,"got: data"));
			if(recv_file) {
				if((n=fwrite(rxbuf,1,*(long*)tmp,rxfd))<0) {
					recv_file=0;
					sline("binkp: file write error");
					write_log("can't write %s, suspended.",recvf.fname);
					snprintf(tmp,512,"%s %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime);
					msgs(BPM_SKIP,tmp,NULL);
					rxclose(&rxfd,FOP_ERROR);
				} else {
					rxpos+=*(long*)tmp;
					recvf.foff+=*(long*)tmp;
					qpfrecv();
					if(rxpos>recvf.ftot) {
						recv_file=0;
						write_log("binkp: got too many data (%ld, %ld expected)",rxpos,(long)recvf.ftot);
						rxclose(&rxfd,FOP_SUSPEND);
						snprintf(tmp,512,"%s %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime);
						msgs(BPM_SKIP,tmp,NULL);
					} else if(rxpos==recvf.ftot) {
						recv_file=0;
						snprintf(tmp,512,"%s %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime);
						if(rxclose(&rxfd,FOP_OK)!=FOP_OK) {
							msgs(BPM_SKIP,tmp,NULL);
							sline("binkp: file close error");
							write_log("can't close %s, suspended.",recvf.fname);
						} else msgs(BPM_GOT,tmp,NULL);
					}
				}
			} else DEBUG(('B',1,"ignore received data block"));
			break;
		    case BPM_FILE:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			if(recv_file) {
				rxclose(&rxfd,FOP_OK);
				recv_file=0;
			}
			if(f_pars(tmp,&fname,&fsize,&ftime,&foffs)) {
				msgs(BPM_ERR,"FILE: ","unparsable arguments");
				write_log("unparsable file info: \"%s\"",tmp);
				if(send_file)txclose(&txfd,FOP_ERROR);
				flkill(&fl,0);
				return S_FAILURE;
			}
			if(recvf.fname&&!strncasecmp(fname,recvf.fname,64) &&
			    recvf.mtime==ftime&&recvf.ftot==fsize&&recvf.soff==foffs) {
				recv_file=1;rxpos=foffs;
				break;
			}
//mb?			if((bp_opt&BP_OPT_MB)&&recv_eob&&!sent_eob)recv_eob=0;
			switch(rxopen(fname,ftime,fsize,&rxfd)) {
			    case FOP_OK:
				recv_file=1;rxpos=0;
				break;
			    case FOP_ERROR:
				write_log("binkp: error open for write \"%s\"",recvf.fname);
			    case FOP_SUSPEND:
				snprintf(tmp,512,"%s %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime);
				msgs(BPM_SKIP,tmp,NULL);
				break;
			    case FOP_SKIP:
				snprintf(tmp,512,"%s %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime);
				msgs(BPM_GOT,tmp,NULL);
				break;
			    case FOP_CONT:
				snprintf(tmp,512,"%s %ld %ld %ld",recvf.fname,(long)recvf.ftot,recvf.mtime,(long)recvf.soff);
				/* ???? work? */
				msgs(BPM_GET,tmp,NULL);
				break;
			}
			break;
		    case BPM_EOB:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			if(recv_file) {
				rxclose(&rxfd,FOP_OK);
				recv_file=0;
			}
			recv_eob=1;
			break;
		    case BPM_GOT:
		    case BPM_SKIP:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			if(!f_pars(tmp,&fname,&fsize,&ftime,NULL)) {
				if(send_file&&sendf.fname&&!strncasecmp(fname,sendf.fname,64) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize&&sendf.soff==foffs) {
					DEBUG(('B',2,"file %s %s",sendf.fname,(rc==BPM_GOT)?"skipped":"suspended"));
					txclose(&txfd,(rc==BPM_GOT)?FOP_SKIP:FOP_SUSPEND);
					if(rc==BPM_GOT)flexecute(lst);
					send_file=0;
					break;
				}
				if(wait_got&&sendf.fname&&!strncasecmp(fname,sendf.fname,64) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize&&sendf.soff==foffs) {
					wait_got=0;
					DEBUG(('B',2,"file %s %s",sendf.fname,(rc==BPM_GOT)?"done":"suspended"));
					txclose(&txfd,(rc==BPM_GOT)?FOP_OK:FOP_SUSPEND);
					if(rc==BPM_GOT)flexecute(lst);
				} else write_log("got M_%s for unknown file",mess[rc]);
			} else DEBUG(('B',1,"unparsable fileinfo"));
			break;
		    case BPM_ERR:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			write_log("BinkP error: \"%s\"",tmp);
		    case ERROR:
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			flkill(&fl,0);
			return S_REDIAL|S_ADDTRY;
		    case BPM_BSY:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			write_log("BinkP busy: \"%s\"",tmp);
			if(send_file)txclose(&txfd,FOP_ERROR);
			if(recv_file)rxclose(&rxfd,FOP_ERROR);
			flkill(&fl,0);
			return S_REDIAL|S_ADDTRY;
		    case BPM_GET:
			DEBUG(('B',4,"got: M_%s",mess[rc]));
			if(!f_pars(tmp,&fname,&fsize,&ftime,&foffs)) {
				if(send_file&&sendf.fname&&!strncasecmp(fname,sendf.fname,64) &&
				    sendf.mtime==ftime&&sendf.ftot==fsize&&sendf.soff==foffs) {
					if(fseek(txfd,foffs,SEEK_SET)<0) {
						write_log("can't send file from requested offset");
						msgs(BPM_ERR,"can't send file from requested offset",NULL);
						txclose(&txfd,FOP_ERROR);
						send_file=0;
					} else {
						sendf.soff=sendf.foff=txpos=foffs;
						snprintf(tmp,512,"%s %ld %ld %ld",sendf.fname,(long)sendf.ftot,sendf.mtime,(long)foffs);
						msgs(BPM_FILE,tmp,NULL);
					}
				} else write_log("got M_%s for unknown file",mess[rc]);
			} else DEBUG(('B',1,"unparsable fileinfo"));
			break;
		    default:
			DEBUG(('B',1,"got unknown msg %d",rc));
		}
		check_cps();
	}
	msgs(BPM_EOB,NULL,NULL);
	DEBUG(('B',2,"session done"));
	flkill(&fl,1);
	xfree(rxbuf);
	xfree(txbuf);
	return S_OK;
}
