/* $Id: ftn.h,v 1.21 2004/06/23 17:59:35 sisoft Exp $ */
#ifndef __FTN_H__
#define __FTN_H__

#define F_ERR  0
#define F_NORM 1
#define F_HOLD 2
#define F_DIR  3
#define F_CRSH 4
#define F_IMM  5
#define F_REQ  6
#define F_MAX  F_REQ

#define T_UNKNOWN 0
#define T_ARCMAIL 1
#define T_NETMAIL 2
#define T_REQ     4

#define NT_NORMAL 0
#define NT_DOWN   1
#define NT_HOLD   2
#define NT_PVT    3
#define NT_HUB    4

#define IS_ERR    4
#define IS_REQ    3
#define IS_FILE   2
#define IS_ARC    1
#define IS_PKT    0

#define LCK_x   0
#define LCK_c	1
#define LCK_t	2
#define LCK_s   3

#define BSO	1
#define ASO	2

typedef struct {
	short z,n,f,p;
	char *d;
} ftnaddr_t;

#define FTNADDR_T(a) ftnaddr_t (a)={0,0,0,0,NULL}

#include "slists.h"

typedef struct {
	char sign[20];
	char nlname[MAX_NODELIST][20];
	time_t nltime[MAX_NODELIST];
} idxh_t;

typedef struct {
	ftnaddr_t addr;
	unsigned offset:24;
	unsigned index:8;
} idxent_t;

typedef struct {
	falist_t *addrs;
	char *name,*place,*sysop,*phone,*wtime,*flags,*pwd,*mailer,*host,*tty;
	int options,speed,realspeed,netmail,files,type,haswtime,hidnum,holded,opt;
	time_t time,starttime;
} ninfo_t;

typedef struct _qitem_t {
	ftnaddr_t addr;
	int try,flv,what,touched;
	off_t sizes[F_MAX+1];
	time_t times[F_MAX+1];
	off_t reqs,pkts;
	time_t onhold;
	struct _qitem_t *next;
} qitem_t;

typedef struct _dialine_t {
	char *phone,*timegaps,*host;
	int num,flags;
	struct _dialine_t *next;
} dialine_t;

typedef struct _subst {
	ftnaddr_t addr;
	dialine_t *hiddens,*current;
	int nhids;
	struct _subst *next;
} subst_t;

typedef struct {
	UINT16 phONode,
		phDNode,
		phYear,
		phMonth,
		phDay,
		phHour,
		phMinute,
		phSecond,
		phBaud,
		phType,
		phONet,
		phDNet;
	UINT8	phPCode,
		phMajor,
		phPass[8];
	UINT16 phQOZone,
		phQDZone,
		phAuxNet,
		phCWValidate;
	UINT8 phPCodeHi,
		phPRevMinor;
	UINT16 phCaps,
		phOZone,
		phDZone,
		phOPoint,
		phDPoint;
	UINT16 phLongData[2];
} pkthdr_t;

typedef struct {
	UINT16 pmType;
	UINT16 pmONode;
	UINT16 pmDNode;
	UINT16 pmONet;
	UINT16 pmDNet;
	UINT16 pmAttr;
	UINT16 pmCost;
} pktmhdr_t;

typedef struct {
	int flags;
	char *name;
	size_t size;
	time_t time;
} bp_status_t;

typedef struct {
	int try,flags;
	time_t htime,utime;
	bp_status_t bp;
} sts_t;

extern void addr_cpy(ftnaddr_t *a,ftnaddr_t *b);
extern int addr_cmp(ftnaddr_t *a,ftnaddr_t *b);
extern int parseftnaddr(char *s, ftnaddr_t *a, ftnaddr_t *b, int wc);
extern ftnaddr_t *akamatch(ftnaddr_t *a, falist_t *akas);
extern char *ftnaddrtoa(ftnaddr_t *a);
extern char *ftnaddrtoda(ftnaddr_t *a);
extern char *ftnaddrtoia(ftnaddr_t *a);
extern char *strip8(char *s);
extern int has_addr(ftnaddr_t *a, falist_t *l);
extern char *engms[];
extern int showpkt(char *fn);
extern FILE *openpktmsg(ftnaddr_t *fa, ftnaddr_t *ta, char *from, char *to, char *subj, char *pwd, char *fn,unsigned attr);
extern void closepkt(FILE *f, ftnaddr_t *fa, char *tear, char *orig);
extern void closeqpkt(FILE *f,ftnaddr_t *fa);
extern falist_t *falist_find(falist_t *, ftnaddr_t *);
extern int havestatus(int status, int cfgkey);
extern int needhold(int status, int what);
extern int xfnmatch(char *pattern,char *name,int flags);
extern char *findpwd(ftnaddr_t *a);
/* nodelist.c */
extern char *NL_SIGN;
extern char *NL_IDX;
extern char *nlerr[];
extern int query_nodelist(ftnaddr_t *addr, char *nlpath, ninfo_t **nl);
extern int is_listed(falist_t *addr, char *nlpath, int needall);
extern void phonetrans(char **pph, slist_t *phtr);
extern int checktimegaps(char *ranges);
extern int chktxy(char *p);
extern int checktxy(char *flags);
extern subst_t *findsubst(ftnaddr_t *fa, subst_t *subs);
extern subst_t *parsesubsts(faslist_t *sbs);
extern int applysubst(ninfo_t *nl, subst_t *subs);
extern void killsubsts(subst_t **l);
extern int can_dial(ninfo_t *nl, int ct);
extern int find_dialable_subst(ninfo_t *nl, int ct, subst_t *subs);
extern void nlfree(ninfo_t *nl);
extern void nlkill(ninfo_t **nl);
extern int compile_nodelists();
/* aso.c */
extern int aso_init(char *asopath,char *bsopath,char *stspath,int def_zone);
extern void aso_done();
extern int aso_flavor(char fl);
extern int aso_rescan(void (*each)(char *, ftnaddr_t *, int, int,int),int rslow);
extern int aso_locknode(ftnaddr_t *adr,int lev);
extern int aso_unlocknode(ftnaddr_t *adr,int lev);
extern int aso_attach(ftnaddr_t *adr, int flv, slist_t *files);
extern int aso_request(ftnaddr_t *adr, slist_t *files);
extern int aso_poll(ftnaddr_t *fa, int flavor);
extern int aso_setstatus(ftnaddr_t *fa, sts_t *st);
extern int aso_getstatus(ftnaddr_t *fa, sts_t *st);
extern int asoflist(flist_t **fl,ftnaddr_t *fa,int mode);
/* queue.c */
extern qitem_t *q_find(ftnaddr_t *fa);
extern int q_rescan(qitem_t **curr,int rslow);
extern off_t q_sum(qitem_t *q);
extern void qsendqueue();

#endif
