/* $Id: globals.h,v 1.8 2004/05/29 23:34:49 sisoft Exp $ */
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

extern char *ccs;
extern char *configname;
extern char *connstr;
extern char *devnull;
extern char ip_id[10];
extern char *log_name;
extern char *log_tty;
extern char *tty_port;
extern falist_t *ccal;
extern faslist_t *ccfasl;
extern flist_t *fl;
extern ftnaddr_t DEFADDR;
extern ftnaddr_t ndefaddr;
extern int bink;
extern int calling;
extern int cci;
extern int chatlg;
extern int chatprot;
extern int do_rescan;
extern int freq_pktcount;
extern int is_ip;
extern int runtoss;
extern int rxstatus;
extern int skipiftic;
extern int ssock,uis_sock,lins_sock;
extern int tty_hangedup;
extern int was_req,got_req;
extern time_t chattimer;
extern ninfo_t *rnode;
extern pfile_t sendf,recvf;
extern qitem_t *q_queue;
extern slist_t *ccsl;
extern subst_t *psubsts;
extern unsigned long totalf,totalm,totaln;

#endif
