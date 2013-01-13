/* $Id: globals.h,v 1.6 2005/03/31 19:28:16 mitry Exp $
 *
 * $Log: globals.h,v $
 * Revision 1.6  2005/03/31 19:28:16  mitry
 * Removed unused variable 'calling'
 *
 */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#ifndef __GLOBALS_C__

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
extern int cci;
extern int chatlg;
extern int chatprot;
extern int detached;
extern int do_rescan;
extern int freq_pktcount;
extern int is_ip;
extern int runtoss;
extern int rxstatus;
extern int skipiftic;
extern int ssock,uis_sock,lins_sock;
extern int tty_online;
extern int tty_gothup;
extern int verbose_config;
extern int was_req,got_req;
extern int emsi_lo;
extern time_t chattimer;
extern ninfo_t *rnode;
extern pfile_t sendf,recvf;
extern qitem_t *q_queue;
extern slist_t *ccsl;
extern subst_t *psubsts;
extern unsigned long totalf,totalm,totaln;

#endif
#endif
