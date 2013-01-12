/*
 * $Id: ftn.h,v 1.11 2005/08/16 15:17:22 mitry Exp $
 *
 * $Log: ftn.h,v $
 * Revision 1.11  2005/08/16 15:17:22  mitry
 * Removed unused ninfo_t.haswtime field
 *
 * Revision 1.10  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.9  2005/05/11 16:34:01  mitry
 * Removed commented out typedefs
 *
 * Revision 1.8  2005/05/11 13:22:40  mitry
 * Snapshot
 *
 * Revision 1.7  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 * Revision 1.6  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.5  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.4  2005/02/08 19:53:14  mitry
 * Added outbound_addr_busy()
 *
 */

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

#define IS_ERR		5
#define IS_FLO		4
#define IS_REQ		3
#define IS_FILE		2
#define IS_ARC		1
#define IS_PKT		0

#define LCK_x   0
#define LCK_c	1
#define LCK_t	2
#define LCK_s   3

#define BSO	1
#define ASO	2

typedef struct {
	int z, n, f, p;
	char *d;
} ftnaddr_t;

#define FTNADDR_T(a) ftnaddr_t (a)={0,0,0,0,NULL}

#include "slists.h"

typedef struct {
	falist_t *addrs;
	char *name,*place,*sysop,*phone,*wtime,*flags,*pwd,*mailer,*host,*tty;
	int options,speed,realspeed,netmail,files,type,hidnum,holded,opt;
	time_t time,starttime;
} ninfo_t;

typedef struct _qitem_t {
	ftnaddr_t	addr;
	int		try, flv, what, touched, canpoll;
	off_t		reqs, pkts;
	off_t		sizes[F_MAX+1];
	time_t		times[F_MAX+1];
	time_t		onhold;
	struct _qitem_t	*next;
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

typedef void (*qeach_t)(const char *, const ftnaddr_t *, int, int, int);

void	addr_cpy(ftnaddr_t *, const ftnaddr_t *);
int	addr_cmp(const ftnaddr_t *, const ftnaddr_t *);
int	parseftnaddr(const char *, ftnaddr_t *, const ftnaddr_t *, int);
ftnaddr_t	*akamatch(const ftnaddr_t *, falist_t *);
char	*ftnaddrtoa(const ftnaddr_t *);
char	*ftnaddrtoda(const ftnaddr_t *);
char	*ftnaddrtoia(const ftnaddr_t *);
char	*strip8(char *);
int	has_addr(const ftnaddr_t *, falist_t *);
int	showpkt(const char *);
FILE	*openpktmsg(const ftnaddr_t *, const ftnaddr_t *, char *, char *, char *, char *, char *, unsigned);
void	closepkt(FILE *, const ftnaddr_t *, const char *, char *);
void	closeqpkt(FILE *, const ftnaddr_t *);
int	whattype(const char *);
int	istic(const char *);
int	havestatus(int, int);
int	needhold(int, int);
int	xfnmatch(char *, const char *, int);
char	*findpwd(const ftnaddr_t *);


/* outbound.c */
int	outbound_init(const char *, const char *, const char *, int);
void	outbound_done(void);
int	outbound_rescan(qeach_t, int);
int	outbound_addr_busy(const ftnaddr_t *);
int	outbound_locknode(const ftnaddr_t *, int);
int	outbound_unlocknode(const ftnaddr_t *, int);
int	outbound_flavor(char fl);
int	outbound_attach(const ftnaddr_t *, int, slist_t *);
int	outbound_request(const ftnaddr_t *, slist_t *);
int	outbound_poll(const ftnaddr_t *, int);
int	outbound_setstatus(const ftnaddr_t *fa, sts_t *);
int	outbound_getstatus(const ftnaddr_t *fa, sts_t *);
int	asoflist(flist_t **fl, const ftnaddr_t *, int);

/* queue.c */
int	q_cmp(const void *, const void *);
qitem_t	*q_find(const ftnaddr_t *);
qitem_t	*q_add(const ftnaddr_t *);
int	q_recountflo(const char *, off_t *, time_t *, int);
void	q_recountbox(const char *, off_t *, time_t *, int);
void	q_each(const char *, const ftnaddr_t *, int, int, int);
off_t	q_sum(const qitem_t *);
void	rescan_boxes(int);
int	q_rescan(qitem_t **, int);
void	qsendqueue(void);

#endif
