/**********************************************************
 * global variables
 * $Id: globals.c,v 1.7 2004/02/13 22:29:01 sisoft Exp $
 **********************************************************/
#include "headers.h"

char *aso_tmp=NULL;
char *bso_tmp=NULL;
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
ftnaddr_t DEFADDR={0,0,0,0,NULL};
ftnaddr_t ndefaddr;
int bink=0;
int calling=0;
int cci;
int chatlg=0;
int chatprot=-1;
int do_rescan=0;
int freq_pktcount=1;
int is_ip=0;
int runtoss;
int rxstatus=0;
int skipiftic=0;
int ssock=-1,uis_sock=-1,lins_sock=-1;
int tty_hangedup=0;
int was_req,got_req;
time_t chattimer;
ninfo_t *rnode=NULL;
pfile_t sendf,recvf;
qitem_t *q_queue=NULL;
slist_t *ccsl;
subst_t *psubsts;
unsigned long totalf,totalm,totaln;
