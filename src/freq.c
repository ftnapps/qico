/***************************************************************************
 * File: freq.c
 * File request support
 * Created at Fri Aug 18 23:48:45 2000 by pqr@yasp.com
 * $Id: freq.c,v 1.9 2001/03/10 19:50:17 lev Exp $
 ***************************************************************************/
#include "headers.h"

int freq_pktcount=1;

int freq_ifextrp(slist_t *reqs)
{
	char s[MAX_PATH], fn[MAX_PATH], priv, *p;
	FILE *f, *g;
	int got=0;
	ftnaddr_t *ma=akamatch(&rnode->addrs->addr, cfgal(CFG_ADDRESS));

	DEBUG(('S',1,"ifextrp job"));
	priv='a';
	if(rnode->options&O_LST) priv='l';
	if(rnode->options&O_PWD) priv='p';
	
	sprintf(fn,"/tmp/qreq.%04lx",(long)getpid());
	f=fopen(fn,"wt");
	if(!f) {
		write_log("can't open '%s' for writing",fn);return got;
	}
	while(reqs) {
		DEBUG(('S',1,"requested '%s'", reqs->str));
		fprintf(f, "%s\n", reqs->str);
		reqs=reqs->next;
	}
	fclose(f);
	
	sprintf(s, "%s -wazoo -%c -s%d %s /tmp/qreq.%04lx /tmp/qfls.%04lx /tmp/qrep.%04lx",
			cfgs(CFG_EXTRP), priv, rnode->realspeed,
			ftnaddrtoa(&rnode->addrs->addr), (long)getpid(),(long)getpid(),(long)getpid());
	write_log("exec '%s' returned rc=%d", s,
		execsh(s));
	lunlink(fn);
	
	sprintf(fn,"/tmp/qfls.%04lx",(long)getpid());
	f=fopen(fn,"rt");
	if(!f) {
		sprintf(fn,"/tmp/qrep.%04lx",(long)getpid());
		lunlink(fn);
		sprintf(fn,"/tmp/qfls.%04lx",(long)getpid());
		lunlink(fn);
		write_log("can't open '%s' for reading",fn);return got;
	}
	while(fgets(s,MAX_PATH-1,f)) {
		p=s+strlen(s)-1;
		while(*p=='\r' || *p=='\n') *p--=0;
		p=strrchr(s,' ');
		if(p) *p++=0;else p=s;
		DEBUG(('S',1,"sending '%s' as '%s'", s, p));
		addflist(&fl, strdup(s), strdup((p!=s)?p:basename(s)), ' ',0,NULL,0);
		got=1;
	}
	fclose(f);lunlink(fn);
	
	sprintf(fn,"/tmp/qrep.%04lx",(long)getpid());
	f=fopen(fn,"rt");
	if(!f) write_log("can't open '%s' for reading",fn);
	
	sprintf(fn,"/tmp/qpkt.%04lx%02x",(long)getpid(),freq_pktcount);
	g=openpktmsg(ma, &rnode->addrs->addr,
				 rnode->sysop,cfgs(CFG_FREQFROM),
				 cfgs(CFG_FREQSUBJ),rnode->pwd,fn);
	if(!g) {
		write_log("can't open '%s' for writing",fn);
		if(f) fclose(f);
	}
	if(f && g) {
		while(fgets(s,MAX_PATH-1,f)) {
			p=s+strlen(s)-1;
			while(*p=='\r' || *p=='\n') *p--=0;
			fputs(s,g);fputc('\r',g);
		}
		fclose(f);
		sprintf(s, "%s-%s/%s",
			cfgs(CFG_PROGNAME) == NULL ? progname :	cfgs(CFG_PROGNAME),
			cfgs(CFG_VERSION)  == NULL ? version  :	cfgs(CFG_VERSION),
			cfgs(CFG_OSNAME)   == NULL ? osname   : cfgs(CFG_OSNAME));
		closepkt(g, ma, s, cfgs(CFG_STATION));
		sprintf(s,"/tmp/qpkt.%04lx%02x",(long)getpid(),freq_pktcount);p=strdup(s);
		sprintf(s,"%08lx.pkt", sequencer());
		addflist(&fl, p, strdup(s), '^',0,NULL,0);
		freq_pktcount++;
	}
	sprintf(fn,"/tmp/qrep.%04lx",(long)getpid());
	lunlink(fn);
	
	return got;
}
