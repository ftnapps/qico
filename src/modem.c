/**********************************************************
 * work with modem
 * $Id: modem.c,v 1.5 2004/06/19 22:31:57 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "qipc.h"
#include "tty.h"

static char *mcs[]={"ok","fail","error","busy","no dialtone","ring","bad"};

int modem_sendstr(char *cmd)
{
	int rc=1;
	if(!cmd) return 1;
	DEBUG(('M',1,">> %s",cmd));
	while(*cmd && rc>0) {
		switch(*cmd) {
		case '|': rc=write(STDOUT_FILENO, "\r", 1);usleep(300000L);break;
		case '~': sleep(1);rc=1;break;
		case '\'': usleep(200000L);rc=1;break;
		case '^': rc=tty_setdtr(1); break;
		case 'v': rc=tty_setdtr(0); break;
		default: rc=write(STDOUT_FILENO, cmd, 1);
		DEBUG(('M',4,">>> %c",C0(*cmd)));
		}
		cmd++;
	}
	if(rc>0) DEBUG(('M',4,"modem_sendstr: sent"));
	    else DEBUG(('M',3,"modem_sendstr: error, rc=%d, errno=%d",rc,errno));
	return rc;
}

int modem_chat(char *cmd, slist_t *oks, slist_t *nds, slist_t *ers, slist_t *bys,
			   char *ringing, int maxr, int timeout, char *rest, size_t restlen)
{
	char buf[MAX_STRING];
	int rc,nrng=0;
	slist_t *cs;
	time_t t1;
	calling=1;
	DEBUG(('M',4,"modem_chat: cmd=\"%s\" timeout=%d",cmd,timeout));
	rc=modem_sendstr(cmd);
	if(rc!=1) {
		if(rest) xstrcpy(rest, "FAILURE", restlen);
		DEBUG(('M',3,"modem_chat: modem_sendstr failed, rc=%d",rc));
		return MC_FAIL;
	}
	if(!oks && !ers && !bys) return MC_OK;
	rc=OK;
	t1=t_set(timeout);
	while(ISTO(rc) && !t_exp(t1) && (!maxr || nrng<maxr)) {
		getevt();
		rc=tty_gets(buf, MAX_STRING-1, t_rest(t1));
		if(rc==RCDO) {
			if(rest)xstrcpy(rest,"HANGUP",restlen);
	    		return MC_BUSY;
		}
		if(rc!=OK) {
			if(rest) xstrcpy(rest, "FAILURE", restlen);
			DEBUG(('M',3,"modem_chat: tty_gets failed, rc=%d",rc));
			return MC_FAIL;
		}
		if(!*buf)continue;
		for(cs=oks;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_OK;
			}
		for(cs=ers;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_ERROR;
			}
		if(ringing&&!strncmp(buf,ringing,strlen(ringing))) {
			if(!nrng&&strlen(ringing)==4) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_RING;
			}
			nrng++;
			continue;
		}
		for(cs=nds;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_NODIAL;
			}
		for(cs=bys;cs;cs=cs->next)
			if(!strncmp(buf,cs->str,strlen(cs->str))) {
				if(rest)xstrcpy(rest,buf,restlen);
				return MC_BUSY;
			}
	}
	if(rest) {
		if(nrng && maxr && nrng>=maxr) snprintf(rest, restlen, "%d RINGINGs", nrng);
		    else if(ISTO(rc)) xstrcpy(rest, "TIMEOUT", restlen);
			else xstrcpy(rest, "FAILURE", restlen);
	}
	return MC_FAIL;
}

static int modem_stat(char *cmd, slist_t *oks, slist_t *ers,int timeout, char *stat, size_t stat_len)
{
	char buf[MAX_STRING];int rc;
	slist_t *cs;time_t t1=t_set(timeout);

	DEBUG(('M',4,"modem_stat: cmd=\"%s\" timeout=%d",cmd,timeout));

	rc=modem_sendstr(cmd);
	if(rc!=1) {
		if(stat) xstrcpy(stat, "FAILURE", stat_len);
		DEBUG(('M',3,"modem_stat: modem_sendstr failed, rc=%d",rc));
		return MC_FAIL;
	}
	if(!oks && !ers) return MC_OK;
	rc=OK;
	while(ISTO(rc) && !t_exp(t1)) {
		rc=tty_gets(buf, MAX_STRING-1, t_rest(t1));
		if(!*buf) continue;
		if(rc!=OK) {
			if(stat) xstrcat(stat, "FAILURE",
				stat_len);
			DEBUG(('M',3,"modem_stat: tty_gets failed"));
			return MC_FAIL;
		}
		for(cs=oks;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(stat) xstrcat(stat, buf,
					stat_len);
				return MC_OK;
			}
		for(cs=ers;cs;cs=cs->next)
			if(!strncmp(buf, cs->str, strlen(cs->str))) {
				if(stat) xstrcat(stat, buf,
					stat_len);
				return MC_ERROR;
			}
		if(stat)
			{
			xstrcat(stat,buf,stat_len);
		 	xstrcat(stat,"\n",stat_len);
			}
	}

	if(stat) {
		if (ISTO(rc)) xstrcat(stat, "TIMEOUT", stat_len);
		else xstrcat(stat, "FAILURE", stat_len);
	}
	return MC_FAIL;
}

int alive()
{
	char *ac;
	int rc=MC_OK;
	ac=cfgs(CFG_MODEMALIVE);
	DEBUG(('M',4,"alive: checking modem..."));
	rc=modem_chat(ac,cfgsl(CFG_MODEMOK),cfgsl(CFG_MODEMERROR),cfgsl(CFG_MODEMNODIAL),
			 cfgsl(CFG_MODEMBUSY),cfgs(CFG_MODEMRINGING),
			 cfgi(CFG_MAXRINGS),2,NULL,0);
#ifdef NEED_DEBUG
	if(rc!=MC_OK)DEBUG(('M',3,"alive: failed, rc=%d [%s]",rc,mcs[rc]));
#endif
	return rc;
}

int hangup()
{
	slist_t *hc;
	int rc=MC_FAIL,to=t_set(cfgi(CFG_WAITRESET));
	if(!cfgsl(CFG_MODEMHANGUP))return MC_OK;
	write_log("hanging up...");
	while(rc!=MC_OK&&!t_exp(to)) {
		for(hc=cfgsl(CFG_MODEMHANGUP);hc;hc=hc->next) {
			rc=modem_chat(hc->str,cfgsl(CFG_MODEMOK),cfgsl(CFG_MODEMNODIAL),
				cfgsl(CFG_MODEMERROR),cfgsl(CFG_MODEMBUSY),
				cfgs(CFG_MODEMRINGING),cfgi(CFG_MAXRINGS),
				t_rest(to),NULL,0);
		}
		if(rc!=MC_OK)sleep(1);
		tty_purge();
		rc=alive();
		tty_purge();
	}
#ifdef NEED_DEBUG
	if(rc!=MC_OK)DEBUG(('M',3,"hangup: failed, rc=%d [%s]",rc,mcs[rc]));
#endif
	return rc;
}

int stat_collect()
{
	slist_t *hc;
	int rc=MC_OK,stat_len=8192;
	char stat[8192],*cur_stat,*p;
	if(!cfgsl(CFG_MODEMSTAT))return MC_OK;
	write_log("collecting statistics...");
	for(hc=cfgsl(CFG_MODEMSTAT);hc;hc=hc->next) {
		*stat=0;
		rc=modem_stat(hc->str,cfgsl(CFG_MODEMOK),cfgsl(CFG_MODEMERROR),
					cfgi(CFG_WAITRESET),stat,stat_len);
		for(cur_stat=stat;*cur_stat;) {
			for(p=cur_stat;*p&&*p!='\n';p++);
			if(*p)*(p++)=0;
			write_log("%s",cur_stat);
			cur_stat=p;
		}
	}
	return rc;
}

static int reset()
{
	slist_t *hc;
	int rc=MC_OK;
	if(!cfgsl(CFG_MODEMRESET)) return MC_OK;
	write_log("resetting modem...");
	for(hc=ccsl;hc && rc==MC_OK;hc=hc->next)
		rc=modem_chat(hc->str,cfgsl(CFG_MODEMOK),cfgsl(CFG_MODEMNODIAL),
					  cfgsl(CFG_MODEMERROR),cfgsl(CFG_MODEMBUSY),
					  cfgs(CFG_MODEMRINGING),cfgi(CFG_MAXRINGS),
					  cfgi(CFG_WAITRESET),NULL,0);
	if(rc!=MC_OK) write_log("modem reset failed, rc=%d [%s]",rc,mcs[rc]);
	return rc;
}

int mdm_dial(char *phone,char *port)
{
	int rc;
	char s[MAX_STRING],conn[MAX_STRING];
	if((rc=tty_openport(port))) {
		write_log("can't open port: %s",tty_errs[rc]);
		return MC_BAD;
	}
	reset();
	xstrcpy(s,cfgs(CFG_DIALPREFIX),MAX_STRING);
	xstrcat(s,phone,MAX_STRING);
	xstrcat(s,cfgs(CFG_DIALSUFFIX),MAX_STRING);
	tty_local(1);
	sline("Dialing %s",s);vidle();
	rc=modem_chat(s,cfgsl(CFG_MODEMCONNECT),cfgsl(CFG_MODEMNODIAL),cfgsl(CFG_MODEMERROR),
			cfgsl(CFG_MODEMBUSY),cfgs(CFG_MODEMRINGING),cfgi(CFG_MAXRINGS),
			cfgi(CFG_WAITCARRIER),conn,MAX_STRING);
	sline("Modem said: %s",conn);
	xfree(connstr);connstr=xstrdup(conn);
	if(rc!=MC_OK) {
		write_log("got %s",conn);
		if(rc==MC_FAIL)hangup();
		tty_close();
		if(rc==MC_RING) {
			sline("RING found..");
			sleep(2);
			execsh("killall -USR1 mgetty vgetty >/dev/null 2>&1");
		} else sline("Call failed (%s)",mcs[rc]);
		return rc;
	}
	write_log("*** %s",conn);
	tty_local(0);
	return rc;
}

void mdm_done()
{
	tty_local(1);
	hangup();
	stat_collect();
	tty_close();
}
