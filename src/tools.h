/* $Id: tools.h,v 1.16 2004/06/22 14:26:21 sisoft Exp $ */
#ifndef __TOOLS_H__
#define __TOOLS_H__

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
	slist_t *condition;
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
	int type,flags;
	cfgitem_t *items;
	char *def_val;
} cfgstr_t;

extern char *hexdigits;
extern char *engms[];
extern char *sigs[];
extern void recode_to_remote(char *str);
extern void recode_to_local(char *str);
extern int hexdcd(char d,char c);
extern slist_t *slist_add(slist_t **l, char *s);
extern slist_t *slist_addl(slist_t **l,char *s);
extern char *slist_dell(slist_t **l);
extern void slist_kill(slist_t **l);
extern void slist_killn(slist_t **l);
extern void strbin2hex(char *string,const unsigned char *binptr,size_t binlen);
extern int strhex2bin(unsigned char *binptr,const char *string);
extern size_t filesize(char *fname);
extern int lockpid(char *pidfn);
extern int islocked(char *pidfn);
extern unsigned long sequencer();
extern int mkdirs(char *name);
extern void rmdirs(char *name);
extern FILE *mdfopen(char *fn,char *pr);
extern size_t getfreespace(const char *path);
extern int randper(int base,int diff);
extern void to_dev_null();
extern int fexist(char *s);
extern char *fnc(char *s);
extern int whattype(char *fn);
extern int lunlink(char *s);
extern char *mapname(char *fn, char *map, size_t size);
extern int isdos83name(char *fn);
extern char *qver(int what);
extern int istic(char *fn);
extern int execsh(char *cmd);
extern int execnowait(char *cmd,char *p1,char *p2,char *p3);
/* config.c */
extern int cfgi(int i);
extern char *cfgs(int i);
extern slist_t *cfgsl(int i);
extern faslist_t *cfgfasl(int i);
extern falist_t *cfgal(int i);
extern int readconfig(char *cfgname);
extern int parsekeyword(char *kw,char *arg,char *cfgname,int line);
extern int parseconfig(char *cfgname);
extern void killconfig();
#ifdef NEED_DEBUG
extern void dumpconfig();
#endif
/* log.c */
extern void (*log_callback)(char *str);
extern int log_init(char *,char *);
extern void write_log(char *fmt, ...);
extern int chatlog_init(char *remname,ftnaddr_t *remaddr,int side);
extern void chatlog_write(char *text,int side);
extern void chatlog_done();
#ifdef NEED_DEBUG
extern int facilities_levels[256];
extern void parse_log_levels();
extern void write_debug_log(unsigned char facility,int level,char *fmt,...);
#ifdef __GNUC__
#define DEBUG(all) __DEBUG all
#define __DEBUG(F,L,A...) do { if(facilities_levels[(F)]>=(L)) write_debug_log((F),(L),##A); } while(0)
#else
#define DEBUG(all) write_debug_log all
#endif
#else
#define DEBUG(all)
#endif
extern void log_done();
/* main.c */
extern RETSIGTYPE sigerr(int sig);
extern void stopit(int rc);
/* daemon.c */
extern void daemon_mode();
/* flagexp.y */
extern int flagexp(slist_t *expr,int strict);

#endif
