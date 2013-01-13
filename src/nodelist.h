/*
 * $Id: nodelist.h,v 1.4 2005/08/11 19:06:53 mitry Exp $
 *
 * $Log: nodelist.h,v $
 * Revision 1.4  2005/08/11 19:06:53  mitry
 * Added new macro MAILHOURS
 *
 * Revision 1.3  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 * Revision 1.2  2005/05/11 13:22:40  mitry
 * Snapshot
 *
 * Revision 1.1  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 */

#ifndef __NODELIST_H__
#define __NODELIST_H__

#include "slists.h"

#ifdef HAVE_ATTR_PACKED
#define PPACK			__attribute__ ((__packed__))
#else
#define PPACK
#endif

#define NDL_VER			0x0001

#define NDL_ENOPATH		0
#define NDL_ELOCKED		1
#define NDL_EOPENINDEX		2
#define NDL_EREADINDEXH		3
#define NDL_ENOISIGN		4
#define NDL_ENOVERSION		5
#define NDL_EVERSION		6
#define NDL_EREADINDEXD		7
#define NDL_EOUTOFDATE		8
#define NDL_EENTRYNOTFOUND	9
#define NDL_EOPENLIST		10
#define NDL_EREADLIST		11
#define NDL_ENEEDCOMPILE	12

#define MAILHOURS	( checktimegaps( cfgs( CFG_MAILONLY )) || checktimegaps( cfgs( CFG_ZMH )))

typedef struct {
	unsigned short	z, n, f, p;
} PPACK idxaddr_t;


/* nodelist entry */
typedef struct {
	idxaddr_t	addr;
	unsigned int	offset:24;
	unsigned int	index:8;
} PPACK idxent_t;


typedef struct {
	char		sign[20];	/* qico nodelist index\0 */
	unsigned short	version;	/* Internal index version ORed 0x8000 */
	off_t		offset;		/* Raw data offset */
	size_t		ecount;		/* Raw data entries count */
	short		nlists;		/* Number of lists in index */
} PPACK idxh_t;


typedef struct {
	time_t	mtime;			/* List's st_mtime */
	short	nsize;			/* List name size + 1 */
} PPACK idx_l_t;


typedef struct {
	idx_l_t	info;
	char	*name;
} PPACK idx_list_t;


typedef struct {
	char		*path;		/* Index name path */
	time_t		mtime;		/* Index mod. time */
	off_t		offset;		/* Offset of raw data in index */
	size_t		ecount;		/* Count of raw data entries in index */
	short		nlists;		/* Count of lists */
	idx_list_t	*lists;		/* Array of lists */
	idxent_t	*data;		/* Raw index data */
} idx_t;


extern char *ndl_errors[];

int	nodelist_compile(int);
int	nodelist_query(const ftnaddr_t *, ninfo_t **);
int	nodelist_listed(falist_t *, int);
void	nodelist_done(void);

void	phonetrans(char **, slist_t *);
int	checktimegaps(const char *);
subst_t	*parsesubsts(faslist_t *);
int	applysubst(ninfo_t *, subst_t *);
void	killsubsts(subst_t **);
int	can_dial(ninfo_t *, int);
int	find_dialable_subst(ninfo_t *, int, subst_t *);
void	nlkill(ninfo_t **);


#endif /* __NODELIST_H__ */
