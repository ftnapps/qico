/**********************************************************
 * work with various lists.
 **********************************************************/
/*
 * $Id: slists.c,v 1.3 2005/05/16 11:17:30 mitry Exp $
 *
 * $Log: slists.c,v $
 * Revision 1.3  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 */

#include "headers.h"

slist_t *slist_add(slist_t **l, const char *s)
{
	slist_t **t;

	for( t = l; *t; t = &((*t)->next ));

	*t = (slist_t *) xmalloc( sizeof( slist_t ));
	(*t)->next = NULL;
	(*t)->str = xstrdup( s );
	return *t;
}


slist_t *slist_addl(slist_t **l, const char *s)
{
	slist_t **t;

	for( t = l; *t; t = &((*t)->next ));

	*t = (slist_t *) xmalloc( sizeof( slist_t ));
	(*t)->next = NULL;
	(*t)->str = (char *) s;
	return *t;
}


char *slist_dell(slist_t **l)
{
	char	*p = NULL;
	slist_t	*t, *cc = NULL;

	for( t = *l; t && t->next; cc = t, p = t->next->str, t = t->next );

	if ( cc )
		xfree( cc->next );
	else {
		xfree( t );
		*l = NULL;
	}
	return p;
}


void slist_kill(slist_t **l)
{
	slist_t *t;

	while( *l ) {
		t = (*l)->next;
		xfree( (*l)->str );
		xfree( *l );
		*l = t;
	}
}


void slist_killn(slist_t **l)
{
	slist_t *t;

	while( *l ) {
		t = (*l)->next;
		xfree( *l );
		*l = t;
	}
}


aslist_t *aslist_add(aslist_t **l, const char *s, const char *a)
{
	aslist_t **t;

	for( t = l; *t; t = &((*t)->next) );

	*t = (aslist_t *) xmalloc( sizeof( aslist_t ));
	(*t)->next = NULL;
	(*t)->str = xstrdup( s );
	(*t)->arg = xstrdup( a );
	return *t;
}


aslist_t *aslist_find(aslist_t *l, const char *s)
{
	for(; l; l = l->next )
		if ( !strcmp( l->str, s ))
			return l;
	return NULL;
}


void aslist_kill(aslist_t **l)
{
	aslist_t *t;

	while( *l ) {
		t = (*l)->next;
		xfree( (*l)->str );
		xfree( (*l)->arg );
		xfree( *l );
		*l = t;
	}
}


void falist_add(falist_t **l, const ftnaddr_t *a)
{
	falist_t **t;

	for( t = l; *t; t = &((*t)->next) );

	*t = (falist_t *) xmalloc( sizeof( falist_t ));
	(*t)->next = NULL;
	addr_cpy( &(*t)->addr, a );
}


falist_t *falist_find(falist_t *l, const ftnaddr_t *a)
{
	for(; l; l = l->next )
		if ( addr_cmp( &l->addr, a ))
			return l;
	return NULL;
}


void falist_kill(falist_t **l)
{
	falist_t *t;

	while( *l ) {
		t = (*l)->next;
		xfree( (*l)->addr.d );
		xfree( *l );
		*l = t;
	}
}


void faslist_add(faslist_t **l, const char *s, const ftnaddr_t *a)
{
	faslist_t **t;

	for( t = l; *t; t = &((*t)->next) );

	*t = (faslist_t *) xmalloc( sizeof( faslist_t ));
	(*t)->next = NULL;
	(*t)->str = xstrdup( s );
	addr_cpy( &(*t)->addr, a );
}


void faslist_kill(faslist_t **l)
{
	faslist_t *t;

	while( *l ) {
		t = (*l)->next;
		xfree( (*l)->addr.d );
		xfree( (*l)->str );
		xfree( *l );
		*l = t;
	}
}
