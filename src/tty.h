/* $Id: tty.h,v 1.1.1.1 2003/07/12 21:27:18 sisoft Exp $ */
#ifndef __TTY_H__
#define __TTY_H__
#include "ftn.h"
#include <time.h>
#include <termios.h>

#define MODEM_OK 0
#define MODEM_PORTLOCKED -1
#define MODEM_PORTACCESSDENIED -2
#define ME_OK 0
#define ME_ATTRS 1
#define ME_SPEED 2
#define ME_OPEN 3
#define ME_READ 4
#define ME_WRITE 5
#define ME_TIMEOUT 6
#define ME_CLOSE 7
#define ME_CANTLOCK 8
#define ME_FLAGS 9
#define ME_NOTATT 10

#define MC_OK 0
#define MC_FAIL 1
#define MC_ERROR 2
#define MC_BUSY 3
#define MC_NODIAL 4
#define MC_RING 5

extern char *tty_errs[];
extern char *tty_port;
extern int tty_hangedup;
extern int calling;
extern void tty_sighup(int sig);
extern int selectmy(int n,fd_set *rfs,fd_set *wfs,fd_set *efs,struct timeval *to);
extern int tty_isfree(char *port, char *nodial);
extern char *tty_findport(slist_t *ports, char *nodial);
extern int tty_openport(char *port);
extern void tty_unlock(char *port);
extern int tty_lock(char *port);
extern int tty_open(char *port, int speed);
extern int tty_setattr(int speed);
extern speed_t tty_transpeed(int speed);
extern int tty_local(void);
extern int tty_nolocal(void);
extern int tty_cooked(void);
extern int tty_close(void);
extern int tty_unblock(void);
extern int tty_block();
extern int tty_put(unsigned char *buf, int size);
extern int tty_get(unsigned char *buf, int size, int *timeout);
extern int tty_bufc(char ch);
extern int tty_bufflush();
extern void tty_bufclear();

extern int tty_putc(char ch);
extern int tty_getc(int timeout);
extern int tty_getc_timed(int *timeout);
extern int tty_hasdata(int sec, int usec);
extern int tty_hasdata_timed(int *timeout);
extern void tty_purge();
extern void tty_purgeout();
extern char canistr[];
extern int tty_gets(char *what, int n, int timeout);
extern int tty_expect(char *what, int timeout);
extern char *baseport(char *p);
extern int modem_sendstr(char *cmd);
extern int modem_chat(char *cmd, slist_t *oks, slist_t *dns, slist_t *ers, slist_t *bys,
					  char *ringing, int maxr, int timeout, char *rest, size_t restlen);
extern int modem_stat(char *cmd, slist_t *oks, slist_t *ers, 
		  int timeout, char *stat, size_t stat_len);

extern pid_t getsid(pid_t pid);

#define M_STAT (tty_hangedup?"hangup":"ok")

#define t_start() time(NULL) 
#define t_isexp(timer,dif) ((time(NULL)-timer) >= dif)
#define t_time(timer) (time(NULL)-timer)
#define t_set(expire) (time(NULL)+expire)
#define t_exp(timer) (time(NULL) > timer)
#define t_rest(timer) (timer - time(NULL))

#endif
