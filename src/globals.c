/**********************************************************
 * global variables
 **********************************************************/
/*
 * $Id: globals.c,v 1.10 2005/03/31 19:28:16 mitry Exp $
 *
 * $Log: globals.c,v $
 * Revision 1.10  2005/03/31 19:28:16  mitry
 * Removed unused variable 'calling'
 *
 */

#define __GLOBALS_C__

#include "headers.h"

char *ccs;
char *configname=CONFIG;
char *connstr=NULL;
char *devnull="/dev/null";
char ip_id[10];
char *log_name=NULL;
char *log_tty=NULL;
char *tty_port=NULL;

falist_t *ccal;
faslist_t *ccfasl;
flist_t *fl=NULL;
FTNADDR_T(DEFADDR);
FTNADDR_T(ndefaddr);

int bink=0;
int cci;
int chatlg=0;
int chatprot=-1;
int detached = TRUE;
volatile int do_rescan = 0;
int freq_pktcount = 0;
int is_ip = 0;
int runtoss;
volatile int rxstatus = 0;
int skipiftic = 0;
int ssock = -1, uis_sock = -1, lins_sock = -1;
volatile int tty_online = 0;
volatile int tty_gothup = 0;
int verbose_config = 0;
int was_req,got_req;
int emsi_lo = 0;
volatile time_t chattimer;
ninfo_t *rnode=NULL;
pfile_t sendf,recvf;
qitem_t *q_queue=NULL;
slist_t *ccsl;
subst_t *psubsts;
unsigned long totalf,totalm,totaln;
