/***************************************************************************
 * File request support
 * $Id: freq.c,v 1.1.1.1 2003/07/12 21:26:34 sisoft Exp $
 ***************************************************************************/
#include "headers.h"

int freq_pktcount=1;

int freq_ifextrp(slist_t *reqs)
{
	FILE *f,*g,*r;
	char s[MAX_PATH],fn[MAX_PATH],sfn[MAX_PATH],*ss;
	char priv='a',*p,*sprt="UNPROTEC",*slst="UNLIS";
	int got=0,wz=cfgs(CFG_EXTRP)?1:0,kil;
	long tpid=(long)getpid();
	ftnaddr_t *ma=akamatch(&rnode->addrs->addr,cfgal(CFG_ADDRESS));

	DEBUG(('R',1,"Freq received"));
	if(rnode->options&O_LST){priv='l';slst+=2;}
	if(rnode->options&O_PWD){priv='p';sprt+=2;}

	snprintf(fn,MAX_PATH,"/tmp/qreq.%04lx",tpid);
	if(!(f=fopen(fn,"wt"))) {
		write_log("can't open '%s' for writing",fn);
		return 0;
	}
	while(reqs) {
		stokoi((unsigned char*)reqs->str);
		DEBUG(('R',1,"requested '%s'",reqs->str));
		fprintf(f,"%s\n",reqs->str);
		reqs=reqs->next;
	}
	fclose(f);
	if(!wz) {
		falist_t *ra;
		snprintf(sfn,MAX_PATH,"/tmp/qsrif.%04lx",tpid);
		if(!(r=fopen(sfn,"wt"))){write_log("can't open '%s' for writing",sfn);return 0;}
		fprintf(r,"SessionType EMSI\n");
		fprintf(f,"Sysop %s\n",rnode->sysop);
		for(ra=rnode->addrs;ra;ra=ra->next)fprintf(r,"AKA %s\n",ftnaddrtoa(&ra->addr));
		fprintf(r,"Baud %d\n",rnode->realspeed);
		fprintf(r,"Time -1\n");
		fprintf(r,"RemoteStatus %sTED\n",sprt);
		fprintf(r,"SystemStatus %sTED\n",slst);
		fprintf(r,"RequestList /tmp/qreq.%04lx\n",tpid);
		fprintf(r,"ResponseList /tmp/qfls.%04lx\n",tpid);
		fprintf(r,"Location %s\n",rnode->place);
		fprintf(r,"Phone %s\n",rnode->phone);
		if(rnode->options&O_PWD)fprintf(r,"Password %s\n",rnode->pwd);
		fprintf(r,"Mailer %s\n",rnode->mailer);
		fprintf(r,"Site %s\n",rnode->name);
		if((ss=getenv("CALLER_ID"))&&strcasecmp(ss,"none")&&strlen(ss)>3)fprintf(r,"CallerID %s\n",ss);
		fprintf(r,"OurAKA %s\n",ftnaddrtoa(ma));
		fprintf(r,"TRANX %08lu\n",time(NULL));
		fclose(r);
		snprintf(s,MAX_PATH,"%s %s",cfgs(CFG_SRIFRP),sfn);
	} else snprintf(s,MAX_PATH,"%s -wazoo -%c -s%d %s /tmp/qreq.%04lx /tmp/qfls.%04lx /tmp/qrep.%04lx",
			cfgs(CFG_EXTRP),priv,rnode->realspeed,ftnaddrtoa(&rnode->addrs->addr),tpid,tpid,tpid);
	write_log("exec '%s' returned rc=%d",s,execsh(s));
	lunlink(fn);lunlink(sfn);
	
	snprintf(fn,MAX_PATH,"/tmp/qfls.%04lx",tpid);
	if(!(f=fopen(fn,"rt"))) {
		snprintf(fn,MAX_PATH,"/tmp/qrep.%04lx",tpid);
		lunlink(fn);
		snprintf(fn,MAX_PATH,"/tmp/qfls.%04lx",tpid);
		lunlink(fn);
		write_log("can't open '%s' for reading",fn);
		return 0;
	}
	while(fgets(s,MAX_PATH-1,f)) {
		if(*s=='\n'||*s=='\r'||*s==' '||!*s)continue;
		ss=s;kil=0;got=1;
		if(*s=='='||*s=='-'){ss++;kil=1;}
		    else if(*s=='+')ss++;
		p=ss+strlen(ss)-1;
		while(*p=='\r'||*p=='\n')*p--=0;
		p=strrchr(ss,' ');
		if(p)*p++=0;else p=ss;
		DEBUG(('R',1,"sending '%s' as '%s'%s",ss,p,kil?" and kill":""));
		addflist(&fl,xstrdup(ss),xstrdup((p!=s)?p:basename(s)),kil?'^':' ',0,NULL,0);
	}
	fclose(f);lunlink(fn);
	
	snprintf(fn,MAX_PATH,"/tmp/qrep.%04lx",tpid);
	if(!(f=fopen(fn,"rt"))&&wz)write_log("can't open '%s' for reading",fn);
	
	snprintf(fn,MAX_PATH,"/tmp/qpkt.%04lx%02x",tpid,freq_pktcount);
	g=openpktmsg(ma,&rnode->addrs->addr,cfgs(CFG_FREQFROM),rnode->sysop,cfgs(CFG_FREQSUBJ),NULL,fn,1);
	if(!g) {
		write_log("can't open '%s' for writing",fn);
		if(f)fclose(f);
	}
	if(f&&g) {
		while(fgets(s,MAX_PATH-1,f)) {
			p=s+strlen(s)-1;
			while(*p=='\r'||*p=='\n')*p--=0;
			stodos((unsigned char*)s);
			fputs(s,g);fputc('\r',g);
		}
		fclose(f);
		closeqpkt(g,ma);
		snprintf(s,MAX_PATH,"/tmp/qpkt.%04lx%02x",tpid,freq_pktcount);
		p=xstrdup(s);
		snprintf(s,MAX_PATH,"%08lx.pkt",sequencer());
		addflist(&fl,p,xstrdup(s),'^',0,NULL,1);
		freq_pktcount++;
	}
	snprintf(fn,MAX_PATH,"/tmp/qrep.%04lx",tpid);
	lunlink(fn);
	
	return got;
}
