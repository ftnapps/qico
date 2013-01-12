/**********************************************************
 * for work with various lists.
 * $Id: slists.h,v 1.1 2004/06/24 09:53:32 sisoft Exp $
 **********************************************************/
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
} flist_t;

extern slist_t *slist_add(slist_t **l, char *s);
extern slist_t *slist_addl(slist_t **l,char *s);
extern char *slist_dell(slist_t **l);
extern void slist_kill(slist_t **l);
extern void slist_killn(slist_t **l);
extern aslist_t *aslist_add(aslist_t **l,char *s,char *a);
extern aslist_t *aslist_find(aslist_t *l,char *s);
extern void aslist_kill(aslist_t **l);
extern void falist_add(falist_t **l, ftnaddr_t *a);
extern falist_t *falist_find(falist_t *l,ftnaddr_t *a);
extern void falist_kill(falist_t **l);
extern void faslist_add(faslist_t **l, char *s, ftnaddr_t *a);
extern void faslist_kill(faslist_t **l);

#endif
