/**********************************************************
 * File: emsi.c
 * Created at Thu Jul 15 16:11:11 1999 by pk // aaz@ruxy.org.ru
 * EMSI
 * $Id: emsi.c,v 1.1 2000/07/18 12:37:18 lev Exp $
 **********************************************************/
#include "mailer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qconf.h"
#include "ver.h"
#include "qipc.h"
#include "globals.h"
#include "defs.h"

#ifdef E_DEBUG
#define sline log
#undef PUTSTR
#undef PUTCHAR
#define PUTSTR(x) {m_puts(x);log("putstr %d %x", strlen(x), strlen(x));}
#define PUTCHAR(x) {m_putc(x);log("putchar %x '%c'", x, (x>=32)?x:'.');}
#endif

#define EMSI_BUF 65536
	
#define HS_TIMEOUT 60

char *emsireq="**EMSI_REQA77E";
char *emsiack="**EMSI_ACKA490";
char *emsiinq="**EMSI_INQC816";
char *emsinak="**EMSI_NAKEEC3";
char *emsidat="**EMSI_DAT";

char *emsi_makedat(ftnaddr_t *remaddr, unsigned long mail,
				   unsigned long files, int lopt, char *protos,
				   falist_t *adrs, int showpwd)	
{
	char *dat=calloc(EMSI_BUF,1), tmp[1024], *p;int c;
	falist_t *cs;ftnaddr_t *ba;
	time_t tm=time(NULL);
	struct tm *tt=localtime(&tm);
	
	strcpy(dat, "**EMSI_DAT0000{EMSI}{");
	ba=akamatch(remaddr, adrs?adrs:cfgal(CFG_ADDRESS));
	strcat(dat, ftnaddrtoa(ba));
	for(cs=cfgal(CFG_ADDRESS);cs;cs=cs->next)
		if(&cs->addr!=ba) {
			strcat(dat," ");				   
			strcat(dat, ftnaddrtoa(&cs->addr));
		}
	strcat(dat, "}{");
	if(showpwd) {
		p=findpwd(remaddr);if(p) strcat(dat, p);
	}
	strcat(dat, "}{8N1,RH1");
	if(lopt&O_PUA) strcat(dat,",PUA");
	if(lopt&O_PUP) strcat(dat,",PUP");
	if(lopt&O_HRQ) strcat(dat,",HRQ");
	if(lopt&O_HXT) strcat(dat,",HXT");
	if(lopt&O_HAT) strcat(dat,",HAT");
	strcat(dat, "}{XMA");
	if(lopt&O_NRQ) strcat(dat,",NRQ");
	p=protos;c=0;
	if(!(lopt&P_NCP)) while(*p) {
		switch(toupper(*p++)) {
		case '1':strcat(dat, ",ZMO");c=1;break;
		case 'Z':strcat(dat, ",ZAP");c=1;break;
		case 'H':strcat(dat, ",HYD");c=1;break;
		case '8':strcat(dat, ",HY8");c=1;break;
		case '6':strcat(dat, ",H16");c=1;break;
		case 'J':strcat(dat, ",JAN");c=1;break;
		case 'T':strcat(dat, ",TCP");c=1;break;
		}
	}
	if(!c) strcat(dat, ",NCP");

	sprintf(tmp, "}{FE}{%s}{%s}{%s}",
		cfgs(CFG_PROGNAME) == NULL ? progname :	strip8(cfgs(CFG_PROGNAME)),
		cfgs(CFG_VERSION)  == NULL ? version  :	strip8(cfgs(CFG_VERSION)),
		cfgs(CFG_OSNAME)   == NULL ? osname   : strip8(cfgs(CFG_OSNAME)));

	strcat(dat, tmp);
	/* TODO: 8bit conversion */
	sprintf(tmp,
			"{IDENT}{[%s][%s][%s][%s][%d][%s]}{TRAF}{%lX %lX}{OHFR}{%s%s%s}{TRX#}{[%lX]}{TZUTC}{[%+03ld00]}",
			strip8(cfgs(CFG_STATION)),strip8(cfgs(CFG_PLACE)),
			strip8(cfgs(CFG_SYSOP)),strip8(cfgs(CFG_PHONE)),
			cfgi(CFG_SPEED),strip8(cfgs(CFG_FLAGS)),
			mail, files, cfgs(CFG_WORKTIME),
			cfgs(CFG_FREQTIME)?" ":"",cfgs(CFG_FREQTIME)?cfgs(CFG_FREQTIME):"",
			time(NULL)+tt->tm_gmtoff,tt->tm_gmtoff/3600
		);
	strcat(dat, tmp);
	sprintf(tmp, "%04X", strlen(dat)-14);
	memcpy(dat+10,tmp,4);
	sprintf(tmp, "%04X",crc16s(dat+2));
	strcat(dat, tmp);
	return dat;
}

char *emsi_tok(char **b, char *kc)
{
	char *p=*b, *t;
	int st=0;
	if(*p!=kc[0]) return NULL;
	t=++p;
	while(st<2) {
		switch(st) {
		case 0:
			if(*t==kc[1]) st=1;
			if(!*t) st=3;else t++;
			break;
		case 1:
			if(*t!=kc[1]) {st=2;t--;}
			else {st=0;t++;}
			break;
		}
	}
	if(st==3) return NULL;
	*t++=0;
	*b=t;
	return p;
}


int hexdcd(char c)
{
	c=tolower(c);
	if(c>='a'&&c<='f') return c-'a';
	return c-'0';
}

void emsi_dcds(char *s)
{
	char *d=s;
	while(*s) {
		if(*s=='}'||*s==']') s++;
		*d++=(*s=='\\')?((hexdcd(*(++s))<<4)+hexdcd(*(++s))):(((*s)<32)?'.':*s);
		s++;
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
	if(!(str=strstr(str, "**EMSI_DAT"))) return 0;
	sscanf(str+10, "%04X", &l);
	if(l!=strlen(str)-18) return 0;
	sscanf(str+strlen(str)-4, "%04X", &l);
	if(l!=crc16(str+2, strlen(str)-6)) return 0;
	if(strncmp(str+14, "{EMSI}",6)) return 0;
	t=str+20;
	str[0]=1;
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	falist_kill(&dat->addrs);
	while((s=strsep(&p," "))) 
		if(parseftnaddr(s, &fa, NULL, 0))
			if(!falist_find(dat->addrs, &fa)) falist_add(&dat->addrs,&fa);
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	xstrcpy(&dat->pwd,p);
	emsi_dcds(dat->pwd);
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	lcod=p;
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	ccod=p;
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	xstrcpy(&dat->mailer,p);
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	xstrcat(&dat->mailer,"-");
	xstrcat(&dat->mailer, p);
	p=emsi_tok(&t,"{}");
	if(!p) return 0;
	xstrcat(&dat->mailer,"/");
	xstrcat(&dat->mailer, p);
	emsi_dcds(dat->mailer);

	dat->options|=emsi_parsecod(lcod, ccod);
#ifdef E_DEBUG	
	log("emsi codes %s/%s",lcod,ccod); 
#endif

	free(dat->wtime);dat->wtime=NULL;
	while((p=emsi_tok(&t,"{}"))) {
		if(!strcmp(p, "IDENT")) {
			p=emsi_tok(&t,"{}");
			if(!p) return 0;
			s=emsi_tok(&p, "[]");
			xstrcpy(&dat->name, s);
			emsi_dcds(dat->name);
			s=emsi_tok(&p, "[]");
			xstrcpy(&dat->place, s);
			emsi_dcds(dat->place);
			s=emsi_tok(&p, "[]");
			xstrcpy(&dat->sysop, s);
			emsi_dcds(dat->sysop);
			s=emsi_tok(&p, "[]");
			xstrcpy(&dat->phone, s);
			emsi_dcds(dat->phone);
 			s=emsi_tok(&p, "[]"); 
 			if(s) sscanf(s,"%d",&dat->speed); 
 			s=emsi_tok(&p, "[]"); 
 			xstrcpy(&dat->flags, s); 
			emsi_dcds(dat->flags);
		} else if(!strcmp(p, "TRAF")) {
			p=emsi_tok(&t,"{}");
			if(!p) return 0;
			sscanf(p, "%x %x", &dat->netmail, &dat->files);	
		} else if(!strcmp(p, "OHFR")) {
			p=emsi_tok(&t,"{}");
			if(!p) return 0;
			xstrcpy(&dat->wtime, p);
		} else if(!strcmp(p, "MOH#")) {
			p=emsi_tok(&t, "{}");
			if(!p) return 0;
			s=emsi_tok(&p, "[]");
			if(sscanf(s, "%x", &dat->holded)!=1) return 0;
		} else if(!strcmp(p, "TRX#")) {
			p=emsi_tok(&t,"{}");
			if(!p) return 0;
			s=emsi_tok(&p, "[]");
			if(sscanf(s, "%lx", &dat->time)!=1) return 0;			
		} else p=emsi_tok(&t,"{}");
	}		
	return 1;
}


int emsi_send(int mode, char *dat)
{
	time_t t1, t2;
	int tries=0, got=0, ch;
	char str[MAX_STRING], *p=str;

	bzero(str, MAX_STRING);
	t1=t_set(60);
	while(1) {
		sline("Sending EMSI_DAT");
		PUTSTR(dat);PUTCHAR('\r');
		tries++;
		if(tries>10) return TIMEOUT;
		t2=t_set(20);got=0;p=str;
		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
/*   			log("er: '%c' %03d %d %d", C0(ch), ch, got, p-str);   */
			if(NOTTO(ch)) return ch;
			if(t_exp(t1)) return TIMEOUT;
			if(t_exp(t2)) break;
			if(!got && ch=='*') got=1;
			if(got && (ch=='\r' || ch=='\n')) {
#ifdef E_DEBUG	
				log("got str '%s' %d", str, strlen(str));
#endif
				*p=0;p=str;got=0;
				if(!strncmp(str, emsiack, 14)) {
					sline("Got EMSI_ACK");return OK;
				}
				if(!strncmp(str, emsireq, 14)) {
					sline("Skipping EMSI_REQ");continue;
				}
				if(!strncmp(str, emsiack, 7)) break;
			}
			if(got) *p++=ch;
			if((p-str)>=MAX_STRING) {
				got=0;p=str;
			}
			
		}		
	}
}

int emsi_recv(int mode, ninfo_t *rememsi)
{
	int tries, got=0, ch;
	time_t t1,t2;
	char str[EMSI_BUF], *p=str;

	
	t1=t_set(20);t2=t_set(60);tries=0;
	while(1) {
		tries++;
		if(tries>10) return TIMEOUT;
		if(!mode) {
			sline("Sending EMSI_REQ (%d)...", tries);
			PUTSTR(emsireq);PUTCHAR('\r');
			t1=t_set(20);
		} else if(tries>1) {
			sline("Sending EMSI_NAK...");
			PUTSTR(emsinak);PUTCHAR('\r');
			t1=t_set(20);
		}

		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
/*  			log("er: '%c' %03d %d %d", C0(ch), ch, got, p-str);  */
			if(NOTTO(ch)) return ch;
			if(ch<0) break;
			if(!got && ch=='*') got=1;
			if(got && (ch=='\r' || ch=='\n')) {
				*p=0;p=str;got=0;
#ifdef E_DEBUG	
				if(strstr(str, emsidat))
					log("emsidat at offs %d", strstr(str, emsidat)-str);
				log("got str '%s' %d", str, strlen(str));
#endif
				if(!strncmp(str, emsidat, 10)) {
#ifdef E_DEBUG	
					log("got emsidat!");
#endif
					sline("Received EMSI_DAT");
					ch=emsi_parsedat(str, rememsi);
#ifdef E_DEBUG	
					log("parse %d", ch);
#endif
					if(ch) {
						sline("Sending EMSI_ACK...");
						PUTSTR(emsiack);PUTCHAR('\r');
						return OK;
					} else break;
				}
			}
			if(got) *p++=ch;
			if((p-str)>=EMSI_BUF) {
				log("too long EMSI packet!");
				got=0;p=str;
			}
			if(t_exp(t2)) {
				sline("Timeout receiving EMSI_DAT");return TIMEOUT;
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

	if(mode) {
		t1=t_set(HS_TIMEOUT);
		PUTCHAR('\r');  
		do {
/* 			PUTCHAR('\r');   */
			ch=HASDATA(0);
		} while (!ISTO(ch) && !t_isexp(t1, HS_TIMEOUT));

/* 		if(NOTTO(ch)) return ch; */
/* 		if(t_exp(t1)) return TIMEOUT; */
		t2=t_set(5);
		while(1) {
			ch=GETCHAR(MIN(t_rest(t1),t_rest(t2)));
#ifdef E_DEBUG	
			log("getchar '%c' %d", C0(ch), ch);
#endif
			if(NOTTO(ch)) return ch;
			if(!got) got=1;
			if(got && (ch=='\r' || ch=='\n')) {
				*p=0;p=str;got=0;
				if(strstr(str, emsireq)) {
#ifdef E_DEBUG	
					log("got emsireq!");
#endif
					sline("Received EMSI_REQ, sending EMSI_INQ...");
					PUTSTR(emsiinq);PUTCHAR('\r');
					return OK;
				} else {
					str[79]=0;
					if(cfgi(CFG_SHOWINTRO)) if(*str) log("intro: %s", str);
				}
			}
			if(got && ch>=32 && ch<=255) *p++=ch;
			if((p-str)>=EMSI_BUF) {
				got=0;p=str;
			}
			if(t_exp(t2)) {
				t2=t_set(5);
				PUTSTR(emsiinq);PUTCHAR('\r');
/* 				PUTSTR(emsiinq);PUTCHAR('\r'); */
			}
		}
		return OK;
	}
	t1=t_set(HS_TIMEOUT);
	sline("Sending EMSI_REQ...");
	PUTSTR(emsireq);PUTCHAR('\r');
	sline("Waiting for EMSI_INQ...");
	ch=tty_expect(emsiinq, HS_TIMEOUT);  /* ???????????????????????? */
	return ch;
}

int emsi_parsecod(char *lcod, char *ccod)
{
	char *q, *p;
	int o=0;

	q=ccod;
	while((p=strsep(&q, ","))) {
		if(!strcmp(p, "TCP")) { o|=P_TCPP;continue;}
		if(!strcmp(p, "ZMO")) { o|=P_ZMODEM;continue;}
		if(!strcmp(p, "ZAP")) { o|=P_ZEDZAP;continue;}
		if(!strcmp(p, "HY8")) { o|=P_HYDRA8;continue;}
		if(!strcmp(p, "H16")) { o|=P_HYDRA16;continue;}
		if(!strcmp(p, "HYD")) { o|=P_HYDRA;continue;}
		if(!strcmp(p, "NCP")) { o|=P_NCP;continue;}
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


