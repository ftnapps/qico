/**********************************************************
 * File: anylist.h
 * Created at Sun Jul  1 00:43:23 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: anylist.h,v 1.2 2001/09/17 18:56:48 lev Exp $
 **********************************************************/
#ifndef __ANYLIST_H__
#define __ANYLIST_H__

/* Universal list descriptor -- for generic procedures */
typedef struct _AL_ITEM {
	struct _AL_ITEM *next;
	CHAR data[];
} al_item_t;


typedef al_item_t *(*listop_alloc)(void *key);
typedef void (*listop_free)(al_item_t *l);
typedef int (*listop_cmp)(al_item_t *l, void *key);

/* Head of list -- operations and first element pointer */
typedef struct _ANYLIST {
	listop_alloc *alloc;
	listop_free *free;
	listop_cmp *cmp;
	al_item_t *head;
	al_item_t *tail;
} anylist_t;

anylist_t *al_new(listop_alloc *alloc, listop_free *free, listop_cmp *cmp);
void al_free(anylist_t *l);

/* Dispose element by key */
void al_delk(anylist_t *l, void *key);
/* Dispose element by pointer */
void al_deli(anylist_t *l, al_item_t *i);
/* Insert in end of list */
void al_toend(al_item_t *l, al_item_t *i);
/* Insert in begin of lst */
void al_tobeg(al_item_t *l, al_item_t *i);
/* Find by key */
al_item_t *al_find(anylist_t *l, void *key);
/* Find by key, add new in end, if not found */
al_item_t *al_finda(anylist_t *l, void *key, int *found);

#endif/*__ANYLIST_H__*/
