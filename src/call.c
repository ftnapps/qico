/**********************************************************
 * outgoing call implementation
 * $Id: call.c,v 1.10 2004/05/27 18:50:03 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include "qipc.h"
#include "tty.h"

int do_call(ftnaddr_t *fa,char *site,char *port)
{
	int rc,fd=0;
	if(cfgs(CFG_RUNONCALL)) {
		char buf[MAX_PATH];
		snprintf(buf,MAX_PATH,"%s %s %s",ccs,ftnaddrtoa(fa),port?port:site);
		if((rc=execsh(buf)))write_log("exec '%s' returned rc=%d",buf,rc);
	}
	if(port) {
		rc=mdm_dial(site,port);
		switch(rc) {
			case MC_OK:
				rc=-1;
				break;
			case MC_RING:
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
				rc=S_REDIAL|S_ADDTRY;
				break;
			case MC_BAD:
				rc=S_OK;
				break;
		}
	} else {
		fd=tcp_dial(fa,site);
		if(fd>=0)rc=-1;
		    else rc=S_REDIAL|S_ADDTRY;
	}
	if(rc==-1) {
		rc=session(1,bink?SESSION_BINKP:SESSION_AUTO,fa,
		    port?atoi(connstr+strcspn(connstr,"0123456789")):TCP_SPEED);
		if(port)mdm_done();
		    else tcp_done(fd);
		if((rc&S_MASK)==S_REDIAL&&cfgi(CFG_FAILPOLLS)) {
			write_log("creating poll for %s",ftnaddrtoa(fa));
			if(BSO)bso_poll(fa,F_ERR);
			    else if(ASO)aso_poll(fa,F_ERR);
		}
	}
	title("Waiting...");
	vidle();
	sline("");
	return rc;
}

int force_call(ftnaddr_t *fa,int line,int flags)
{
	int rc;
	char *port=NULL;
	slist_t *ports=NULL;
	rc=query_nodelist(fa,cfgs(CFG_NLPATH),&rnode);
	if(rc>0)write_log(nlerr[rc-1]);
	if(!rnode) {
		rnode=xcalloc(1,sizeof(ninfo_t));
		falist_add(&rnode->addrs,fa);
		rnode->name=xstrdup("Unknown");
		rnode->phone=xstrdup("");
	}
	rnode->tty=NULL;
	ports=cfgsl(CFG_PORT);
	if((flags&2)!=2) {
		do {
			if(!ports)return S_FAILURE;
			port=tty_findport(ports,cfgs(CFG_NODIAL));
			if(!port)return S_FAILURE;
			if(rnode->tty)xfree(rnode->tty);
			rnode->tty=xstrdup(baseport(port));
			ports=ports->next;
		} while(!checktimegaps(cfgs(CFG_CANCALL)));
		if(!checktimegaps(cfgs(CFG_CANCALL)))return S_FAILURE;
	} else {
		if((port=tty_findport(ports,cfgs(CFG_NODIAL)))) {
			rnode->tty=xstrdup(baseport(port));
		} else return S_FAILURE;
	}
	if(!cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
	if(line) {
		applysubst(rnode,psubsts);
		while(rnode->hidnum&&line!=rnode->hidnum&&applysubst(rnode,psubsts)&&rnode->hidnum!=1);
		if(line!=rnode->hidnum) {
			write_log("%s doesn't have line #%d",ftnaddrtoa(fa),line);
			return S_NODIAL;
		}
		if(!can_dial(rnode,(flags&1)==1)) {
			write_log("We should not call to %s #%d at this time",ftnaddrtoa(fa),line);
			return S_NODIAL;
		}
	} else {
		if (!find_dialable_subst(rnode,((flags&1)==1),psubsts)) {
			write_log("We should not call to %s at this time",ftnaddrtoa(fa));
			return S_NODIAL;
		}
	}
	if(cfgi(CFG_TRANSLATESUBST))phonetrans(&rnode->phone,cfgsl(CFG_PHONETR));
	if(!log_init(cfgs(CFG_LOG),rnode->tty)) {
		write_log("can't open log %s",ccs);
		return S_FAILURE;
	}
	if(rnode->hidnum)
	    write_log("calling %s #%d, %s (%s)",rnode->name,rnode->hidnum,ftnaddrtoa(fa),rnode->phone);
		else write_log("calling %s, %s (%s)",rnode->name,ftnaddrtoa(fa),rnode->phone);
	rc=do_call(fa,rnode->phone,port);
	return rc;
}
