/**********************************************************
 * File: anylist.h
 * Created at Sun Jul  1 00:43:23 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: anylist.h,v 1.1 2001/07/05 20:40:33 lev Exp $
 **********************************************************/
#ifndef __ANYLIST_H__
#define __ANYLIST_H__

/* Universal list descriptor -- for generic procedures */
typedef struct _ANYLISTELEM {
	struct _ANYLISTELEM *next;
	CHAR data[];
} anylistelem_t;


typedef anylist_t *(*listop_alloc)(void *key);
typedef void (*listop_free)(anylistelem_t *l);
typedef int (*listop_cmp)(anylist_t *l, void *key);

/* Head of list -- operations and first element pointer */
typedef struct _ANYLIST {
	listop_alloc *alloc;
	listop_free *free;
	listop_cmp *cmp;
	anylistelelm *head;
} anylist_t;

anylist_t *al_new(listop_alloc *alloc, listop_free *free, listop_cmp *cmp);
void al_free(anylist_t *l);

/* Dispose element by key */
void al_delk(anylist_t *l, void *key);
/* Dispose element by pointer */
void al_dele(anylist_t *l, anylistelem_t *e);
/* Insert in end of list */
void al_toend(anylist_t *l, anylistelem_t *e);
/* Insert in begin of lst */
void al_tobeg(anylist_t *l, anylistelem_t *e);
/* Find by key */
anylistelem_t *al_find(anylist_t *l, void *key);
/* Find by key, add new in end, if not found */
anylistelem_t *al_finda(anylist_t *l, void *key, int *found);

#endif/*__ANYLIST_H__*/
