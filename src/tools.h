/* $Id: tools.h,v 1.3 2004/02/09 01:05:33 sisoft Exp $ */
#ifndef __TOOLS_H__
#define __TOOLS_H__

#define EXC_OK 0
#define EXC_BADCONFIG 1

#define C_INT     1
#define C_STR     2
#define C_ADDR    3
#define C_ADRSTR  4
#define C_PATH    5
#define C_YESNO   6
#define C_ADDRL   7
#define C_ADRSTRL 8
#define C_STRL    9
#define C_OCT     10

typedef struct _cfgitem_t {
	char *condition;
	union {
		int v_int;
		char *v_char;
		falist_t *v_al;
		faslist_t *v_fasl;
		slist_t *v_sl;
	} value;
	struct _cfgitem_t *next;
} cfgitem_t;

typedef struct {
	char *keyword;
	int type, required, found;
	cfgitem_t *items;
	char *def_val;
} cfgstr_t;

#ifndef MAX
#define MAX(a,b) ((a>b)?a:b)
#endif
#ifndef MIN
#define MIN(a,b) ((a<b)?a:b)
#endif
#define C0(c) ((c>=32)?c:'.')
#define SIZES(x) (((x)<1024)?(x):((x)/1024))
#define SIZEC(x) (((x)<1024)?'b':'k')

extern void strlwr(char *s);
extern void strupr(char *s);
extern void strtr(char *s, char a, char b);
extern unsigned char todos(unsigned char c);
extern unsigned char tokoi(unsigned char c);
extern void stodos(unsigned char *str);
extern void stokoi(unsigned char *str);
extern char *chop(char *s, int n);
extern void strbin2hex(char *string,const unsigned char *binptr,int binlen);
extern int strhex2bin(unsigned char *binptr,const char *string);
extern unsigned long filesize(char *fname);
extern int lockpid(char *pidfn);
extern int islocked(char *pidfn);
extern unsigned long sequencer(void);
extern int mkdirs(char *name);
extern void rmdirs(char *name);
extern FILE *mdfopen(char *fn, char *pr);
extern size_t getfreespace(const char *path);
extern int randper(int base,int diff);
extern void to_dev_null();
#ifndef HAVE_SETPROCTITLE
extern void setargspace(int argc, char **argv, char **envp);
extern void setproctitle(char *str);
#endif
/* config.c */
extern int cfgi(int i);
extern char *cfgs(int i);
extern slist_t *cfgsl(int i);
extern faslist_t *cfgfasl(int i);
extern falist_t *cfgal(int i);
extern int readconfig(char *cfgname);
extern int parseconfig(char *cfgname);
extern void killconfig(void);
#ifdef NEED_DEBUG
extern void dumpconfig();
#endif
/* log.c */
extern void (*log_callback)(char *str);
extern int log_init(char *, char *);
extern void write_log(char *fmt, ...);
extern int chatlog_init(char *remname,ftnaddr_t *remaddr,int side);
extern void chatlog_write(char *text,int side);
extern void chatlog_done();
#ifdef NEED_DEBUG
extern int facilities_levels[256];
extern void parse_log_levels();
extern void write_debug_log(unsigned char facility, int level, char *fmt, ...);
#ifdef __GNUC__
#	define DEBUG(all)	 __DEBUG all
#	define __DEBUG(F,L,A...)	do { if(facilities_levels[(F)]>=(L)) write_debug_log((F),(L),##A); } while(0)
#else
#	define DEBUG(all)	 write_debug_log all
#endif
#else
#	define DEBUG(all)
#endif
extern void log_done(void);
/* gmtoff.c */
extern time_t gmtoff(time_t tt,int mode);
/* main.c */
extern RETSIGTYPE sigerr(int sig);
extern void stopit(int rc);
/* daemon.c */
extern void daemon_mode();

#endif
