/**********************************************************
 * outgoing call implementation
 * $Id: call.c,v 1.1.1.1 2003/07/12 21:26:29 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "tty.h"

char *mcs[]={"ok","fail","error","busy","no dialtone","ring"};

int alive()
{
	char *ac;
	int rc=MC_OK;
	ac=cfgs(CFG_MODEMALIVE);
	DEBUG(('M',4,"alive: checking modem..."));
	rc=modem_chat(ac,cfgsl(CFG_MODEMOK),cfgsl(CFG_MODEMERROR),cfgsl(CFG_MODEMNODIAL),
			 cfgsl(CFG_MODEMBUSY),cfgs(CFG_MODEMRINGING),
			 cfgi(CFG_MAXRINGS),2,NULL,0);
	if(rc!=MC_OK)DEBUG(('M',3,"alive: failed, rc=%d",rc));
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
// ???		if(rc!=MC_OK)sleep(1);
		tty_purge();
	}
	if(rc!=MC_OK)DEBUG(('M',3,"hangup: failed, rc=%d",rc));
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

int reset()
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
	if(rc!=MC_OK) write_log("modem reset failed [%s]",mcs[rc]);
	return rc;
}

int do_call(ftnaddr_t *fa,char *phone,char *port)
{
	int rc,ringm=0;
	char s[MAX_STRING],conn[MAX_STRING];

	if((rc=tty_openport(port))) {
		write_log("can't open port: %s",tty_errs[rc]);
		return 0;
	}

	reset();
	
	xstrcpy(s,cfgs(CFG_DIALPREFIX),MAX_STRING);
	xstrcat(s,phone,MAX_STRING);xstrcat(s,cfgs(CFG_DIALSUFFIX),MAX_STRING);

	tty_local();

	sline("Dialing %s",s);vidle();
	rc=modem_chat(s,cfgsl(CFG_MODEMCONNECT),cfgsl(CFG_MODEMNODIAL),cfgsl(CFG_MODEMERROR),
			cfgsl(CFG_MODEMBUSY),cfgs(CFG_MODEMRINGING),cfgi(CFG_MAXRINGS),
			cfgi(CFG_WAITCARRIER),conn,MAX_STRING);
	sline("Modem said: %s",conn);
	if(rc!=MC_OK) {
		write_log("got %s",conn);
		title("Waiting...");
		vidle();
		switch(rc) {		
			case MC_RING:
				ringm=1;
			case MC_BUSY:
				rc=S_BUSY;
				break;
			case MC_ERROR:
				rc=S_REDIAL|S_ADDTRY;
				break;
			case MC_NODIAL:
				rc=S_NODIAL;
				break;
			case MC_FAIL:
				hangup();
				rc=S_REDIAL|S_ADDTRY;
		}
		if(!ringm)sline("Call failed");
		    else sline("RING found..");
		tty_close();
		if(ringm) {
			sleep(2);
			execsh("killall -USR1 mgetty vgetty >/dev/null 2>&1");
		}
		return rc;
	}
	write_log("*** %s",conn);
	tty_nolocal();
	if(rc==MC_OK) {
		rc=session(1,SESSION_AUTO,fa,atoi(conn+strcspn(conn,"0123456789")));
		if((rc&S_MASK)==S_REDIAL) {
			write_log("creating poll for %s",ftnaddrtoa(fa));
			bso_poll(fa,F_ERR);
		}
	} else rc=S_REDIAL;
	title("Waiting...");
	vidle();
	sline("");
	tty_local();
	hangup();
	stat_collect();
	tty_close();
	return rc;
}
