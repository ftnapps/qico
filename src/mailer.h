/**********************************************************
 * File: mailer.h
 * Created at Thu Jul 15 16:16:07 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: mailer.h,v 1.7 2000/11/10 12:37:21 lev Exp $
 **********************************************************/
#ifndef __MAILER_H__
#define __MAILER_H__
#include "ftn.h"
#include "tty.h"
#include "opts.h"

#define ZMAXBLOCK  8192

#define P_NCP    1
#define P_ZMODEM 1<<1
#define P_ZEDZAP 1<<2
#define P_HYDRA  1<<3
#define P_JANUS  1<<4
#define P_TCPP   1<<5
#define P_HYDRA8 1<<6
#define P_HYDRA16 1<<7
#define P_MASK   255

#define S_OK      0
#define S_UNDIAL  1
#define S_REDIAL  2
#define S_BUSY    3
#define S_MASK    7

#define S_HOLDR   8
#define S_HOLDX   16
#define S_HOLDA   32
#define S_ANYHOLD (S_HOLDR|S_HOLDX|S_HOLDA)

#define FOP_OK      0
#define FOP_CONT    1
#define FOP_SKIP    2
#define FOP_ERROR   3
#define FOP_SUSPEND 4

#define SESSION_AUTO    0
#define SESSION_EMSI    1
#define SESSION_FTS0001 2
#define SESSION_YOOHOO  3
#define SESSION_BINKP   4

typedef struct _flist_t {
	struct _flist_t *next;
	char *tosend, *sendas, kill;
	FILE *lo;
	off_t loff;
	int type;
} flist_t;

int (*receive_callback)(char *fn);

int zmodem_receive(char *m);
int zmodem_send(slist_t *, slist_t *);
int zmodem_sendfile(char *tosend, char *sendas,
					unsigned long *totalleft, unsigned long
					*filesleft);
int zmodem_sendinit(int canzap);
int zmodem_senddone();
int session(int mode, int type, ftnaddr_t *calladdr, int speed);
int do_call(ftnaddr_t *fa, char *phone, char *port);
int compile_nodelists();
int tcp_call(char *host, ftnaddr_t *fa);

int execsh(char *cmd);
int execnowait(char *cmd,char *p1,char *p2,char *p3);

/* emsi.c */
extern int (*receive_callback)(char *fn);
extern char *emsireq;
extern char *emsiack;
extern char *emsiinq;
extern char *emsinak;
extern char *emsidat;
extern char *emsi_makedat(ftnaddr_t *remaddr, unsigned long mail,
						  unsigned long files, int lopt, char *protos,
						  falist_t *adrs, int showpwd);
extern char *emsi_tok(char **b, char *kc);
extern int hexdcd(char c);
extern void emsi_dcds(char *s);
extern int emsi_parsedat(char *str, ninfo_t *dat);
extern void emsi_log(ninfo_t *e);
extern int emsi_send(int mode, char *dat);
extern int emsi_recv(int mode, ninfo_t *rememsi);
extern int emsi_init(int mode);
extern int emsi_parsecod(char *lcod, char *ccod);
extern char *findpwd(ftnaddr_t *a);

extern int rxopen(char *name, time_t rtime, size_t rsize, FILE **f);
extern int rxclose(FILE **f, int what);
extern FILE *txopen(char *tosend, char *sendas);
extern int txclose(FILE **f, int what);
extern void log_rinfo(ninfo_t *e);
extern void check_cps();

#undef word
#undef byte
#undef dword
#define dword unsigned long
#define word unsigned short
#define byte unsigned char

extern FILE *txfd;
extern FILE *rxfd;  
extern long txpos;
extern long rxpos;
extern word txretries;
extern word rxretries;   
extern long txwindow;
extern long rxwindow;     
extern word txblklen;
extern word rxblklen;
extern long txsyncid;
extern long rxsyncid;    
extern byte *txbuf;
extern byte *rxbuf;
extern dword txoptions;
extern dword rxoptions;  
extern unsigned effbaud;
extern byte *rxbufptr;
extern byte txstate;
extern byte rxstate;
extern byte *rxbufmax;
extern long txstart;
extern long rxstart;  
extern word txmaxblklen;
extern word timeout;
extern byte txlastc;

#define GETCHAR(t) tty_getc(t)
#define PUTCHAR(c) tty_putc(c)
#define PUTSTR(s) tty_put(s, strlen(s))
#define HASDATA(t) tty_hasdata(t,0)
#define UHASDATA(t) tty_hasdata(0,t)
#define PURGE() tty_purge()
#define PURGEOUT() tty_purgeout()
#define PURGEALL() {tty_purge();tty_purgeout();}
#define CARRIER() (!tty_hangedup)
#define PUTBLK(bl, size) tty_put(bl,size)
#define CANCEL() tty_put(canistr, strlen(canistr))
#define BUFCHAR(c) tty_bufc(c)
#define BUFFLUSH() tty_bufflush()
#define BUFCLEAR() tty_bufclear()

extern void flexecute(flist_t *fl);
extern void addflist(flist_t **fl, char *loc, char *rem, char kill,
					 off_t off, FILE *lo, int sort);

extern void simulate_send(ftnaddr_t *fa);

extern int freq_ifextrp(slist_t *reqs);
extern int freq_pktcount;

#endif
