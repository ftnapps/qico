/**********************************************************
 * for work with various lists.
 **********************************************************/
/*
 * $Id: slists.h,v 1.3 2005/05/16 11:17:30 mitry Exp $
 *
 * $Log: slists.h,v $
 * Revision 1.3  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 */

#ifndef __SLISTS_H__
#define __SLISTS_H__

typedef struct _slist_t {
	char *str;
	struct _slist_t *next;
} slist_t;

typedef struct _aslist_t {
	char *str;
	char *arg;
	struct _aslist_t *next;
} aslist_t;

typedef struct _falist_t {
	ftnaddr_t addr;
	struct _falist_t *next;
} falist_t;

typedef struct _faslist_t {
	ftnaddr_t addr;
	char *str;
	struct _faslist_t *next;
} faslist_t;

typedef struct _flist_t {
	struct _flist_t *next;
	char *tosend, *sendas, kill;
	FILE *lo;
	off_t loff;
	int type;
	int suspend;
} flist_t;

slist_t		*slist_add(slist_t **, const char *);
slist_t		*slist_addl(slist_t **, const char *);
char		*slist_dell(slist_t **);
void		slist_kill(slist_t **);
void		slist_killn(slist_t **);
aslist_t	*aslist_add(aslist_t **, const char *, const char *);
aslist_t	*aslist_find(aslist_t *, const char *);
void		aslist_kill(aslist_t **);
void		falist_add(falist_t **, const ftnaddr_t *);
falist_t	*falist_find(falist_t *, const ftnaddr_t *);
void		falist_kill(falist_t **);
void		faslist_add(faslist_t **, const char *, const ftnaddr_t *);
void		faslist_kill(faslist_t **);

#endif
