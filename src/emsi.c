/**********************************************************
 * EMSI
 * $Id: emsi.c,v 1.1.1.1 2003/07/12 21:26:32 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "defs.h"

#define EMSI_BUF 65536
	
char *emsireq="**EMSI_REQA77E";
char *emsiack="**EMSI_ACKA490";
char *emsiinq="**EMSI_INQC816";
char *emsinak="**EMSI_NAKEEC3";
char *emsidat="**EMSI_DAT";

int chat_need=-1,chatmy=-1;

char *emsi_makedat(ftnaddr_t *remaddr,unsigned long mail,unsigned long files,int lopt,
				    char *protos,falist_t *adrs,int showpwd)	
{
	char *dat=xcalloc(EMSI_BUF,1),tmp[1024],*p;
	int c;
	falist_t *cs;ftnaddr_t *ba;
	time_t tm=time(NULL);
	
	xstrcpy(dat,"**EMSI_DAT0000{EMSI}{",EMSI_BUF);
	ba=akamatch(remaddr,adrs?adrs:cfgal(CFG_ADDRESS));
	xstrcat(dat,ftnaddrtoa(ba),EMSI_BUF);
	for(cs=cfgal(CFG_ADDRESS);cs;cs=cs->next)
		if(&cs->addr!=ba) {
			xstrcat(dat," ",EMSI_BUF);
			xstrcat(dat,ftnaddrtoa(&cs->addr),EMSI_BUF);
		}
	xstrcat(dat,"}{",EMSI_BUF);
	if(showpwd) {
		p=findpwd(remaddr);
		if(p)xstrcat(dat,p,EMSI_BUF);
	}
	xstrcat(dat,"}{8N1",EMSI_BUF);
	if(strchr(protos,'H')||strchr(protos,'h')
#ifdef HYDRA8K16K
		||strchr(protos,'8')||strchr(protos,'6')
#endif/*HYDRA8K16K*/		
		)xstrcat(dat,",RH1",EMSI_BUF);
	if(lopt&O_PUA)xstrcat(dat,",PUA",EMSI_BUF);
	if(lopt&O_PUP)xstrcat(dat,",PUP",EMSI_BUF);
	if(lopt&O_HRQ)xstrcat(dat,",HRQ",EMSI_BUF);
	if(lopt&O_HXT)xstrcat(dat,",HXT",EMSI_BUF);
	if(lopt&O_HAT)xstrcat(dat,",HAT",EMSI_BUF);
	xstrcat(dat,"}{XMA",EMSI_BUF);
	if(lopt&O_NRQ)xstrcat(dat,",NRQ",EMSI_BUF);
	p=protos;c=0;chatmy=0;
	if(!(lopt&P_NCP))while(*p) {
		switch(toupper(*p++)) {
		case '1':xstrcat(dat,",ZMO",EMSI_BUF);c=1;break;
		case 'Z':xstrcat(dat,",ZAP",EMSI_BUF);c=1;break;
		case 'D':xstrcat(dat,",DZA",EMSI_BUF);c=1;break;
		case 'H':xstrcat(dat,",HYD",EMSI_BUF);c=1;break;
#ifdef HYDRA8K16K		
		case '8':xstrcat(dat,",HY8",EMSI_BUF);c=1;break;
		case '6':xstrcat(dat,",H16",EMSI_BUF);c=1;break;
#endif/*HYDRA8K16K*/	
		case 'J':xstrcat(dat,",JAN",EMSI_BUF);c=1;break;
		case 'C':xstrcat(dat,",CHT",EMSI_BUF);chatmy=1;break;
		}
	}
	if(!c)xstrcat(dat,",NCP",EMSI_BUF);
    	snprintf(tmp,1024,"}{FE}{%s}{%s}{%s}",strdup(qver(0)),strdup(qver(1)),strdup(qver(2)));
	xstrcat(dat,tmp,EMSI_BUF);
	snprintf(tmp,1024,"{IDENT}{[%s][%s][%s][%s][%d][%s]}{TRAF}{%lX %lX}{OHFR}{%s %s}{TRX#}{[%lX]}{TZUTC}{[%+03ld00]}",
			strip8(cfgs(CFG_STATION)),strip8(cfgs(CFG_PLACE)),
			strip8(cfgs(CFG_SYSOP)),strip8(cfgs(CFG_PHONE)),
			cfgi(CFG_SPEED),strip8(cfgs(CFG_FLAGS)),
			mail,files,strip8(cfgs(CFG_WORKTIME)?ccs:strdup("Never")),
			strip8(cfgs(CFG_EMSIFREQTIME)?ccs:(cfgs(CFG_FREQTIME)?ccs:strdup("Never"))),
			time(NULL)+gmtoff(tm,0),gmtoff(tm,0)/3600);
	xstrcat(dat,tmp,EMSI_BUF);
	snprintf(tmp,1024,"%04X",strlen(dat)-14);
	memcpy(dat+10,tmp,4);
	snprintf(tmp,1024,"%04X",crc16usds(dat+2));
	xstrcat(dat,tmp,EMSI_BUF);
	return dat;
}

char *emsi_tok(char **b, char *kc)
{
	char *p=*b,*t;
	int st=0;
	if(*p!=kc[0])return NULL;
	t=++p;
	while(st<2) {
		switch(st) {
			case 0:
				if(*t==kc[1])st=1;
				if(!*t)st=3;
				    else t++;
				break;
			case 1:
				if(*t!=kc[1]){st=2;t--;}
				    else {st=0;t++;}
				break;
		}
	}
	if(st==3)return NULL;
	*t++=0;
	*b=t;
	return p;
}


int hexdcd(char d,char c)
{
	c=tolower(c);d=tolower(d);
	if(c>='a'&&c<='f')c-=('a'-10);
	    else c-='0';
	if(d>='a'&&d<='f')d-=('a'-10);
	    else d-='0';
	return(c*16+d);
}

void emsi_dcds(char *s)
{
	unsigned char *d=(unsigned char*)s,t;
	while((t=*s)) {
		if(t=='}'||t==']')t=*++s;
		*d=tokoi((t=='\\')?(hexdcd(*++s,*++s)):((t<32)?'.':t));
		s++;d++;
	}
	*d=0;
}

int emsi_parsedat(char *str, ninfo_t *dat)
{
	char *p, *t, *s, *lcod, *ccod;
	int l;ftnaddr_t fa;
	FILE *elog;

	if(cfgs(CFG_EMSILOG)) {
		elog=fopen(ccs, "at");
		if(elog) {
			fputs(str, elog);fputc('\n', elog);fclose(elog);
		}
	}
/* 	*(str+strlen(str)-1)=0; */
	if(!(str=strstr(str, "**EMSI_DAT")))return 0;
	sscanf(str+10,"%04X",&l);
	if(l!=strlen(str)-18)return 0;
	sscanf(str+strlen(str)-4,"%04X",&l);
	if(l!=crc16usd(str+2,strlen(str)-6))return 0;
	if(strncmp(str+14,"{EMSI}",6))return 0;
	t=str+20;
	*str=1;
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	falist_kill(&dat->addrs);
	while((s=strsep(&p," ")))if(parseftnaddr(s,&fa,NULL,0))
		if(!falist_find(dat->addrs,&fa))falist_add(&dat->addrs,&fa);
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	restrcpy(&dat->pwd,p);
	emsi_dcds(dat->pwd);
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	lcod=p;
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	ccod=p;
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	restrcpy(&dat->mailer,p);
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	restrcat(&dat->mailer,"-");
	restrcat(&dat->mailer, p);
	p=emsi_tok(&t,"{}");
	if(!p)return 0;
	restrcat(&dat->mailer,"/");
	restrcat(&dat->mailer, p);
	emsi_dcds(dat->mailer);

	DEBUG(('E',1,"emsi codes %s/%s",lcod,ccod));

	dat->options|=emsi_parsecod(lcod, ccod);
	dat->chat=(chat_need==1&&chatmy!=0);

	xfree(dat->wtime);
	while((p=emsi_tok(&t,"{}"))) {
		if(!strcmp(p, "IDENT")) {
			p=emsi_tok(&t,"{}");
			if(!p)return 0;
			s=emsi_tok(&p,"[]");
			restrcpy(&dat->name,s);
			emsi_dcds(dat->name);
			s=emsi_tok(&p,"[]");
			restrcpy(&dat->place,s);
			emsi_dcds(dat->place);
			s=emsi_tok(&p,"[]");
			restrcpy(&dat->sysop, s);
			emsi_dcds(dat->sysop);
			s=emsi_tok(&p,"[]");
			restrcpy(&dat->phone,s);
			emsi_dcds(dat->phone);
 			s=emsi_tok(&p,"[]"); 
 			if(s) sscanf(s,"%d",&dat->speed); 
 			s=emsi_tok(&p,"[]"); 
 			restrcpy(&dat->flags,s); 
			emsi_dcds(dat->flags);
		} else if(!strcmp(p, "TRAF")) {
			p=emsi_tok(&t,"{}");
			if(!p)return 0;
			sscanf(p,"%x %x",&dat->netmail,&dat->files);	
		} else if(!strcmp(p,"OHFR")) {
			p=emsi_tok(&t,"{}");
			if(!p)return 0;
			restrcpy(&dat->wtime,p);
			emsi_dcds(dat->wtime);
		} else if(!strcmp(p,"MOH#")) {
			p=emsi_tok(&t,"{}");
			if(!p)return 0;
			s=emsi_tok(&p,"[]");
			if(sscanf(s,"%x",&dat->holded)!=1)return 0;
		} else if(!strcmp(p,"TRX#")) {
			p=emsi_tok(&t,"{}");
			if(!p)return 0;
			s=emsi_tok(&p,"[]");
			if(sscanf(s,"%lx",&dat->time)!=1)return 0;			
		} else p=emsi_tok(&t,"{}");
	}		
	return 1;
}


int emsi_send(int mode,unsigned char *dat)
{
	time_t t1,t2;
	int tries=0,got=0,ch;
	char str[MAX_STRING],*p=str;

	memset(str,0,MAX_STRING);
	t1=t_set(60);
	while(1) {
		tries++;
		sline("Sending EMSI_DAT");
		DEBUG(('E',1,"Sending EMSI_DAT (%d)",tries));
		PUTSTR(dat);PUTCHAR('\r');
		if(tries>10)return TIMEOUT;
		t2=t_set(20);got=0;p=str;
		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
			if(NOTTO(ch))return ch;
			if(t_exp(t1))return TIMEOUT;
			if(t_exp(t2))break;
			if(!got&&ch=='*')got=1;
			if(got&&(ch=='\r'||ch=='\n')) {
				*p=0;p=str;got=0;
            			DEBUG(('E',2,"Got str '%s' %d",str,strlen(str)));
				if(!strncmp(str,emsiack,14)) {
					sline("Got EMSI_ACK");
					DEBUG(('E',1,"Got EMSI_ACK"));
					return OK;
				}
				if(!strncmp(str, emsireq, 14)) {
					sline("Skipping EMSI_REQ");
					DEBUG(('E',1,"Skipping EMSI_REQ"));
					continue;
				}
				if(!strncmp(str,emsiack,7))break;
			}
			if(got)*p++=ch;
			if((p-str)>=MAX_STRING) {
				got=0;p=str;
			}
			
		}		
	}
}

int emsi_recv(int mode,ninfo_t *rememsi)
{
	int tries,got=0,ch,emsidatlen=0,emsidatgot=-1;
	time_t t1,t2;
	char str[EMSI_BUF],*p=str,*emsidathdr;
	t1=t_set(20);t2=t_set(60);tries=0;
	while(1) {
		tries++;
		if(tries>10)return TIMEOUT;
		if(!mode) {
			sline("Sending EMSI_REQ (%d)...",tries);
			DEBUG(('E',1,"Sending EMSI_REQ (%d)...",tries));
			PUTSTR((unsigned char*)emsireq);PUTCHAR('\r');
			t1=t_set(20);
		} else if(tries>1) {
			sline("Sending EMSI_NAK...");
			DEBUG(('E',1,"Sending EMSI_NAK (%d)...",tries));
			PUTSTR((unsigned char*)emsinak);PUTCHAR('\r');
			t1=t_set(20);
		}
		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
			if(NOTTO(ch))return ch;
			if(ch<0)break;
			if(!got&&ch=='*') got=1;
			if(got&&(ch=='\r'||ch=='\n'||(emsidatgot==emsidatlen))) {
				DEBUG(('E',2,"Got %d bytes of %d of EMSI_DAT",emsidatgot,emsidatlen));
				*p=0;p=str;got=0;
				emsidatgot=-1;emsidatlen=0;
				emsidathdr=NULL;
#ifdef NEED_DEBUG
				if(strstr(str,emsidat))DEBUG(('E',1,"EMSI_DAT at offset %d",strstr(str,emsidat)-str));
#endif
				DEBUG(('E',1,"got str '%s' %d",str,strlen(str)));

				if(!strncmp(str,emsidat,10)) {
					sline("Received EMSI_DAT");
					DEBUG(('E',1,"Received EMSI_DAT"));
					ch=emsi_parsedat(str, rememsi);
					DEBUG(('E',1,"Parser result: %d", ch));
					if(ch) {
						sline("Sending EMSI_ACK...");
						DEBUG(('E',1,"Sending EMSI_ACK"));
						PUTSTR((unsigned char*)emsiack);PUTCHAR('\r');
						PUTSTR((unsigned char*)emsiack);PUTCHAR('\r');
						return OK;
					} else break;
				}
			}
			if(got) *p++=ch;
			if((p-str)>=EMSI_BUF) {
				write_log("too long EMSI packet!");
				got=0;p=str;
			}
			if(emsidatlen) {
				emsidatgot++;
			} else if((emsidathdr=strstr(str, emsidat)) && (p-emsidathdr==14)) {
				*p = 0;
				sscanf(emsidathdr+10,"%04X",&emsidatlen);
				emsidatgot = 0;
				emsidatlen += 4; /* CRC on the ned of EMSI_DAT is 4 bytes long */
				DEBUG(('E',1,"Got start of EMSI_DAT, length is %d",emsidatlen));
			}
			if(t_exp(t2)) {
				sline("Timeout receiving EMSI_DAT");
				DEBUG(('E',1,"Timeout receiving EMSI_DAT"));
				return TIMEOUT;
			}
			if(t_exp(t1)) break;
		}
	}
}

int emsi_init(int mode)
{
	int ch, got=0;
	time_t t1, t2;
	char str[EMSI_BUF], *p=str;
	int tries = 0;

	if(mode) {
		t1=t_set(cfgi(CFG_HSTIMEOUT));
		PUTCHAR('\r');
		do { ch = HASDATA(t_rest(t1)); } while (!ISTO(ch) && !t_exp(t1));

		if(NOTTO(ch)) return ch;
 		if(t_exp(t1)) return TIMEOUT;
 		t1=t_set(cci);
		t2=t_set(5);
		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
			DEBUG(('E',2,"getchar '%c' %d (%d, %d)", C0(ch), ch, t_rest(t1), t_rest(t2)));

			if(NOTTO(ch)) return ch;
	 		if(!t_rest(t1)) return TIMEOUT;
			if(!got) got=1;
			if(got && (ch=='\r' || ch=='\n')) {
				*p=0;p=str;got=0;
				if(strstr(str, emsireq)) {
	                		sline("Received EMSI_REQ, sending EMSI_INQ...");
					DEBUG(('E',1,"Got EMSI_REQ"));
					PUTSTR((unsigned char*)emsiinq);PUTCHAR('\r');
					return OK;
				} else {
					str[79]=0;
					if(cfgi(CFG_SHOWINTRO)) if(*str) write_log("intro: %s", str);
				}
			}
			if(got && ch>=32 && ch<=255) *p++=ch;
			if((p-str)>=EMSI_BUF) {
				got=0;p=str;
			}
			if(!t_rest(t2)) {
				t2=t_set(5);
				tries++;
				if(tries > 10) return TIMEOUT;
				sline("Sending EMSI_INQ (Try %d of %d)...",tries,10);
				DEBUG(('E',1,"Sending EMSI_INQ (Try %d of %d)...",tries,10));
				PUTSTR((unsigned char*)emsiinq);
				PUTCHAR('\r');
				if (cfgi(CFG_STANDARDEMSI)) {
					PUTSTR((unsigned char*)emsiinq);
					PUTCHAR('\r');
				}
			}
		}
		return OK;
	}
	t1=t_set(cfgi(CFG_HSTIMEOUT));
	sline("Sending EMSI_REQ...");
	DEBUG(('E',1,"Sending EMSI_REQ"));
	PUTSTR((unsigned char*)emsireq);PUTCHAR('\r');
	sline("Waiting for EMSI_INQ...");
	DEBUG(('E',1,"Waiting for EMSI_INQ"));
	ch=tty_expect(emsiinq,cci);
	return ch;
}

int emsi_parsecod(char *lcod, char *ccod)
{
	char *q, *p;
	int o=0;
	chat_need=0;
	q=ccod;
	while((p=strsep(&q, ","))) {
		if(!strcmp(p, "TCP")) { o|=P_TCPP;continue;}
		if(!strcmp(p, "ZMO")) { o|=P_ZMODEM;continue;}
		if(!strcmp(p, "ZAP")) { o|=P_ZEDZAP;continue;}
		if(!strcmp(p, "DZA")) { o|=P_DIRZAP;continue;}
#ifdef HYDRA8K16K
		if(!strcmp(p, "HY8")) { o|=P_HYDRA8;continue;}
		if(!strcmp(p, "H16")) { o|=P_HYDRA16;continue;}
#endif/*HYDRA8K16K*/		
		if(!strcmp(p, "HYD")) { o|=P_HYDRA;continue;}
		if(!strcmp(p, "JAN")) { o|=P_JANUS;continue;}
		if(!strcmp(p, "NCP")) { o|=P_NCP;continue;}
		if(!strcmp(p, "CHT")) { chat_need=1;continue;}
		if(!strcmp(p, "NRQ")) { o|=O_NRQ;continue;}
		if(!strcmp(p, "FNC")) { o|=O_FNC;continue;}
		if(!strcmp(p, "XMA")) { o|=O_XMA;continue;}
	}	
	q=lcod;
	while((p=strsep(&q, ","))) {
		if(!strcmp(p, "RH1")) { o|=O_RH1;continue;}
		if(!strcmp(p, "PUA")) { o|=O_PUA;continue;}
		if(!strcmp(p, "PUP")) { o|=O_PUP;continue;}
		if(!strcmp(p, "NPU")) { o|=O_NPU;continue;}
		if(!strcmp(p, "HAT")) { o|=O_HAT;continue;}
		if(!strcmp(p, "HXT")) { o|=O_HXT;continue;}
		if(!strcmp(p, "HRQ")) { o|=O_HRQ;continue;}
	}
	return o;
}


char *findpwd(ftnaddr_t *a)
{
	faslist_t *cf;
	for(cf=cfgfasl(CFG_PASSWORD);cf;cf=cf->next)
		if(ADDRCMP(cf->addr, (*a)))
			return cf->str;
	return NULL;
}


