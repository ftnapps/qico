/**********************************************************
 * File: call.c
 * Created at Sun Jul 25 22:15:36 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: call.c,v 1.2 2000/11/01 10:29:23 lev Exp $
 **********************************************************/
#include "ftn.h"
#include <stdlib.h>
#include <string.h>
#include "tty.h"
#include "mailer.h"
#include "qconf.h"
#include "qipc.h"

char *mcs[]={"ok", "fail", "error", "busy"};

int hangup()
{
	slist_t *hc;
	int rc=MC_OK;
	if(!cfgsl(CFG_MODEMHANGUP)) return MC_OK; 
	write_log("hanging up...");
	for(hc=cfgsl(CFG_MODEMHANGUP);hc;hc=hc->next)
		rc=modem_chat(hc->str, cfgsl(CFG_MODEMOK),
					  cfgsl(CFG_MODEMERROR), cfgsl(CFG_MODEMBUSY),
					  cfgs(CFG_MODEMRINGING), cfgi(CFG_MAXRINGS), 
					  cfgi(CFG_WAITRESET), NULL);
	return rc;
}

int reset()
{
	slist_t *hc;
	int rc=MC_OK;
	if(!cfgsl(CFG_MODEMRESET)) return MC_OK; 
	write_log("resetting modem...");
	for(hc=ccsl;hc && rc==MC_OK;hc=hc->next)
		rc=modem_chat(hc->str, cfgsl(CFG_MODEMOK),
					  cfgsl(CFG_MODEMERROR), cfgsl(CFG_MODEMBUSY),
					  cfgs(CFG_MODEMRINGING), cfgi(CFG_MAXRINGS), 
					  cfgi(CFG_WAITRESET), NULL);
	if(rc!=MC_OK) write_log("modem reset failed [%s]", mcs[rc]);
	return rc;
}

int do_call(ftnaddr_t *fa, char *phone, char *port)
{
	int rc;
	char s[MAX_STRING], conn[MAX_STRING];

	if((rc=tty_openport(port))) {
		write_log("can't open port: %s", tty_errs[rc]);
		return 0;
	}

	reset();
	
	strcpy(s,cfgs(CFG_DIALPREFIX));
	strcat(s, phone);strcat(s, cfgs(CFG_DIALSUFFIX));

	tty_local();

	sline("Dialing %s", s);vidle();
	rc=modem_chat(s,
				  cfgsl(CFG_MODEMCONNECT), cfgsl(CFG_MODEMERROR),
				  cfgsl(CFG_MODEMBUSY), cfgs(CFG_MODEMRINGING),
				  cfgi(CFG_MAXRINGS), cfgi(CFG_WAITCARRIER),
				  conn);
	sline("Modem said: %s", conn);
	if(rc!=MC_OK) {
		write_log("got %s",conn);
		title("Waiting...");
		vidle();
		switch(rc) {		
		case MC_BUSY:
			rc=S_BUSY;
			break;
		case MC_ERROR:
			rc=S_REDIAL;
			break;
		case MC_FAIL:
			hangup();
			rc=S_REDIAL;
		}
		sline("Call failed");
		tty_close();
		return rc;
	}
	write_log("*** %s", conn);
	tty_nolocal();
	if(rc==MC_OK) {
		rc=session(1, SESSION_AUTO, fa, atoi(conn+strcspn(conn,"0123456789")));
		if((rc&S_MASK)==S_REDIAL) {
			write_log("creating poll for %s", ftnaddrtoa(fa));
			bso_poll(fa);
		}
	} else rc=S_REDIAL;
	title("Waiting...");
	vidle();
	sline("");
	tty_local();
	hangup();
	tty_close();
	return rc;
}
