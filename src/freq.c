/***************************************************************************
 * File: freq.c
 * File request support
 * Created at Fri Aug 18 23:48:45 2000 by pqr@yasp.com
 * $Id: freq.c,v 1.4 2000/10/11 21:35:46 lev Exp $
 ***************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "mailer.h"
#include "globals.h"
#include "qconf.h"
#include "ver.h"

int freq_pktcount=1;

int freq_ifextrp(slist_t *reqs)
{
	char s[MAX_PATH], fn[MAX_PATH], priv, *p;
	FILE *f, *g;
	int got=0;
	ftnaddr_t *ma=akamatch(&rnode->addrs->addr, cfgal(CFG_ADDRESS));

#ifdef S_DEBUG
	log("ifextrp job");
#endif
	priv='a';
	if(rnode->options&O_LST) priv='l';
	if(rnode->options&O_PWD) priv='p';
	
	sprintf(fn,"/tmp/qreq.%04x",getpid());
	f=fopen(fn,"wt");
	if(!f) {
		log("can't open '%s' for writing",fn);return got;
	}
	while(reqs) {
#ifdef S_DEBUG
		log("requested '%s'", reqs->str);
#endif
		fprintf(f, "%s\n", reqs->str);
		reqs=reqs->next;
	}
	fclose(f);
	
	sprintf(s, "%s -wazoo -%c -s%d %s /tmp/qreq.%04x /tmp/qfls.%04x /tmp/qrep.%04x",
			cfgs(CFG_EXTRP), priv, rnode->realspeed,
			ftnaddrtoa(&rnode->addrs->addr), getpid(),getpid(),getpid());
	log("exec '%s' returned rc=%d", s,
		execsh(s));
	lunlink(fn);
	
	sprintf(fn,"/tmp/qfls.%04x",getpid());
	f=fopen(fn,"rt");
	if(!f) {
		sprintf(fn,"/tmp/qrep.%04x",getpid());
		lunlink(fn);
		sprintf(fn,"/tmp/qfls.%04x",getpid());
		lunlink(fn);
		log("can't open '%s' for reading",fn);return got;
	}
	while(fgets(s,MAX_PATH-1,f)) {
		p=s+strlen(s)-1;
		while(*p=='\r' || *p=='\n') *p--=0;
		p=strrchr(s,' ');
		if(p) *p++=0;else p=s;
#ifdef S_DEBUG
		log("sending '%s' as '%s'", s, p);
#endif
		addflist(&fl, strdup(s), strdup((p!=s)?p:basename(s)), ' ',0,NULL,0);
		got=1;
	}
	fclose(f);lunlink(fn);
	
	sprintf(fn,"/tmp/qrep.%04x",getpid());
	f=fopen(fn,"rt");
	if(!f) log("can't open '%s' for reading",fn);
	
	sprintf(fn,"/tmp/qpkt.%04x%02x",getpid(),freq_pktcount);
	g=openpktmsg(ma, &rnode->addrs->addr,
				 rnode->sysop,cfgs(CFG_FREQFROM),
				 cfgs(CFG_FREQSUBJ),rnode->pwd,fn);
	if(!g) {
		log("can't open '%s' for writing",fn);
		if(f) fclose(f);
	}
	if(f && g) {
		while(fgets(s,MAX_PATH-1,f)) {
			p=s+strlen(s)-1;
			while(*p=='\r' || *p=='\n') *p--=0;
			fputs(s,g);fputc('\r',g);
		}
		fclose(f);
		sprintf(s, "%s-%s/%s", progname, version, osname);
		closepkt(g, ma, s, cfgs(CFG_STATION));
		sprintf(s,"/tmp/qpkt.%04x%02x",getpid(),freq_pktcount);p=strdup(s);
		sprintf(s,"%08lx.pkt", sequencer());
		addflist(&fl, p, strdup(s), '^',0,NULL,0);
		freq_pktcount++;
	}
	sprintf(fn,"/tmp/qrep.%04x",getpid());
	lunlink(fn);
	
	return got;
}
