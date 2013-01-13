/**********************************************************
 * File: anylist.c
 * Created at Sat Aug  4 11:51:09 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: anylist.c,v 1.1 2001/09/17 18:56:48 lev Exp $
 **********************************************************/
#include <config.h>
#include "types.h"

anylist_t *al_new(listop_alloc *alloc, listop_free *free, listop_cmp *cmp)
{
	anylist_t *res;
	if(!(res=xmalloc(sizeof(*res)))) return NULL;
	res->alloc = alloc;
	res->free = free;
	res->cmp = cmp;
	res->head = res->tail = NULL;
	return res;
}

void al_free(anylist_t *l)
{
	al_item_t *i, *j;
	if(!l) return;
	i = l->head;
	while(i) {
		j = i->next;
		l->free(i);
		i = j;
	}
	xfree(l);
}

void al_delk(anylist_t *l, void *key)
{
	al_item_t **pp, *p;
	if(!l) return;
	pp = &l->head;
	while(*pp&&l->cmp(*pp,key)) pp = &((*pp)->next);
	if(!*pp) return;
	p = *pp;
	*pp = (*pp)->next;
	l->free(p);
	if(l->tail==p) l->tail = *pp;
}

void al_deli(anylist_t *l, al_item_t *i)
{
	al_item_t **pp, *p;
	if(!l) return;
	pp = &l->head;
	while(*pp&&*pp!=i) pp = &((*pp)->next);
	if(!*pp) return;
	p = *pp;
	*pp = (*pp)->next;
	l->free(p);
	if(l->tail==p) l->tail = *pp;
}

void al_toend(al_item_t *l, al_item_t *i)
{
	if(!l||!i) return;
	i->next = NULL;
	if(l->tail) {
		l->tail->next = i;
	} else {
		l->tail = l->head = i;
	}
}

void al_tobeg(al_item_t *l, al_item_t *i)
{
	if(!l||!i) return;
	i->next = l->head;
	l->head = i;
	if(!l->tail) l->tail = i;
}

al_item_t *al_find(anylist_t *l, void *key)
{
	al_item_t *p;
	if(!l) return;
	p = l->head;
	while(p&&l->cmp(p,key)) p = p->next;
	return p;
}

al_item_t *al_finda(anylist_t *l, void *key, int *found)
{
	al_item_t *p;
	if(!l) return;
	p = l->head;
	while(p&&l->cmp(p,key)) p = p->next;
	if(p) {
		if(found) *found = 1;
		return p;
	}
	if(found) *found = 0;
	if(!(p=l->alloc(key))) return NULL;
	al_toend(l,p);
	return p;
}
